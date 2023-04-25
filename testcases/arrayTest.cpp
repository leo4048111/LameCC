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

int main()
{
    int a[5];
    a[0] += 1;
    a[1] = 3;
    putInt(a[1]);
}