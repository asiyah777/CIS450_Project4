#ifndef COMMON_H
#define COMMON_H

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

void Fatal(const char *pMessage);
void Sem_Init(semaphore_t *pSem, int value);
void Sem_Wait(semaphore_t *pSem);
void Sem_Post(semaphore_t *pSem);
void Sem_Destroy(semaphore_t *pSem);

#ifdef _WIN32
typedef struct _thread_start {
    void *(*pFunc)(void *);
    void *pArg;
} thread_start;
#endif

void Thread_Create(thread_t *pThread, void *(*pFunc)(void *), void *pArg);
void Thread_Join(thread_t *pThread);
double GetTime(void);
void SleepSeconds(double seconds);
void Spin(double seconds);
double ElapsedTime(void);

extern double gStartTime;

#endif