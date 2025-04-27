#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

sem_t RRmutex;
sem_t WRmutex;
sem_t WWmutex;
sem_t readTry;

int counter = 0;
int WritersWaiting = 0;

void *reader(void *args)
{
    // while(1) to simulate constantly ye kaam horaha hai
    while (1)
    {
        sem_wait(&readTry); // what if this locks the writerwaiting out can't - why not?
        sem_wait(&RRmutex);
        counter++; // critical among readers
        sem_post(&RRmutex);
        if (counter == 1)
        {
            sem_wait(&WRmutex);
        }
        sem_post(&readTry); // brief
        // start
        sleep(2);
        printf("reader is reading\n");
        // done
        sem_wait(&RRmutex);
        counter--; // critical among readers
        sem_post(&RRmutex);

        if (counter == 0)
        {
            sem_post(&WRmutex);
        }
    }
}
void *writer(void *args)
{
    // while(1) to simulate constantly ye kaam horaha hai
    while (1)
    {
        sem_wait(&WWmutex);
        WritersWaiting++; // critical among readers
        sem_post(&WWmutex);
        if(WritersWaiting){
            // so that readtry cant aquire the lock - but it will if it writer setting to unlock
            sem_wait(&readTry);
        }
        sem_wait(&WRmutex);

        // start
        sleep(2);
        printf("writer is writing\n");
        // done - after being done or after done waiting
        sem_wait(&WWmutex);
        WritersWaiting--; // critical among readers
        sem_post(&WWmutex);
        sem_post(&WRmutex);
        // allow new readers only when writers done
        if (!WritersWaiting)
        {
            sem_post(&readTry);
        }
    }
}
int main()
{
    // (This is called First Readers - Writers Problem — Reader - biased.)
    sem_init(&RRmutex, 0, 1); // unlocked jayega
    sem_init(&WRmutex, 0, 1); // unlocked jayega
    sem_init(&WWmutex, 0, 1); // unlocked jayega
    sem_init(&readTry, 0, 1); // unlocked jayega
    pthread_t readerT, writerT;
    pthread_create(&readerT, NULL, reader, NULL);
    pthread_create(&writerT, NULL, writer, NULL);
    pthread_join(readerT, NULL); // jab tak reader doesnt return it wont finish
    pthread_join(writerT, NULL);
    sem_destroy(&RRmutex);
    sem_destroy(&WRmutex);
    sem_destroy(&WWmutex);
    sem_destroy(&readTry);
}