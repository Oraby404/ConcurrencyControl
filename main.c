//
// Created by oraby on 24/12/2021.
//

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////

#define TYPE int

typedef struct {
    int head, tail, numberItems;
    int Capacity;
    TYPE *items;
} Queue;

Queue *initialize(int size) {
    Queue *q = malloc(sizeof(Queue));
    q->head = 0;
    q->tail = 0;
    q->numberItems = 0;
    q->Capacity = size;
    q->items = malloc(size * sizeof(TYPE));
    return q;
}

void enqueue(Queue *q, TYPE value) {
    q->items[q->tail++] = value;
    q->tail %= (q->Capacity);
    q->numberItems++;
}

TYPE dequeue(Queue *q) {
    q->head %= (q->Capacity);
    q->numberItems--;
    return q->items[q->head++];
}

int isFull(Queue *q) {
    return q->numberItems == q->Capacity ? 1 : 0;
}

int isEmpty(Queue *q) {
    return q->numberItems == 0 ? 1 : 0;
}
/////////////////////////////////////////////////////////////////////////////////

#define   sizeOfBuffer  100
#define        N         5

pthread_t mCounter[N];

sem_t c;//for counter

sem_t s;//producer or consumer -> touch buffer one at a time
sem_t n;//count of items in the buffer -> if empty buffer block consumer
sem_t e;//count of empty spots in the buffer (for finite buffer) -> if full buffer block producer


int data = 0;

void *Counter(void *);

void *Producer(void *);

void *Consumer(void *);


int main(int argc, char *argv[]) {

    Queue *queue = initialize(sizeOfBuffer);

    pthread_t mCollector, mMonitor;

    sem_init(&c, 0, 1);

    sem_init(&s, 0, 1);
    sem_init(&n, 0, 0);
    sem_init(&e, 0, sizeOfBuffer);

    pthread_create(&mCollector, NULL, &Consumer, queue);
    pthread_create(&mMonitor, NULL, &Producer, queue);

    for (int i = 0; i < N; i++)
        pthread_create(&mCounter[i], NULL, &Counter, NULL);

    for (int i = 0; i < N; i++)
        pthread_join(mCounter[i], NULL);
    pthread_join(mCollector, NULL);
    pthread_join(mMonitor, NULL);

}

void *Counter(void *arg) {
    while (1) {

        int i;
        pthread_t id = pthread_self();
        for (i = 0; i < N; i++) {
            if (pthread_equal(id, mCounter[i])) {
                printf("Counter Thread %d received a message\n", i);
                break;
            }
        }

        printf("Counter thread : %d waiting to write\n", i);

        sem_wait(&c);
        data++;

        printf("Counter thread : %d Now adding to counter \n", i);
        printf("Counter value : %d \n", data);

        sem_post(&c);

        int random = rand() % 10;
        printf("Counter Thread %d is sleeping for %d seconds\n", i, random);
        sleep(random);

    }
}

void *Producer(void *arg) {
    while (1) {
        printf("Monitor Thread: waiting to read counter\n");

        //critical section for counter
        sem_wait(&c);//wait for counter to read it
        int value = data;
        data = 0;
        printf("Monitor thread: reading a count value of %d \n", value);
        sem_post(&c);

        sem_wait(&e);//wait for empty spot

        //critical section for buffer
        sem_wait(&s);

        Queue *queue = arg;
        if (isFull(queue))
            printf("Monitor thread: Buffer full!\n");
        else {
            enqueue(queue, value);
            printf("Monitor thread: writing to buffer at position %d\n", queue->tail - 1);
        }
        sem_post(&s);

        sem_post(&n);//an item is added to buffer

        int random = rand() % 10;
        printf("Monitor Thread is sleeping for %d seconds\n", random);
        sleep(random);
    }
}

void *Consumer(void *arg) {
    while (1) {
        sem_wait(&n);//wait for n (buffer may be empty)

        //critical section for buffer
        sem_wait(&s);

        Queue *queue = arg;
        if (isEmpty(queue))
            printf("Collector thread: nothing is in the buffer!\n");
        else
            printf("Collector thread: reading from the buffer at position %d value %d \n", queue->head - 1,
                   dequeue(queue));

        sem_post(&s);

        sem_post(&e);//an empty spot is produced

        int random = rand() % 10;
        printf("Collector Thread is sleeping for %d seconds\n", random);
        sleep(random);
    }
}