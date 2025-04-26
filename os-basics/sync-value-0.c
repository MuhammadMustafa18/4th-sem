#include<stdio.h>
#include<pthread.h>
#include <semaphore.h>
#include <stdlib.h>

void* waiter(void args){
    sem_wait(&sem1); // ye acquire
    // sleep(3);
    printf("YO in waiter\n");
    // sem_post(&sem1);
}
void *signaler(void args)
{
    // sem_wait(&sem1);
    sleep(1);
    printf("YO in signaler\n");
    sem_post(&sem1); // ye release krega
}
int main(){
    // goal sem value init pe zero
    sem_init(&sem1,0,0); // 
    pthread_t t1,t2;
    pthread_create(&t1, NULL, waiter, NULL);
    pthread_create(&t1, NULL, signaler , NULL);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);

}