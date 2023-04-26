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
}