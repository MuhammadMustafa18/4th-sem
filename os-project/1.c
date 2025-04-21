#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
struct readerInfo
{
    pthread_t tid;
    char readerName[100];
    char** booknames;
    int totalbooks;
};
struct bookInfo
{
    pthread_t tid;
    char bookName[100];
};
void *read_func(void *args)
{
    
    struct readerInfo *ri = (struct readerInfo*)args;
    char** booksArray = ri->booknames;
    int totalbooks = ri->totalbooks;
    printf("Reader %s reading in func: %lu \n", ri->readerName, ri->tid);
    for (int i = 0; i < totalbooks; i++)
    {
        printf("%s\n", booksArray[i]);
    }
    
}
void *write_func(void *args)
{

    
}

int main()
{
    while(1){
        int choice;
        printf("1. Read\n");
        printf("2. Write\n");
        printf("3. Terminate session\n");
        scanf("%d", &choice);
        switch(choice){
            case 1:
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
            case 2:
                printf("Writing\n");
                break;
            case 3:
                printf("Terminating\n");
                exit(0);
            default:
                printf("Incorrect inputs\n");
                exit(0);
            }
    }
    // mechanism for main to end without terminating others
}