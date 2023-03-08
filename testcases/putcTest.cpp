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
    int a = 98765;
    putInt(a);
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
}