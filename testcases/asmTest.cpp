//#include <stdio.h>

int main()
{
    // basic asm
    __asm__("movl %eax, %ebx\n\t"
            "movl $56, %esi\n\t"
            "movl $999, %ebx"
            );

    // extended asm
    int a = 10;
    int b = 10;
    __asm__("movl %1, %%eax\n\t"
        "movl %%eax, %0\n\t"
        "movl $1099, %%eax\n\t"
        "movl %1, %%eax\n\t"
        "movl %%eax, %0"
        : "=a"(b) /* output */
        : "r"(a + 9), "a"(a), "b"(a), "c"(a)  /* input */
        :"%edx"  /* clobbered register */
    );

    //printf("%d", b);
    return 0;
}