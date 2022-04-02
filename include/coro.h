#ifndef _CORO_H
#define _CORO_H

#include "common.h"

#define STACK_SZ (1 << 13)

enum co_status { CO_STATUS_CREATED, CO_STATUS_STARTED, CO_STATUS_STOPPED };

typedef uintptr_t co_context[14];

typedef struct {
    co_context context;
    co_context caller;
    int64_t yielded;
    enum co_status status;
    uint8_t stack[STACK_SZ];
} coroutine;

/*
 * Call co_yield in this function to hand out control.
 */
typedef void (*co_func)(coroutine *co, void *data);

__attribute__((malloc)) coroutine *co_new(co_func, void *data);

void co_free(coroutine *coro);

void co_yield(coroutine *coro, int64_t value);

int64_t co_resume(coroutine *coro);

int64_t co_resume_value(coroutine *coro, int64_t value);

#endif
