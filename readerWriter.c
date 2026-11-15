/*
 * readerWriter.c
 *
 * CIS 450 — Operating Systems
 * Starter framework for Project 4: One-Lane Bridge
 *
 * This program shows the classic readers-writers solution built from semaphores
 * (OSTEP Chapter 31, "Reader-Writer Locks").  Many readers may use the shared
 * resource at the same time, but a writer needs the resource to itself.
 *
 * Each reader or writer is one thread.  The main thread creates them one at a time
 * using a fixed arrival schedule, the same way Project 4 creates one thread per
 * vehicle.  Every thread runs the same three steps:
 *
 *		ArriveLock()	->	UseResource()	->	LeaveLock()
 *
 * which line up with the bridge project's:
 *
 *		ArriveBridge()	->	CrossBridge()	->	ExitBridge()
 *
 * The program prints the time whenever a thread STARTS using the resource.
 *
 * ---------------------------------------------------------------------------------
 * BUILDING AND RUNNING
 * ---------------------------------------------------------------------------------
 * The same file builds on Linux and on Windows.  A small portability layer near the
 * top hides the differences, so the rest of the code reads the same on both.
 *
 *	Linux / WSL (gcc or clang):
 *		gcc -Wall -o readerWriter readerWriter.c -pthread
 *		./readerWriter
 *
 *	Windows, MinGW or MSYS2 gcc:
 *		gcc -Wall -o readerWriter.exe readerWriter.c
 *		readerWriter.exe
 *
 *	Windows, Visual Studio Developer Command Prompt:
 *		cl /W3 readerWriter.c
 *		readerWriter.exe
 *
 *	(macOS is not supported: it does not implement unnamed POSIX semaphores.)
 *
 * ---------------------------------------------------------------------------------
 * EXPECTED OUTPUT
 * ---------------------------------------------------------------------------------
 *	Time 0.0: Reader 0 reading
 *	Time 1.0: Reader 1 reading
 *	Time 3.0: Reader 3 reading
 *	Time 4.0: Reader 4 reading
 *	Time 6.0: Reader 6 reading
 *	Time 8.5: Writer 2 writing
 *	Time 11.0: Writer 5 writing
 *
 * Writer 2 arrives at time 2.0 but does not get in until 8.5.  New readers keep
 * joining the readers already inside, so the writer waits until every reader is
 * gone.  This is WRITER STARVATION, and it is the same behavior you will see in the
 * Greedy Crossing policy of Project 4.
 *
 * Note: when two writers are both waiting, semaphores do not promise which one
 * wakes first.  On most systems Writer 2 goes before Writer 5, but the order of
 * those two lines may differ.
 *
 * Set SHOW_ARRIVALS to 1 (below) to also print when each thread arrives and leaves.
 * That makes the waiting easy to see.
 *
 * ---------------------------------------------------------------------------------
 * FROM READERS-WRITERS TO THE ONE-LANE BRIDGE
 * ---------------------------------------------------------------------------------
 *	- Vehicles heading the SAME direction can share the bridge, just as readers
 *	  share the resource.
 *	- The two directions exclude each other.  Think of each direction as its own
 *	  group of "readers": the first vehicle of a group to get on claims the bridge
 *	  for that direction, and the last one off releases it.
 *	- Readers-writers has no limit on how many readers are inside.  The bridge does:
 *	  never more than 3 vehicles at once.  You must add that bound.
 *	- This framework favors whoever is already inside (Greedy).  For FCFS, a
 *	  vehicle that arrives must not slip past a waiting vehicle from the other
 *	  direction.
 */

/* =============================================================================== */
/*	Portability layer: threads, semaphores, time and sleep for Linux and Windows	*/
/* =============================================================================== */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L		/* needed for clock_gettime() and nanosleep()	*/
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32

#include <windows.h>

typedef HANDLE semaphore_t;
typedef HANDLE thread_t;

#else

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

typedef sem_t semaphore_t;
typedef pthread_t thread_t;

#endif

/*
 * Fatal
 * Print an error message and stop the program.
 */
static void Fatal(const char *pMessage)
{
	fprintf(stderr, "Error: %s\n", pMessage);
	exit(1);
}

/*
 * Sem_Init
 * Create a semaphore with the given starting value.
 */
static void Sem_Init(semaphore_t *pSem, int value)
{
#ifdef _WIN32
	*pSem = CreateSemaphore(NULL, value, 0x7FFFFFFF, NULL);
	if (*pSem == NULL) {
		Fatal("CreateSemaphore failed");
	}
#else
	int result = sem_init(pSem, 0, (unsigned int) value);
	if (result != 0) {
		Fatal("sem_init failed");
	}
#endif
}

/*
 * Sem_Wait
 * Decrement the semaphore.  Block while its value is zero.
 */
static void Sem_Wait(semaphore_t *pSem)
{
#ifdef _WIN32
	WaitForSingleObject(*pSem, INFINITE);
#else
	sem_wait(pSem);
#endif
}

/*
 * Sem_Post
 * Increment the semaphore, waking one waiting thread if there is one.
 */
static void Sem_Post(semaphore_t *pSem)
{
#ifdef _WIN32
	ReleaseSemaphore(*pSem, 1, NULL);
#else
	sem_post(pSem);
#endif
}

/*
 * Sem_Destroy
 * Release a semaphore's resources once no thread is using it.
 */
static void Sem_Destroy(semaphore_t *pSem)
{
#ifdef _WIN32
	CloseHandle(*pSem);
#else
	sem_destroy(pSem);
#endif
}

#ifdef _WIN32
/*
 * On Windows a thread function has a different signature than on Linux.  This
 * small wrapper lets every thread in this file be written the POSIX way:
 *		void *ThreadFunction(void *arg)
 */
typedef struct _thread_start {
	void	*(*pFunc)(void *);		/* the thread function to run					*/
	void	*pArg;					/* the argument to pass to it					*/
} thread_start;

/*
 * ThreadTrampoline
 * Windows entry point.  Unpacks the function and argument, then calls the function.
 */
static DWORD WINAPI ThreadTrampoline(LPVOID pParam)
{
	thread_start *pStart = (thread_start *) pParam;
	void *(*pFunc)(void *) = pStart->pFunc;
	void *pArg = pStart->pArg;

	free(pStart);
	pFunc(pArg);
	return 0;
}
#endif

/*
 * Thread_Create
 * Start a new thread running pFunc(pArg).
 */
static void Thread_Create(thread_t *pThread, void *(*pFunc)(void *), void *pArg)
{
#ifdef _WIN32
	thread_start *pStart = (thread_start *) malloc(sizeof(thread_start));
	if (pStart == NULL) {
		Fatal("out of memory");
	}
	pStart->pFunc = pFunc;
	pStart->pArg = pArg;

	*pThread = CreateThread(NULL, 0, ThreadTrampoline, pStart, 0, NULL);
	if (*pThread == NULL) {
		Fatal("CreateThread failed");
	}
#else
	int result = pthread_create(pThread, NULL, pFunc, pArg);
	if (result != 0) {
		Fatal("pthread_create failed");
	}
#endif
}

/*
 * Thread_Join
 * Wait for a thread to finish.
 */
static void Thread_Join(thread_t *pThread)
{
#ifdef _WIN32
	WaitForSingleObject(*pThread, INFINITE);
	CloseHandle(*pThread);
#else
	pthread_join(*pThread, NULL);
#endif
}

/*
 * GetTime
 * Return the current time in seconds as a double.  Only differences between two
 * calls are meaningful.  Same idea as GetTime() in OSTEP's common.h.
 */
static double GetTime(void)
{
#ifdef _WIN32
	LARGE_INTEGER frequency;
	LARGE_INTEGER counter;

	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&counter);
	return ((double) counter.QuadPart) / ((double) frequency.QuadPart);
#else
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return ((double) now.tv_sec) + (((double) now.tv_nsec) / 1e9);
#endif
}

/*
 * SleepSeconds
 * Put the calling thread to sleep (not spinning) for the given number of seconds.
 * Used by main() to space out arrivals, like usleep() in the project handout.
 */
static void SleepSeconds(double seconds)
{
#ifdef _WIN32
	DWORD milliseconds = (DWORD) (seconds * 1000.0);
	Sleep(milliseconds);
#else
	struct timespec delay;

	delay.tv_sec = (time_t) seconds;
	delay.tv_nsec = (long) ((seconds - ((double) delay.tv_sec)) * 1e9);
	nanosleep(&delay, NULL);
#endif
}

/*
 * Spin
 * Busy-wait for the given number of seconds.  Same idea as Spin() in OSTEP's
 * common.h.  It keeps the CPU busy, the way real work would.
 */
static void Spin(double seconds)
{
	double startTime = GetTime();
	double elapsed = 0.0;

	while (elapsed < seconds) {
		double now = GetTime();
		elapsed = now - startTime;
	}
}

/* =============================================================================== */
/*	Readers-writers program: everything below is the same on Linux and Windows	*/
/* =============================================================================== */

#define SHOW_ARRIVALS	0		/* 1 = also print when each thread arrives and leaves	*/

#define READER			0
#define WRITER			1

#define USE_TIME		2.5		/* seconds a reader or writer holds the resource		*/

/*
 * Reader-writer lock (OSTEP Figure 31.13).
 * The group of readers inside holds writeLock together.  The first reader in takes
 * it and the last reader out gives it back.  A writer takes it alone.
 */
typedef struct _rwlock_t {
	semaphore_t	lock;			/* protects the readers count					*/
	semaphore_t	writeLock;		/* held by one writer OR by the group of readers	*/
	int			readers;		/* number of readers currently inside			*/
} rwlock_t;

/*
 * Information passed to each thread.  Modeled on vehicle_info from Project 4.
 */
typedef struct _worker_info {
	int		id;					/* thread ID									*/
	int		role;				/* READER or WRITER								*/
	double	inter_arrival_t;	/* time to wait before the next thread arrives	*/
} worker_info;

static rwlock_t gRwLock;		/* the lock shared by all threads				*/
static double gStartTime;		/* time the simulation started					*/

/*
 * Rwlock_Init
 * Set up the lock: no readers inside, and the resource free.
 */
static void Rwlock_Init(rwlock_t *pRw)
{
	pRw->readers = 0;
	Sem_Init(&pRw->lock, 1);
	Sem_Init(&pRw->writeLock, 1);
}

/*
 * Rwlock_Destroy
 * Release the lock's semaphores.
 */
static void Rwlock_Destroy(rwlock_t *pRw)
{
	Sem_Destroy(&pRw->lock);
	Sem_Destroy(&pRw->writeLock);
}

/*
 * Rwlock_AcquireReadLock
 * Enter as a reader.  Only the first reader of a group waits for writeLock.
 */
static void Rwlock_AcquireReadLock(rwlock_t *pRw)
{
	Sem_Wait(&pRw->lock);
	pRw->readers++;

	/* First reader in claims the resource for the whole group of readers */
	if (pRw->readers == 1) {
		Sem_Wait(&pRw->writeLock);
	}
	Sem_Post(&pRw->lock);
}

/*
 * Rwlock_ReleaseReadLock
 * Leave as a reader.  The last reader out lets writers in.
 */
static void Rwlock_ReleaseReadLock(rwlock_t *pRw)
{
	Sem_Wait(&pRw->lock);
	pRw->readers--;

	/* Last reader out hands the resource back */
	if (pRw->readers == 0) {
		Sem_Post(&pRw->writeLock);
	}
	Sem_Post(&pRw->lock);
}

/*
 * Rwlock_AcquireWriteLock
 * Enter as a writer.  Waits until no reader or writer is inside.
 */
static void Rwlock_AcquireWriteLock(rwlock_t *pRw)
{
	Sem_Wait(&pRw->writeLock);
}

/*
 * Rwlock_ReleaseWriteLock
 * Leave as a writer.
 */
static void Rwlock_ReleaseWriteLock(rwlock_t *pRw)
{
	Sem_Post(&pRw->writeLock);
}

/*
 * RoleName
 * Return "Reader" or "Writer" for printing.
 */
static const char *RoleName(int role)
{
	if (role == READER) {
		return "Reader";
	}
	return "Writer";
}

/*
 * ElapsedTime
 * Return the seconds since the simulation started.
 */
static double ElapsedTime(void)
{
	double now = GetTime();
	return now - gStartTime;
}

/*
 * ArriveLock
 * Block until it is safe to use the resource.  (Project 4: ArriveBridge)
 */
static void ArriveLock(worker_info *pInfo)
{
#if SHOW_ARRIVALS
	double arriveTime = ElapsedTime();
	const char *pName = RoleName(pInfo->role);
	printf("Time %.1f: %s %d arrives\n", arriveTime, pName, pInfo->id);
	fflush(stdout);
#endif

	/* Readers share the lock; a writer must have it alone */
	if (pInfo->role == READER) {
		Rwlock_AcquireReadLock(&gRwLock);
	} else {
		Rwlock_AcquireWriteLock(&gRwLock);
	}
}

/*
 * UseResource
 * Print the start time, then simulate the work.  (Project 4: CrossBridge)
 */
static void UseResource(worker_info *pInfo)
{
	double startTime = ElapsedTime();
	const char *pName = RoleName(pInfo->role);
	const char *pAction = "writing";

	if (pInfo->role == READER) {
		pAction = "reading";
	}

	/* Report when this thread begins using the resource */
	printf("Time %.1f: %s %d %s\n", startTime, pName, pInfo->id, pAction);
	fflush(stdout);

	/* Simulate the time spent reading or writing */
	Spin(USE_TIME);
}

/*
 * LeaveLock
 * Update the shared state so waiting threads can proceed.  (Project 4: ExitBridge)
 */
static void LeaveLock(worker_info *pInfo)
{
	/* Give back the kind of lock this thread took in ArriveLock() */
	if (pInfo->role == READER) {
		Rwlock_ReleaseReadLock(&gRwLock);
	} else {
		Rwlock_ReleaseWriteLock(&gRwLock);
	}

#if SHOW_ARRIVALS
	double leaveTime = ElapsedTime();
	const char *pName = RoleName(pInfo->role);
	printf("Time %.1f: %s %d leaves\n", leaveTime, pName, pInfo->id);
	fflush(stdout);
#endif
}

/*
 * WorkerAction
 * Body of every thread.  (Project 4: VehicleAction)
 */
static void *WorkerAction(void *arg)
{
	worker_info *pInfo = (worker_info *) arg;

	ArriveLock(pInfo);
	UseResource(pInfo);
	LeaveLock(pInfo);
	return NULL;
}

/*
 * main
 * Create one thread per reader or writer using the arrival schedule, then wait for
 * all of them to finish.
 */
int main(void)
{
	/* Arrival schedule: id, role, and wait before the next arrival (seconds) */
	worker_info workers[] = {
		{ 0, READER, 1.0 },		/* arrives at 0.0	*/
		{ 1, READER, 1.0 },		/* arrives at 1.0	*/
		{ 2, WRITER, 1.0 },		/* arrives at 2.0	*/
		{ 3, READER, 1.0 },		/* arrives at 3.0	*/
		{ 4, READER, 1.0 },		/* arrives at 4.0	*/
		{ 5, WRITER, 1.0 },		/* arrives at 5.0	*/
		{ 6, READER, 0.0 }		/* arrives at 6.0	*/
	};
	int numWorkers = (int) (sizeof(workers) / sizeof(workers[0]));
	thread_t threads[sizeof(workers) / sizeof(workers[0])];
	int i;

	Rwlock_Init(&gRwLock);
	gStartTime = GetTime();

	/* Create the threads, sleeping between arrivals */
	for (i = 0; i < numWorkers; i++) {
		Thread_Create(&threads[i], WorkerAction, &workers[i]);
		SleepSeconds(workers[i].inter_arrival_t);
	}

	/* Wait for every thread to finish */
	for (i = 0; i < numWorkers; i++) {
		Thread_Join(&threads[i]);
	}

	Rwlock_Destroy(&gRwLock);
	return 0;
}
