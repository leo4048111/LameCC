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
    int a[10];
    int idx = 0;
    while(idx < 10)
    {
        a[idx] = 10 - idx;
        idx++;
    }

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
        puts("");
        i = i + 1;
    }

    return 0;
}