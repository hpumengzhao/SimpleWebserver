#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <semaphore.h>
//互斥锁
class locker{
public:
    locker();
    ~locker();
    bool lock();
    bool unlock();
    pthread_mutex_t *get();
private:
    pthread_mutex_t m_mutex;
};
//条件变量
class cond{
public:
    cond();
    ~cond();
    bool wait(pthread_mutex_t * mutex);
    bool timewait(pthread_mutex_t *mutex, struct timespec *t);
    bool signal();
    bool broadcast();
private:
    pthread_cond_t m_cond;
};
//信号量
class sem{
public:
    sem();
    sem(int num);
    ~sem();
    bool wait();
    bool post();
private:
    sem_t m_sem;
};
#endif