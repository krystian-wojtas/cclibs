// Clocks

#include <stdio.h>

unsigned  timebase_hz;
unsigned  basic_period_tb;
unsigned  n_start;
unsigned  n_end;
unsigned  k;
unsigned  n;

int main(int argc, char **argv)
{
    if(argc != 5)
    {
        puts("usage: clks timebase_hz basic_period_tb n_start n_end");
        exit(-1);
    }

    timebase_hz         = atoi(argv[1]);
    basic_period_tb     = atoi(argv[2]);
    n_start             = atoi(argv[3]);
    n_end               = atoi(argv[4]);

    for(n = n_start ; n <= n_end ; n++)
    {
        k = basic_period_tb / n;

        if(k * n == basic_period_tb)
        {
            printf("%u,%.4E,%.1f\n",n,(double)n/(double)timebase_hz,(double)timebase_hz/(double)n);
        }
    }
    exit(0);
}

// EOF
