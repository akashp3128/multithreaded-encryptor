#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "encrypt-module.h"

sem_t inputLock;
sem_t inputFull;
sem_t inputEmpty;
sem_t inputCount;
sem_t inputCounted;
sem_t outputLock;
sem_t outputFull;
sem_t outputEmpty;
sem_t outputCount;
sem_t outputCounted;

int maxSize;
int size;
struct node_t *head;
struct node_t *tail;
struct buffer_t *inputBuffer;
struct buffer_t *outputBuffer;

// create circular buffer
struct buffer_t *initBuffer(int max)
{
    struct buffer_t *buffer;
    if ((buffer = malloc(sizeof(struct buffer_t))) == 0)
        return NULL;

    struct node_t *head;
    struct node_t *tail;
    if ((head = malloc(sizeof(struct node_t))) == 0 || (tail = malloc(sizeof(struct node_t))) == 0)
    {
        free(buffer);
        if (head)
            free(head);
        return NULL;
    }

    buffer->head = head;
    buffer->tail = tail;
    buffer->head->data = -1;
    buffer->tail->data = -1;
    buffer->head->counted = -1;
    buffer->head->counted = -1;
    buffer->head->next = tail;
    buffer->head->prev = tail;
    buffer->tail->next = head;
    buffer->tail->prev = head;
    buffer->maxSize = max;
    buffer->size = 0;

    return buffer;
}

// allocate new node to buffer

struct node_t *allocNode(char c)
{
    struct node_t *node;
    if ((node = malloc(sizeof(struct node_t))) == 0)
        return NULL;

    node->data = c;
    node->counted = 0;

    return node;
}

struct node_t *unlink(struct buffer_t *b)
{
    struct node_t *toRet = b->head->next;
    b->head->next = toRet->next;
    b->head->next->prev = b->head;
    return toRet;
}

int addNode(struct buffer_t *b, char c)
{
    if (b->size == b->maxSize)
        return 0;

    struct node_t *toAdd;
    if ((toAdd = allocNode(c)) == NULL)
        return 0;

    link(b, toAdd);
    b->size = b->size + 1;

    return 1;
}

char removeNode(struct buffer_t *b)
{
    if (b->size == 0)
        return 0;

    struct node_t *toRet = unlink(b);
    char data = toRet->data;
    free(toRet);
    b->size = b->size - 1;
    return data;
}

void link(struct buffer_t *b, struct node_t *n)
{
    n->next = b->tail;
    n->prev = b->tail->prev;
    b->tail->prev->next = n;
    b->tail->prev = n;
}

void printBuffer(struct buffer_t *b)
{
    if (b->size == 0)
    {
        printf("[]\n");
        return;
    }

    struct node_t *curr = b->head->next;

    printf("[");
    while (curr != b->tail)
    {
        if (curr->next == b->tail)
            printf("%c", curr->data);
        else
            printf("%c, ", curr->data);
        curr = curr->next;
    }
    printf("]\n");
}

int isEmpty(struct buffer_t *b)
{
    if (b->size == 0)
        return 1;
    return 0;
}

int isFull(struct buffer_t *b)
{
    if (b->size == b->maxSize)
        return 1;
    return 0;
}

void freeBuffer(struct buffer_t *b)
{
    struct node_t *curr = b->head->next;
    while (curr != b->head)
    {
        struct node_t *next = curr->next;
        free(curr);
        curr = next;
    }
    free(curr);
    free(b);
}

int resetReq = 0;
pthread_cond_t reset_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t reset_mutex = PTHREAD_MUTEX_INITIALIZER;

// reset started
void reset_requested()
{
    resetReq = 1; // stops reader
    int empty;
    while (1)
    {
        sem_wait(&inputLock);
        empty = isEmpty(inputBuffer);
        sem_post(&inputLock);
        if (empty)
            break;
    }
    log_counts();
    reset_finished();
}

// reset finished
void reset_finished()
{
    resetReq = 0;
    pthread_cond_signal(&reset_cond);
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Incorrect number of command line arguments\n");
        return 0;
    }

    char *input = argv[1];
    char *output = argv[2];
    char *log = argv[3];

    init(input, output, log);
    int n = 0;
    int valid = 0;
    int m = 0;

    // prompt user for input buffer size
    while (!valid)
    {
        printf("Input the Buffer Size ");
        scanf("%d", &n);
        if (n <= 1)
            printf("Invalid size.\n");
        else
            valid = 1;
    }
    valid = 0;
    while (!valid)
    {
        printf("Output the Buffer Size "); // prompt user for output buffer size
        scanf("%d", &m);
        if (m <= 1)
            printf("Invalid size\n");
        else
            valid = 1;
    }

    // initialize buffer
    inputBuffer = initBuffer(n);
    outputBuffer = initBuffer(m);

    // initialize semaphore
    sem_init(&inputLock, 0, 1);
    sem_init(&outputLock, 0, 1);
    sem_init(&inputEmpty, 0, n);
    sem_init(&outputEmpty, 0, m);
    sem_init(&inputFull, 0, 0);
    sem_init(&outputFull, 0, 0);
    sem_init(&inputCount, 0, 0);
    sem_init(&outputCount, 0, 0);
    sem_init(&inputCounted, 0, 0);
    sem_init(&outputCounted, 0, 0);
    pthread_t threads[5];

    // create threads
    for (int i = 0; i < 5; i++)
    {
        int status;
        switch (i)
        {
        case 0:
            status = pthread_create(&threads[i], NULL, readerThread, NULL);
            break;
        case 1:
            status = pthread_create(&threads[i], NULL, inputCntThread, NULL);
            break;
        case 2:
            status = pthread_create(&threads[i], NULL, encryptThread, NULL);
            break;
        case 3:
            status = pthread_create(&threads[i], NULL, outputCntThread, NULL);
            break;
        case 4:
            status = pthread_create(&threads[i], NULL, writerThread, NULL);
            break;
        default:
            break;
        }
        if (status)
        {
            printf("Error: %d\n", status);
            freeBuffer(inputBuffer);
            freeBuffer(outputBuffer);
            exit(-1);
        }
    }

    // wait for threads to finish
    for (int i = 0; i < 5; i++)
    {
        pthread_join(threads[i], NULL);
    }

    freeBuffer(inputBuffer);
    freeBuffer(outputBuffer);
    sem_destroy(&inputLock);
    sem_destroy(&outputLock);
    sem_destroy(&inputEmpty);
    sem_destroy(&outputEmpty);
    sem_destroy(&inputFull);
    sem_destroy(&outputFull);
    sem_destroy(&inputCount);
    sem_destroy(&outputCount);
    sem_destroy(&inputCounted);
    sem_destroy(&outputCounted);
    log_counts();
    return 0;
}

// reads from input file one character at a time and places the  character in inputbuffer
void *readerThread(void *param)
{
    char c;
    while (1)
    {
        if (resetReq)
            pthread_cond_wait(&reset_cond, &reset_mutex);
        else
        {
            sem_wait(&inputEmpty);
            sem_wait(&inputLock);
            c = read_input();
            addNode(inputBuffer, c);
            sem_post(&inputLock);
            sem_post(&inputFull);
            sem_post(&inputCount);
        }

        if (c == EOF)
            pthread_exit(NULL);
    }
}

// counts occcurneces of each character in input file by looking at each charcter in input buffer
void *inputCntThread(void *param)
{
    struct node_t *curr;
    int counted;
    while (1)
    {
        counted = 0;
        sem_wait(&inputCount);
        sem_wait(&inputLock);
        curr = inputBuffer->head->next;
        while (curr != inputBuffer->tail)
        {
            if (curr->counted == 0)
            {
                if (curr->data != EOF)
                    count_input(curr->data);
                curr->counted = 1;
                counted++;
                if (curr->data == EOF)
                {
                    sem_post(&inputLock);
                    for (int i = 0; i < counted; i++)
                        sem_post(&inputCounted);
                    pthread_exit(NULL);
                }
            }
            curr = curr->next;
        }
        sem_post(&inputLock);
        for (int i = 0; i < counted; i++)
            sem_post(&inputCounted);
    }
    pthread_exit(NULL);
}

// encrypts the character one at a time and places it in output buffer
void *encryptThread(void *param)
{
    char c;
    while (1)
    {
        sem_wait(&inputCounted);
        sem_wait(&inputFull);
        sem_wait(&inputLock);
        c = removeNode(inputBuffer);
        sem_post(&inputLock);
        sem_post(&inputEmpty);

        sem_wait(&outputEmpty);
        sem_wait(&outputLock);
        if (c == EOF)
        {
            addNode(outputBuffer, EOF);
            sem_post(&outputLock);
            sem_post(&outputFull);
            sem_post(&outputCount);
            pthread_exit(NULL);
        }
        else
        {
            char enc = encrypt(c);
            addNode(outputBuffer, enc);
        }
        sem_post(&outputLock);
        sem_post(&outputFull);
        sem_post(&outputCount);
    }
    pthread_exit(NULL);
}

// output counter that looks at each character in output buffer
void *outputCntThread(void *param)
{
    struct node_t *curr;
    int counted;
    while (1)
    {
        counted = 0;
        sem_wait(&outputCount);
        sem_wait(&outputLock);
        curr = outputBuffer->head->next;
        while (curr != outputBuffer->tail)
        {
            if (curr->counted == 0)
            {
                if (curr->data != EOF)
                    count_output(curr->data);
                curr->counted = 1;
                counted++;
                if (curr->data == EOF)
                {
                    sem_post(&outputLock);
                    for (int i = 0; i < counted; i++)
                        sem_post(&outputCounted);
                    pthread_exit(NULL);
                }
            }
            curr = curr->next;
        }
        sem_post(&outputLock);
        for (int i = 0; i < counted; i++)
        {
            sem_post(&outputCounted);
        }
    }
    pthread_exit(NULL);
}

// writes encrypted character in putput buffer to output file
void *writerThread(void *param)
{
    char c;
    while (1)
    {
        sem_wait(&outputCounted);
        sem_wait(&outputFull);
        sem_wait(&outputLock);
        c = removeNode(outputBuffer);
        if (c == EOF)
        {
            sem_post(&outputLock);
            sem_post(&outputEmpty);
            pthread_exit(NULL);
        }
        else
        {
            write_output(c);
        }
        sem_post(&outputLock);
        sem_post(&outputEmpty);
    }
    pthread_exit(NULL);
}
