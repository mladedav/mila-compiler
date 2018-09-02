#include <stdio.h>

int printi ( int x )
{
    printf ( "%d\n", x );
    return 0;
}

int writeln ( int x )
{
    printf ( "%d\n", x );
    return 0;
}

int readln ( int * x )
{
    scanf ( "%d", x );
    return 0;
}

int inc ( int * x )
{
    return ++*x;
}

int dec ( int * x )
{
    return --*x;
}
