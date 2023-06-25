extern "C" int putchar(char a);
extern "C" int puts(char *a);

void putInt(int i)
{
    if (i < 0)
    {
        putchar('-');
        i = -i;
    }
    if (i >= 10)
        putInt(i / 10);

    putchar(i % 10 + '0');
}

// nonvoid return type function decl with params
int NonVoidFuncDeclWithParams(int parm1, int parm2);

// nonvoid return type function decl without params
char NonVoidFuncDeclWithoutParams();

// nonvoid return type function definition with params
float NonVoidFuncDefWithoutParamsWithEmptyBody()
{
    return 0xAF.D65P-5; // some float representations
}

// nonvoid return type function definition with params
int NonVoidFuncDefWithParamsWithEmptyBody(int param1, int param2)
{
    return 0;
}

// void return type function decl with params
void VoidFuncDeclWithParams(int parm1, int parm2);

// nonvoid return type function decl without params
void VoidFuncDeclWithoutParams();

// void return type function definition with params with empty body
void VoidFuncDefWithoutParamsWithEmptyBody()
{
}

// void return type function definition with params with empty body
void VoidFuncDefWithParamsWithEmptyBody(int param1, int param2)
{
    
}

void integerLiteralTest()
{
    puts("Integer literal test:");
    int a = 0;
    while(a < 100) {
        putInt(a);
        putchar(' ');
        a++;
    }
    putchar('\n');
}

void arrayTest()
{
    puts("Array test:");
    int a[10];
    int idx = 0;
    puts("Before sorted:");
    while(idx < 10)
    {
        a[idx] = 10 - idx;
        putInt(a[idx]);
        putchar(' ');
        idx++;
    }
    puts("");
    puts("Bubble sorted:");
    int i = 0;
    while(i < 10)
    {
        int j = i;
        while(j < 10)
        {
            if (a[i] > a[j])
            {
                int tmp = a[i];
                a[i] = a[j];
                a[j] = tmp;
            }
            j = j + 1;
        }
        i++;
    }

    i = 0;
    while(i < 10)
    {
        putInt(a[i]);
        putchar(' ');
        i = i + 1;
    }

    puts("");
}

int inlineAsmTest(int num1, int num2)
{
    int result = 0;
    __asm__ ("movl %1, %%eax;"
             "movl %2, %%ebx;"
             "addl %%ebx, %%eax;"
             "movl %%eax, %0;"
             :"=r"(result)
             :"r"(num1), "r"(num2)
             :);

    return result;
}

int main()
{
    // external function linkage test
    puts("------------------------------------------------------------");
    puts(".____                          _________ _________"); 
    puts("|    |   _____    _____   ____ \\_   ___ \\\\_   ___ \\"); 
    puts("|    |   \\__  \\  /     \\_/ __ \\/    \\  \\//    \\  \\/");
    puts("|    |___ / __ \\|  Y Y  \\  ___/\\     \\___\\     \\____");
    puts("|_______ (____  /__|_|  /\\___  >\\______  /\\______  /");
    puts("------------------------------------------------------------");

    // run integer literal test
    integerLiteralTest();

    // run array test
    arrayTest();

    // run inline asm test(which is a asm implementation of sum function)
    puts("Inline asm test:");
    int res = inlineAsmTest(1, 2);
    putInt(res);
}