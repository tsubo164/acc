/*
int printf(char *s, int i);
enum color {
    R, G, B
};
*/

/*
struct point {
    int x, y;
};

int add(int x, int y)
{
    return x + y + z;
}
*/

int size(int a)
{
    /*
    printf("size: %d\n", a);
    */
    return a;
}
int main()
{
        /*
    int i;

    for (i = 0; i < 10; i++)
        ;
        printf("i -> %d\n", i);
        */

    /*
    int a = 5;

    if (a > 3)
        goto error;
    else
        goto final;

error:
    printf("**** error\n", 0);

final:
    printf("**** error\n", 0);

    return 42;
    */

    /*
    {
        int a = 12;

        switch (a) {
        case 1:
            return a;
        case 2:
            break;
        case 12:
            switch (110) {
            case 110:
                break;
            case 111:
                break;
            case 12:
                break;
            case 13:
                break;
            case 112:
                break;
            }
            break;
            return a;

        case 13:
            return a;

        default:
            break;
        }
    }
    goto foo;
    {
error:
    }
    */
    {
        struct point {
            int x, y;
        };
        struct point p;
        /*
        int b = sizeof (struct point);
        size(sizeof(struct point));
        size(sizeof(p));
        */
        size(8);

        return 0;
    }
    /*
    struct point pt;
    int a = 42;
    int *p = 0;
    char c = 0;
    int b;

    b = sizeof(a + 2);
    printf("size of b: => %d\n", b);

    b = sizeof p;
    printf("size of b: => %d\n", b);

    b = sizeof c;
    printf("size of b: => %d\n", b);
    */

    /*
    b = sizeof pt;
    printf("size of b: => %d\n", b);

    return b;
    return sizeof pt;
    return sizeof (struct point);
    */
}
