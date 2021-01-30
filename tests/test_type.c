int assert(int expected, int actual);

void foo()
{
    int a = 1;

    if (a > 0)
        return;

    return;
}

struct point {
    int x, y, z;
};

enum color {
    R, G, B
};

typedef int id_t;

int main()
{
    {
        /* char data type */
        char x[2];
        int a;

        x[0] = 2;
        assert(2, x[0]);

        *(x + 1) = -7;
        assert(-7, x[1]);

        x[0] = x[1] - x[0];
        assert(-9, x[0]);

        a = 51;
        assert(42, a + x[0]);
    }
    {
        /*
        void i = 0;
        */
    }
    {
        struct point pt;
        int a = 42;
        int *p = &a;
        char c = 92;

        assert(4, sizeof a);

        assert(4, sizeof(a + 2));

        assert(8, sizeof p);

        assert(4, sizeof *p);

        assert(1, sizeof c);

        assert(12, sizeof pt);

        assert(4, sizeof pt.x);

        assert(12, sizeof (struct point ));

        assert(4, sizeof R);

        assert(4, sizeof (enum color));
    }
    {
        typedef char token_t;

        id_t id = 123;
        token_t tok = 97;

        assert(123, id);
        assert(4, sizeof id);
        assert(4, sizeof(id_t));

        assert(97, tok);
        assert(1, sizeof tok);
        assert(1, sizeof(token_t));
    }

    return 0;
}
