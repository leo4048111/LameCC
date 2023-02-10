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
}

int func(int a)
{
    return a;
}

void callExprTest()
{
    func(1);
}

int main()
{
    return 0;
}