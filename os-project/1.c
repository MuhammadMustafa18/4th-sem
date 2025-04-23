#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <unistd.h> // For sleep()
#include <time.h>

sem_t ww_mutex;
pthread_mutex_t rw_mutex;
sem_t r_mutex;
int readerCount = 0;
struct readerInfo
{
    pthread_t tid;
    char readerName[100];
    char** booknames;
    int totalbooksborrowed;
};
struct bookInfo
{
    // meta data 
    char bookName[100];
    char bookAuthor[100];
    int bookPages;
    char* borrowed;
};

void *read_func(void *args)
{
    pthread_mutex_lock(&rw_mutex); 
    // kisi aik ne bhi lock kardiya to write nahi hoga, but again agar kisi aik ne bhi unlock krdiya to hojayega, isliye we need counts
    int maxWait = 10;
    int actualWait = rand() % maxWait + 1;
    // before the request is catered, each reader has to wait
    sleep(actualWait);

    struct readerInfo *ri = (struct readerInfo *)args;
    char **booksArray = ri->booknames;
    int totalbooks = ri->totalbooksborrowed;

    for (int i = 0; i < totalbooks; i++)
    {
        printf("NOTIFICATION:Reader %s requested the borrow of: %s.... \n", ri->readerName, booksArray[i]);
        // check if the book is available
        sem_wait(&r_mutex); // file is shared between readers, can allow concurrent access as multiple readers cannot borrow the same book
        FILE *fr = fopen("Books.txt","r");
        FILE *temp = fopen("Temp.txt","w");
        if(!fr){
            perror("Error opening file for read\n");
            break;
        }
        char line[256];
        int foundFlag = 0;
        while(fgets(line, sizeof(line), fr)){
            // fgets remembers where it left off
            line[strcspn(line, "\n")] = '\0'; 
            char* bookname = strtok(line, " "); // starts at start of line and cuts at the delimiter
            char* authorname = strtok(NULL, " "); // null tells it to remember where it left off, continues from there
            char *pages = strtok(NULL, " ");   
            char *borrowed = strtok(NULL, " ");
            if (strcmp(bookname, ri->booknames[i]) == 0 && (strcmp(borrowed, "NB") == 0 || strcmp(borrowed, "nb") == 0))
            {
                // found
                foundFlag = 1;
                printf("NOTIFICATION: Reader %s borrowed the book: %s\n", ri->readerName, booksArray[i]);
                fprintf(temp, "%s %s %s B\n", bookname, authorname, pages, borrowed);
                break; // inner loop
            }
            else
            {
                fprintf(temp, "%s %s %s %s\n", bookname, authorname, pages, borrowed);
            }
        }
        if(!foundFlag){
            printf("The book: %s is either borrowed or not available\n", booksArray[i]);
        }
        fclose(fr);
        fclose(temp);
        remove("Books.txt");
        rename("Temp.txt", "Books.txt");
        sem_post(&r_mutex);
    }
    // read completed ke baad decrease bhi phir aik hi karay
    // during that time all readers paused - ig

    pthread_mutex_unlock(&rw_mutex);
}
void *write_func(void *args)
{
    sem_wait(&ww_mutex);
    pthread_mutex_lock(&rw_mutex);
    
    sleep(3);
    struct bookInfo *BI = (struct bookInfo *)args;
    FILE *fp = fopen("Books.txt","a");
    if(!fp){
        printf("Error opening the database for writing");
        return NULL; // null allowed
    }
    
    
    fprintf(fp,"%s %s %d %s\n", BI->bookName, BI->bookAuthor, BI->bookPages, BI->borrowed);
    printf("NOTIFICATION: Writing the book : %s.... \n", BI->bookName);
    sleep(3);
    // so issue because of writing input and overall input so, as soon as write is chosed lock must be put on reading and vice versa
    fclose(fp);

    pthread_mutex_unlock(&rw_mutex);
    sem_post(&ww_mutex);
}

int main()
{
    // initialize mutex here
    srand(time(0)); // seed the random nmber generator on current time
    pthread_mutex_init(&rw_mutex, NULL); // used for r-w conflicts
    
    sem_init(&r_mutex, 0, 1); // used for within r-r conflicts
    sem_init(&ww_mutex, 0, 1); // used for w-w conflicts
    while (1)
    {
        int choice;
        printf("LIBRARY DASHBOARD\n");
        printf("1. Reader request\n");
        printf("2. Add a book\n");
        printf("3. Terminate session\n");
        printf("Choice 1/2/3?: \n");
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
                    printf("Enter the book name you want to borrow (e to exit): ");
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
                RI.totalbooksborrowed = numbooks;
                pthread_create(&t, NULL, read_func, (void*)&RI);
                
                break;
            }
            case 2:{
                
                printf("Writing\n");
                char bookName[100];
                char authorName[100];
                int pages;
                printf("Enter book name, author name and pages (space seperated): ");
                scanf("%s %s %d", bookName, authorName, &pages);
                // printf("Enter author name: ");
                // scanf("%s", authorName);
                // printf("Enter book pages: ");
                // scanf("%d", &pages);
                
                struct bookInfo BI;
                strcpy(BI.bookName, bookName);
                strcpy(BI.bookAuthor, authorName);
                BI.bookPages = pages;
                strcpy(BI.borrowed,"NB");

                pthread_t t; // redeclared in a different case

                pthread_create(&t, NULL, write_func,(void*)&BI);
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
    pthread_mutex_destroy(&rw_mutex);
    sem_destroy(&r_mutex);
    sem_destroy(&ww_mutex);
    // mechanism for main to end without terminating others
}