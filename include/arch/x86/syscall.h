#ifndef _SYSCALL_X86_H
#define _SYSCALL_X86_H

#define syscall_0(NR)                                                          \
    ({                                                                         \
        int64_t rax;                                                           \
        asm volatile("syscall\n\t"                                             \
                     : "=a"(rax)                                               \
                     : "a"(NR)                                                 \
                     : "rcx", "r11", "memory");                                \
        rax;                                                                   \
    })

#define syscall_1(NR, rdi)                                                     \
    ({                                                                         \
        int64_t rax;                                                           \
        asm volatile("syscall\n\t"                                             \
                     : "=a"(rax)                                               \
                     : "a"(NR), "D"((rdi))                                     \
                     : "rcx", "r11", "memory");                                \
        rax;                                                                   \
    })

#define syscall_2(NR, rdi, rsi)                                                \
    ({                                                                         \
        int64_t rax;                                                           \
        asm volatile("syscall\n\t"                                             \
                     : "=a"(rax)                                               \
                     : "a"(NR), "D"((rdi)), "S"((rsi))                         \
                     : "rcx", "r11", "memory");                                \
        rax;                                                                   \
    })

#define syscall_3(NR, rdi, rsi, rdx)                                           \
    ({                                                                         \
        int64_t rax;                                                           \
        asm volatile("syscall\n\t"                                             \
                     : "=a"(rax)                                               \
                     : "a"(NR), "D"((rdi)), "S"((rsi)), "d"((rdx))             \
                     : "rcx", "r11", "memory");                                \
        rax;                                                                   \
    })

#define syscall_4(NR, rdi, rsi, rdx, r10)                                      \
    ({                                                                         \
        int64_t rax;                                                           \
        asm volatile("syscall\n\t"                                             \
                     : "=a"(rax)                                               \
                     : "a"(NR), "D"((rdi)), "S"((rsi)), "d"((rdx)), "r"((r10)) \
                     : "rcx", "r11", "memory");                                \
        rax;                                                                   \
    })

#define syscall_5(NR, rdi, rsi, rdx, r10, r8)                                  \
    ({                                                                         \
        int64_t rax;                                                           \
        asm volatile("syscall\n\t"                                             \
                     : "=a"(rax)                                               \
                     : "a"(NR), "D"((rdi)), "S"((rsi)), "d"((rdx)),            \
                       "r"((r10)), "r"((r8))                                   \
                     : "rcx", "r11", "memory");                                \
        rax;                                                                   \
    })

#define syscall_6(NR, rdi, rsi, rdx, r10, r8, r9)                              \
    ({                                                                         \
        int64_t rax;                                                           \
        asm volatile("syscall\n\t"                                             \
                     : "=a"(rax)                                               \
                     : "a"(NR), "D"((rdi)), "S"((rsi)), "d"((rdx)),            \
                       "r"((r10)), "r"((r8)) "r"((r9))                         \
                     : "rcx", "r11", "memory");                                \
        rax;                                                                   \
    })

#endif
