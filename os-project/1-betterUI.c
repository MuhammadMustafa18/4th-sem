#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

// Synchronization primitives
sem_t ww_mutex;
pthread_mutex_t rw_mutex;
sem_t r_mutex;
sem_t rr_mutex;
int readerCount = 0;

// Data structures
typedef struct
{
    pthread_t tid;
    char readerName[100];
    char **booknames;
    int totalbooksborrowed;
} ReaderInfo;

typedef struct
{
    char bookName[100];
    char bookAuthor[100];
    int bookPages;
    char borrowed[3]; // "NB" or "B"
} BookInfo;

void *read_func(void *args);
void *write_func(void *args);
void cleanup_reader(ReaderInfo *ri);
void welcomeScreen()
{
    printf("\033[1;31m __          ________ _      _____ ____  __  __ ______ \n");
    printf("\033[1;32m \\ \\        / /  ____| |    / ____/ __ \\|  \\/  |  ____|\n");
    printf("\033[1;33m  \\ \\  /\\  / /| |__  | |   | |   | |  | | \\  / | |__   \n");
    printf("\033[1;34m   \\ \\/  \\/ / |  __| | |   | |   | |  | | |\\/| |  __|  \n");
    printf("\033[1;35m    \\  /\\  /  | |____| |___| |___| |__| | |  | | |____ \n");
    printf("\033[1;36m     \\/  \\/   |______|______\\_____\\____/|_|  |_|______|\n");
    printf("Loading...\n");
    printf("\033[1;35m[");
    for (int i = 0; i < 15; i++)
    {
        printf("*");
        fflush(stdout);
        sleep(1);
    }
    printf("]");
}

int main()
{
    srand(time(0));
    pthread_mutex_init(&rw_mutex, NULL);
    sem_init(&rr_mutex, 0, 1);
    sem_init(&r_mutex, 0, 1);
    sem_init(&ww_mutex, 0, 1);
    welcomeScreen();
    while (1)
    {
        // system("cls");
        // system("color F4");
        printf("\n========= LIBRARY MENU =========\n");
        printf("1. Borrow books as a reader\n");
        printf("2. Add a new book to library\n");
        printf("3. Exit the system\n");
        printf("================================\n");
        printf("Please enter your choice (1-3): ");

        int choice;
        char input[100];
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            perror("Input error");
            continue;
        }

        if (sscanf(input, "%d", &choice) != 1)
        {

            printf("Invalid input. Please enter 1, 2, or 3.\n");
            continue;
        }

        switch (choice)
        {
        case 1:
        {
            ReaderInfo *ri = malloc(sizeof(ReaderInfo));
            // pointer to pointer 4-8 bytes(booknames)
            if (!ri)
            {
                perror("Failed to allocate reader info");
                break;
            }

            printf("Enter your name: ");
            if (fgets(ri->readerName, sizeof(ri->readerName), stdin) == NULL)
            {
                // any error while reading = null
                free(ri);
                break;
            }
            ri->readerName[strcspn(ri->readerName, "\n")] = '\0'; // Remove newline

            ri->booknames = NULL;
            ri->totalbooksborrowed = 0;

            while (1)
            {
                printf("Enter the book name you want to borrow (e to exit): ");
                char bookname[100];
                if (fgets(bookname, sizeof(bookname), stdin) == NULL)
                {
                    break;
                }
                bookname[strcspn(bookname, "\n")] = '\0';

                if (strcmp(bookname, "e") == 0)
                {
                    break;
                }
                // yahan realloc
                char **temp = realloc(ri->booknames, (ri->totalbooksborrowed + 1) * sizeof(char *));
                if (!temp)
                {
                    perror("Failed to allocate book list");
                    cleanup_reader(ri);
                    break;
                }
                // yahan actual assign to that new block
                ri->booknames = temp;
                ri->booknames[ri->totalbooksborrowed] = strdup(bookname);
                // ri->booknames[ri->totalbooksborrowed] = bookname;
                // where bookname is the dynamic string, and that also a dynamically allocated - so what issue?
                // each element holds pointer to char*
                // char* points to a string in memory, = pe pointer is copied, so 2 pointers same memory
                // dup solves this
                if (!ri->booknames[ri->totalbooksborrowed])
                {
                    perror("Failed to duplicate book name");
                    cleanup_reader(ri);
                    break;
                }
                ri->totalbooksborrowed++;
            }

            if (ri->totalbooksborrowed > 0)
            {
                pthread_t tid;
                if (pthread_create(&tid, NULL, read_func, ri) != 0)
                {
                    perror("Failed to create reader thread");
                    cleanup_reader(ri);
                }
                else
                {
                    // Detach thread so when it is done - memory is cleaned
                    // when we create, os keeps the resources allocated till joined
                    // if not joined then zombie thread - never freed
                    pthread_detach(tid);
                }
            }
            else
            {
                free(ri);
            }
            break;
        }
        case 2:
        {
            BookInfo bi; // no dynamic inner structure - but only the array reallocates right?
            // basically (stack) is shared between threads - here this bi is in stack rn and if this goes to the new thread in stack and if main ends before, since it is in the main's stack then probelms

            // but luckily for us this doesnt get passed - it does actually but because of join main wont end until it done hence no issues
            // we could've done detach, heap copy banegi(manually using malloc) of data in stack
            printf("Enter book name, author name and pages (space separated): ");
            char input[256];
            if (fgets(input, sizeof(input), stdin) == NULL)
            {
                printf("Input error\n");
                break;
            }

            if (sscanf(input, "%99s %99s %d", bi.bookName, bi.bookAuthor, &bi.bookPages) != 3)
            {
                printf("Invalid input format\n");
                break;
            }
            strncpy(bi.borrowed, "NB", sizeof(bi.borrowed)); // defines kitne copy krne hain, if source larger it will handle

            pthread_t tid;
            if (pthread_create(&tid, NULL, write_func, &bi) != 0)
            {
                perror("Failed to create writer thread");
            }
            else
            {
                pthread_join(tid, NULL); // Wait for write to complete
            }
            break;
        }
        case 3:
            printf("Terminating...\n");
            pthread_mutex_destroy(&rw_mutex); // have resources allocated so freeing needed
            sem_destroy(&rr_mutex);
            sem_destroy(&r_mutex);
            sem_destroy(&ww_mutex);
            exit(0);
        default:
            printf("Invalid choice. Please enter 1, 2, or 3.\n");
        }
    }
    return 0;
}

void *read_func(void *args)
{
    ReaderInfo *ri = (ReaderInfo *)args;

    // Enter critical section for reader count
    sem_wait(&rr_mutex);
    readerCount++;
    if (readerCount == 1)
    {
        pthread_mutex_lock(&rw_mutex); // First reader locks writers out
    }
    sem_post(&rr_mutex);

    // Simulate processing delay
    sleep(rand() % 10); // 0-100ms

    for (int i = 0; i < ri->totalbooksborrowed; i++)
    {
        sem_wait(&r_mutex); // Protect file access between readers

        FILE *fr = fopen("Books.txt", "r");
        FILE *temp = fopen("Temp.txt", "w");
        if (!fr || !temp)
        {
            perror("Failed to open files");
            sem_post(&r_mutex);
            continue;
        }

        char line[256];
        int foundFlag = 0;
        while (fgets(line, sizeof(line), fr))
        {
            line[strcspn(line, "\n")] = '\0';

            char *tokens[4];
            char *token = strtok(line, " ");
            for (int j = 0; j < 4 && token != NULL; j++)
            {
                tokens[j] = token;
                token = strtok(NULL, " ");
            }

            if (tokens[0] && tokens[3] &&
                strcmp(tokens[0], ri->booknames[i]) == 0 &&
                (strcasecmp(tokens[3], "NB") == 0))
            {

                foundFlag = 1;
                printf("\n[REQUEST] Reader %s requested: %s\n", ri->readerName, ri->booknames[i]);
                printf("[BORROWED] Reader %s successfully borrowed: %s\n", ri->readerName, ri->booknames[i]);
                fprintf(temp, "%s %s %s B\n", tokens[0], tokens[1], tokens[2]);
            }
            else if (tokens[0] && tokens[3])
            {
                fprintf(temp, "%s %s %s %s\n", tokens[0], tokens[1], tokens[2], tokens[3]);
            }
        }

        fclose(fr);
        fclose(temp);

        if (foundFlag)
        {
            remove("Books.txt");
            rename("Temp.txt", "Books.txt");
        }
        else
        {
            remove("Temp.txt");
            printf("[UNAVAILABLE] Book '%s' is either borrowed or does not exist.\n", ri->booknames[i]);
        }

        sem_post(&r_mutex);
    }

    // Exit critical section for reader count
    sem_wait(&rr_mutex);
    readerCount--;
    if (readerCount == 0)
    {
        pthread_mutex_unlock(&rw_mutex); // Last reader releases writers
    }
    sem_post(&rr_mutex);

    cleanup_reader(ri);
    return NULL;
}

void *write_func(void *args)
{
    BookInfo *bi = (BookInfo *)args;

    sem_wait(&ww_mutex);
    pthread_mutex_lock(&rw_mutex);

    // Simulate processing delay
    sleep(rand()%10); // 300ms

    FILE *fp = fopen("Books.txt", "a");
    if (!fp)
    {
        perror("Failed to open books file");
    }
    else
    {
        fprintf(fp, "%s %s %d %s\n", bi->bookName, bi->bookAuthor, bi->bookPages, bi->borrowed);
        printf("\n[ADDED] Book '%s' by %s (%d pages) successfully added to the library.\n",
               bi->bookName, bi->bookAuthor, bi->bookPages);
        fclose(fp);
    }

    pthread_mutex_unlock(&rw_mutex);
    sem_post(&ww_mutex);

    return NULL;
}

void cleanup_reader(ReaderInfo *ri)
{
    if (ri)
    {
        for (int i = 0; i < ri->totalbooksborrowed; i++)
        {
            free(ri->booknames[i]);
        }
        free(ri->booknames);
        free(ri);
    }
}