int num()
{
    int n;
    n = 1;
    return n;
}

int add(int x, int y)
{
    return x + y;
}

int main()
{
    int ret;
    ret = add(40, 2) + 2 * num();

    if (ret == 44)
        return 0;
    else
        return 1;
}
