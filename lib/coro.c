#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "coro.h"

__attribute__((naked, noinline, visibility("internal"))) void
swap_context(co_context *old_context, co_context *new_context)
{
#if defined(__x86_64__)
    /*
     * A simplified implementation from glibc ucontext.
     * parameter order: %rdi, %rsi, %rdx, %rcx, %r8, %r9
     */
    asm("movq   %rbx, 0(%rdi)\n\t" /* start store old_context */
        "movq   %rbp, 8(%rdi)\n\t"
        "movq   %r12, 16(%rdi)\n\t"
        "movq   %r13, 24(%rdi)\n\t"
        "movq   %r14, 32(%rdi)\n\t"
        "movq   %r15, 40(%rdi)\n\t"
        "movq   %rdi, 48(%rdi)\n\t"
        "movq   %rsi, 56(%rdi)\n\t"
        "movq   %rdx, 64(%rdi)\n\t"
        "movq   %rcx, 72(%rdi)\n\t"
        "movq   %r8, 80(%rdi)\n\t"
        "movq   %r9, 88(%rdi)\n\t"
        "movq   (%rsp), %rcx\n\t" /* RPI */
        "movq   %rcx, 96(%rdi)\n\t"
        "leaq   8(%rsp), %rcx\n\t" /* RSP */
        "movq   %rcx, 104(%rdi)\n\t"
        "movq   104(%rsi), %rsp\n\t" /* start restore new_context */
        "movq   0(%rsi), %rbx\n\t"
        "movq   8(%rsi), %rbp\n\t"
        "movq   16(%rsi), %r12\n\t"
        "movq   24(%rsi), %r13\n\t"
        "movq   32(%rsi), %r14\n\t"
        "movq   40(%rsi), %r15\n\t"
        "movq   48(%rsi), %rdi\n\t"
        "movq   64(%rsi), %rdx\n\t"
        "movq   72(%rsi), %rcx\n\t"
        "movq   80(%rsi), %r8\n\t"
        "movq   88(%rsi), %r9\n\t"
        "movq   96(%rsi), %r10\n\t" /* save to R10 before RSI changed */
        "movq   56(%rsi), %rsi\n\t"
        "jmpq   *%r10\n\t" /* go to old RPI ? */
    );
#else
#error Only x86_64 supported.
#endif
}

__attribute__((visibility("internal"))) void
entry_point(coroutine *co, co_func func, void *data)
{
    co->status = CO_STATUS_STARTED;
    func(co, data);
    co->status = CO_STATUS_STOPPED;
    return (void)co_yield(co, -1);
}

__attribute__((malloc)) coroutine *co_new(co_func func, void *data)
{
    coroutine *co = NULL;

    if (UNLIKELY(posix_memalign((void **)&co, 64, sizeof(coroutine)) != 0))
        return NULL;

    memset(co, 0, sizeof(coroutine));

    co->status = CO_STATUS_CREATED;
    /*
     * Set up arguments 0,1,2 before entering entry_point
     */
    co->context[6]  = (uintptr_t)co;   /* RDI */
    co->context[7]  = (uintptr_t)func; /* RSI */
    co->context[8]  = (uintptr_t)data; /* RDX */
    co->context[12] = (uintptr_t)entry_point;

    /*
     *
     * RSP point to top of stack,
     * grow from high memory address to low memory address.
     *
     * DO NOT DEREFERENCE IT!
     */
    uintptr_t rsp = (uintptr_t)(co->stack + STACK_SZ);

    /*
     * The stack pointer %rsp must be aligned to a 16-byte boundary before
     * making a call.
     *
     * See below discussion for why -8 is necessary:
     * https://stackoverflow.com/questions/62714764/should-rsp-be-aligned-to-16-byte-boundary-before-calling-a-function-in-nasm
     */
    co->context[13] = (rsp & ~0xFul) - 0x8ul;

    return co;
}

void co_yield(coroutine *co, int64_t value)
{
    co->yielded = value;
    swap_context(&co->context, &co->caller);
}

int64_t co_resume(coroutine *co)
{
    swap_context(&co->caller, &co->context);
    return co->yielded;
}
