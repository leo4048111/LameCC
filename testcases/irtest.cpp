int g_a = 3.1415926;


void func1(int a)
{
    int b = a;
}

void func(int tmp)
{
    int a = tmp;
    int b = 2 + 3;
    int c = (a + tmp) * 6;
}

void func3(int tmp)
{
    if(g_a) {
        g_a = 2;
    }
    else {
        g_a = 3;
    }

    return;
}