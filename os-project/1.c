#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <unistd.h> // For sleep()

sem_t rw_mutex;
pthread_mutex_t r_mutex;
int readerCount = 0;
struct readerInfo
{
    pthread_t tid;
    char readerName[100];
    char** booknames;
    int totalbooks;
};
struct bookInfo
{
    // meta data 
    char bookName[100];
    char bookAuthor[100];
    int bookPages;
};
void *read_func(void *args)
{
    struct readerInfo *ri = (struct readerInfo *)args;
    char **booksArray = ri->booknames;
    int totalbooks = ri->totalbooks;
    pthread_mutex_lock(&r_mutex);
    readerCount++;
    if(readerCount == 1){
        printf("Reader %s is imposing lock on writer in func: %lu.... \n", ri->readerName, ri->tid);

        sem_wait(&rw_mutex);
    }
    pthread_mutex_unlock(&r_mutex);
    // struct readerInfo *ri = (struct readerInfo*)args;
    // char** booksArray = ri->booknames;
    // int totalbooks = ri->totalbooks;
    
    for (int i = 0; i < totalbooks; i++)
    {
        printf("Reader %s reading in func: %lu.... \n", ri->readerName, ri->tid);
        printf("%s\n", booksArray[i]);
        sleep(3); // entire system sleep or just this thread? - just this
        // in posix only the calling thread sleeps
    }
    // read completed ke baad decrease bhi phir aik hi karay
    // during that time all readers paused - ig
    pthread_mutex_lock(&r_mutex);
    printf("Reader %s is imposing lock on other readers while modifying read count in func: %lu.... \n", ri->readerName, ri->tid);
    readerCount--;
    if (readerCount == 0)
    {
        printf("Reader %s is lifting the lock on writer, as no more readers left in func: %lu.... \n", ri->readerName, ri->tid);

        sem_post(&rw_mutex);
    }
    pthread_mutex_unlock(&r_mutex);
    
    
}
void *write_func(void *args)
{
    sem_wait(&rw_mutex);

    FILE *fp = fopen("Books.txt","a");
    if(!fp){
        printf("Error opening the database for writing");
        return NULL; // null allowed
    }
    sleep(3);
    char bookName[100];
    printf("Enter book name: ");
    scanf("%s", bookName);
    char authorName[100];
    printf("Enter author name: ");
    scanf("%s", authorName);
    int pages;
    printf("Enter book pages: ");
    scanf("%d", &pages);
    fprintf(fp,"%s %s %d", bookName, authorName, pages);
    // so issue because of writing input and overall input so, as soon as write is chosed lock must be put on reading and vice versa
    fclose(fp);
    sem_post(&rw_mutex);
}

int main()
{
    // initialize mutex here
    pthread_mutex_init(&r_mutex, NULL); // timer in null
    sem_init(&rw_mutex,0,1); // 0 means it is shared between threads od same process, 1 is just default value
    while (1)
    {
        int choice;
        printf("1. Read\n");
        printf("2. Write\n");
        printf("3. Terminate session\n");
        printf("3. Inputs are open for read write or terminate as the reading goes on in the bg\n");
        scanf("%d", &choice);
        switch(choice){
            case 1:{
                printf("Reading\n");
                // printf("...\n");
                pthread_t t;
                struct readerInfo RI;
                RI.tid = t;
                char** booksArray;
                int numbooks = 0;
                char readerName[100];
                printf("Enter your name: ");
                scanf("%s", readerName);
                // RI.readerName = readerName; // this wrong In C, you cannot assign to an array like that — arrays are not assignable.
                strcpy(RI.readerName, readerName);
                while(1){
                    char* bookname = (char*)malloc(100*sizeof(char));
                    printf("Enter the book name u plan on accessing(e to exit): ");
                    scanf("%s", bookname);
                    if(strcmp(bookname,"e") == 0){
                        free(bookname);
                        break;
                    }
                    numbooks++;
                    // 0 -> 1
                    char **tempbooks = (char **)realloc(booksArray, (numbooks) * sizeof(char *));

                    if(!tempbooks){
                        printf("Memory allocation for books failed\n");
                        free(bookname);
                        break;
                    }
                    booksArray = tempbooks; // assign - rename
                    booksArray[numbooks - 1] = bookname;

                    // so no need to deallocate temp as it only added an extra block and gave to books, rather than an entire temp array
                    // deallocate books array when done, reallocate didnt do it in 2 steps
                }
                RI.booknames = booksArray;
                RI.totalbooks = numbooks;
                pthread_create(&t, NULL, read_func, (void*)&RI);
                
                break;
            }
            case 2:{
                
                printf("Writing\n");
                pthread_t t; // redeclared in a different case
                pthread_create(&t, NULL, write_func,NULL);
                pthread_join(t, NULL);
                break;
            }
            case 3:
                printf("Terminating\n");
                exit(0);
            default:
                printf("Incorrect inputs\n");
                exit(0);
            }
    }
    pthread_mutex_destroy(&r_mutex);
    sem_destroy(&rw_mutex);
    // mechanism for main to end without terminating others
}