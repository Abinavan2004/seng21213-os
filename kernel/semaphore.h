#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include"../include/types.h"

typedef struct
{
    int value;
} semaphore_t;

void semaphore_init(semaphore_t *sem , int value);
void semaphore_wait(semaphore_t *sem);
void semaphore_signal(semaphore_t *sem);

#endif