#include "queue.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

struct queue {
    void **items;
    int max_capacity;
    int head_index;
    int tail_index;
    pthread_mutex_t mutex;
    sem_t sem_full, sem_empty;
};

queue_t *queue_new(int size) {
    queue_t *queue = malloc(sizeof(queue_t));
    if (!queue)
        return NULL;

    queue->items = malloc(sizeof(void *) * size);
    if (!queue->items) {
        free(queue);
        return NULL;
    }

    queue->max_capacity = size;
    queue->head_index = 0;
    queue->tail_index = -1;
    pthread_mutex_init(&queue->mutex, NULL);
    sem_init(&queue->sem_empty, 0, size);
    sem_init(&queue->sem_full, 0, 0);
    return queue;
}

void queue_delete(queue_t **queue) {
    pthread_mutex_lock(&(*queue)->mutex);
    free((*queue)->items);
    pthread_mutex_unlock(&(*queue)->mutex);
    pthread_mutex_destroy(&(*queue)->mutex);
    sem_destroy(&(*queue)->sem_empty);
    sem_destroy(&(*queue)->sem_full);
    free(*queue);
    *queue = NULL;
}

bool queue_push(queue_t *queue, void *item) {
    sem_wait(&queue->sem_empty);
    pthread_mutex_lock(&queue->mutex);

    queue->tail_index = (queue->tail_index + 1) % queue->max_capacity;
    queue->items[queue->tail_index] = item;

    pthread_mutex_unlock(&queue->mutex);
    sem_post(&queue->sem_full);

    return true;
}

bool queue_pop(queue_t *queue, void **item) {
    sem_wait(&queue->sem_full);
    pthread_mutex_lock(&queue->mutex);

    *item = queue->items[queue->head_index];
    queue->head_index = (queue->head_index + 1) % queue->max_capacity;

    pthread_mutex_unlock(&queue->mutex);
    sem_post(&queue->sem_empty);

    return true;
}
