#ifndef ENCRYPT_H
#define ENCRYPT_H

struct buffer_t {
    struct node_t *head;
    struct node_t *tail;
    int size;
    int maxSize;
};
struct node_t {
    char data;
    int counted;
    struct node_t *next;
    struct node_t *prev;
};


/* You must implement this function.
 * When the function returns the encryption module is allowed to reset.
 */
void reset_requested();
/* You must implement this function.
 * The function is called after the encryption module has finished a reset.
 */
void reset_finished();

/* You must use these functions to perform all I/O, encryption and counting
 * operations.
 */
void init(char *inputFileName, char *outputFileName, char *logFileName);
int read_input();
void write_output(int c);
void log_counts();
int encrypt(int c);
void count_input(int c);
void count_output(int c);
int get_input_total_count();
int get_input_count(int c);
int get_output_count(int c);
int get_output_total_count();
int addNode(struct buffer_t *b, char c);
char removeNode(struct buffer_t *b);
struct node_t* unlink(struct buffer_t *b);
void printBuffer(struct buffer_t *b);
void link(struct buffer_t *b, struct node_t *n);
void freeBuffer(struct buffer_t *b);
int isFull(struct buffer_t *b);
void *readerThread(void *param);
int isEmpty(struct buffer_t *b);
void *inputCntThread(void *param);
void *writerThread(void *param);
void *encryptThread(void *param);
void reset_finished();
void *outputCntThread(void *param);
void reset_requested();
struct node_t * allocNode(char);
struct buffer_t * initBuffer(int);
int addNode(struct buffer_t *, char);
char removeNode(struct buffer_t *);
struct node_t * unlink(struct buffer_t *);
void link(struct buffer_t *, struct node_t *);
void printBuffer(struct buffer_t *);
int isEmpty(struct buffer_t *);
int isFull(struct buffer_t *);
void freeBuffer(struct buffer_t *);



#endif // ENCRYPT_H
