int a;

void assignmentTest()
{
    a += 1;
    a *= 1;
    a /= 1;
    a %= 1;
    a += 1;
    a -= 1;
    a >>= 1;
    a <<= 1;
    a &= 1;
    a ^= 1;
    a |= 1;

    int b = 1;
    int c = a + b;
}

int main()
{
    return 0;
}