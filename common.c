#include "common.h"

void Fatal(const char *pMessage)
{
    fprintf(stderr, "Error: %s\n", pMessage);
    exit(1);
}

#ifdef _WIN32
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
 * Sem_Init
 * Create a semaphore with the given starting value.
 */
void Sem_Init(semaphore_t *pSem, int value)
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
 * Decrement the semaphore. Block while its value is zero.
 */
void Sem_Wait(semaphore_t *pSem)
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
void Sem_Post(semaphore_t *pSem)
{
#ifdef _WIN32
    ReleaseSemaphore(*pSem, 1, NULL);
#else
    sem_post(pSem);
#endif
}

/*
 * Sem_Destroy
 * Release the semaphore's resources once no thread is using it.
 */
void Sem_Destroy(semaphore_t *pSem)
{
#ifdef _WIN32
    CloseHandle(*pSem);
#else
    sem_destroy(pSem);
#endif
}

/*
 * Thread_Create
 * Start a new thread running pFunc(pArg).
 */
void Thread_Create(thread_t *pThread, void *(*pFunc)(void *), void *pArg)
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
void Thread_Join(thread_t *pThread)
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
double GetTime(void)
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
void SleepSeconds(double seconds)
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
void Spin(double seconds)
{
    double startTime = GetTime();
    double elapsed = 0.0;

    while (elapsed < seconds) {
        double now = GetTime();
        elapsed = now - startTime;
    }
}

double gStartTime = 0.0;

/*
 * ElapsedTime
 * Return the seconds since the simulation started.
 */
double ElapsedTime(void)
{
    double now = GetTime();
    return now - gStartTime;
}