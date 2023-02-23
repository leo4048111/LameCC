// #include <stdio.h>

int main()
{
    // basic asm
    __asm__("movl %eax, %ebx\n\t"
            "movl $56, %esi\n\t"
            "movl %ecx, $label(%edx,%ebx,$4)\n\t"
            "movb %ah, (%ebx)"
            );

    // extended asm
    int a = 10;
    int b = 10;
    __asm__("movl %1, %%eax\n\t"
        "movl %%eax, %0\n\t"
        "movl $99, %%eax"
        : "=r"(b) /* output */
        : "r"(a)  /* input */
        : "%eax"  /* clobbered register */
    );

    // printf("%d", foo);
    return 0;
}