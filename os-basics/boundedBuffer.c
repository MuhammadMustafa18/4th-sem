#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define buffersize 10
int buffer[buffersize];
int in = 0, out = 0;
int counter = 0;
sem_t mutex, empty, full; // why global
// so that threads can access - in main, they limited to main scope
void *producer(void *args)
{
    while (1)
    {
        sleep(2);
        sem_wait(&empty); // checks if empty > 0 if yes then produces
        sem_wait(&mutex); // checks if available, if yes then --mutex and locks
        int curr = in;
        buffer[in] = ++counter;
        in = (in + 1) % buffersize;
        printf("Added %d at buffer[%d]\n", counter, curr); // why dont these get re ordered by cpu?
        // how does the cpu see in advance that what reordering better?
        sem_post(&full);  // adds one to full to show reader
        sem_post(&mutex); // makes sure one at a time enters
    }
}
void *consumer(void *args)
{
    // First check if you are allowed to produce / consume.Only then take mutex and touch buffer.
    // WONT MATTER A LOT - since consumer cant lock producer out when it iself is zero by default
    // but ❌ can waste time or cause deadlock later
    // why
    // suppose buffer is full, producer cant produce
    // and it somehow got the main lock mutex
    // it cant produce and wont let consumer consume
    // deadlock example
    while (1)
    {
        sleep(2);
        
        sem_wait(&full);  // checks if full > 0 if yes then consumes
        sem_wait(&mutex); // checks if available, if yes then --mutex and locks
        int curr = 0;
        int val = buffer[out];
        out = (out + 1) % buffersize;
        printf("read %d at buffer[%d]\n", val, curr);
        // why dont these get re ordered by cpu?
        // Deeper note : Also, sem_wait and sem_post act like memory barriers — they also prevent reordering across them.
        
        // how does the cpu see in advance that what reordering better?
        // Short answer: Modern CPUs can reorder instructions for optimization.
        // But printf() is a system call → it forces ordering because it interacts with the operating system (IO barrier).

        // not deadlock, just efficiency
        sem_post(&empty); // adds one to empty to show writer
        sem_post(&mutex); // makes sure one at a time enters
    }
}
int main()
{
    sem_init(&mutex, 0, 1);
    // 1 means unlocked and 0 means locked
    sem_init(&empty, 0, buffersize);
    // counts how many empty, how many producer can produce
    sem_init(&full, 0, 0); // when producer sets to 1, only then consumer can consume - but isnt that the condition for empty?
    // counts how many full, how many consumer can consume
    // so producer waits on full - how do u define full? x when empty = 0
    // 0 pe wait
    // so producer keeps producing till empty = 0
    // consumer keeps on consuming on full > 0

    pthread_t producerT, consumerT;
    pthread_create(&producerT, NULL, producer, NULL);
    pthread_create(&consumerT, NULL, consumer, NULL);
    pthread_join(producerT, NULL);
    pthread_join(consumerT, NULL);

    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
}