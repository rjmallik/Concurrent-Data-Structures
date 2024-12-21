#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "rwlock.h"

typedef struct rwlock {
    pthread_mutex_t lock;
    pthread_cond_t condition_read, condition_write;
    int count_readers_active, count_writers_active, count_readers_waiting, count_writers_waiting;
    int count_readers_served;
    PRIORITY lock_priority;
    int threshold_n_way;
    int count_n_way;
} rwlock_t;

static int check_writer_lock(rwlock_t *lock) {
    return lock->count_readers_active == 0 && lock->count_writers_active == 0;
}

rwlock_t *rwlock_new(PRIORITY priority, uint32_t n) {
    rwlock_t *lock = malloc(sizeof(rwlock_t));
    if (lock == NULL)
        return NULL;
    pthread_mutex_init(&lock->lock, NULL);
    pthread_cond_init(&lock->condition_read, NULL);
    pthread_cond_init(&lock->condition_write, NULL);
    lock->count_readers_active = lock->count_writers_active = 0;
    lock->count_readers_waiting = lock->count_writers_waiting = 0;
    lock->count_readers_served = 0;
    lock->lock_priority = priority;
    lock->threshold_n_way = n;
    lock->count_n_way = 0;
    return lock;
}

void rwlock_delete(rwlock_t **lock_ref) {
    if (lock_ref == NULL || *lock_ref == NULL)
        return;
    rwlock_t *lock = *lock_ref;
    pthread_mutex_destroy(&lock->lock);
    pthread_cond_destroy(&lock->condition_read);
    pthread_cond_destroy(&lock->condition_write);
    free(lock);
    *lock_ref = NULL;
}

void reader_lock(rwlock_t *lock) {
    pthread_mutex_lock(&lock->lock);
    lock->count_readers_waiting++;
    while (lock->count_writers_active > 0
           || (lock->lock_priority == WRITERS && lock->count_writers_waiting > 0)) {
        pthread_cond_wait(&lock->condition_read, &lock->lock);
    }
    lock->count_readers_waiting--;
    lock->count_readers_active++;
    pthread_mutex_unlock(&lock->lock);
    if (lock->lock_priority == N_WAY) {
        pthread_mutex_lock(&lock->lock);
        lock->count_n_way++;
        if (lock->count_n_way >= lock->threshold_n_way) {
            pthread_cond_signal(&lock->condition_write);
            lock->count_n_way = 0;
        }
        pthread_mutex_unlock(&lock->lock);
    }
}


void reader_unlock(rwlock_t *lock) {
    pthread_mutex_lock(&lock->lock);
    lock->count_readers_active--;
    if (lock->count_readers_active == 0) {
        if (lock->count_writers_waiting > 0) {
            pthread_cond_signal(&lock->condition_write);
        } else {
            pthread_cond_broadcast(&lock->condition_read);
        }
    }
    pthread_mutex_unlock(&lock->lock);
}

void writer_lock(rwlock_t *lock) {
    pthread_mutex_lock(&lock->lock);
    lock->count_writers_waiting++;
    while (!check_writer_lock(lock)) {
        pthread_cond_wait(&lock->condition_write, &lock->lock);
    }
    lock->count_writers_waiting--;
    lock->count_writers_active++;
    pthread_mutex_unlock(&lock->lock);
}

void writer_unlock(rwlock_t *lock) {
    pthread_mutex_lock(&lock->lock);
    lock->count_writers_active--;
    lock->count_readers_served = 0;
    if (lock->count_writers_waiting > 0) {
        pthread_cond_signal(&lock->condition_write);
    } else {
        pthread_cond_broadcast(&lock->condition_read);
    }
    pthread_mutex_unlock(&lock->lock);
}
                                                                                                                     102,1         Bot
