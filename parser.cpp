#include <stdio.h>
#include <assert.h>
#include "parser.h"

const char * string = "(2+3)*7$";
static int p = 0;

int GetN(void)
{
    int old_p = p;
    int val = 0;
    while (string[p] > '0' && string[p] < '9')
    {
        val = val * 10 + string[p] - '0';
        p++;
    }
    if (p == old_p) SyntaxError();
    return val;
}

int GetE(void)
{
    int val = GetT();
    while (string[p] == '+' || string[p] == '-')
    {
        int op = string[p];
        p++;
        int val2 = GetT();
        if (op == '+') val += val2;
        else val -= val2;
    }

    return val;
}

int GetG(void)
{
    int val = GetE();
    if (string[p] != '$') SyntaxError();
    p++;
    return val;
}

int GetT(void)
{
    int val = GetP();
    while (string[p] == '*' || string[p] == '/')
    {
        int op = string[p];
        p++;
        int val2 = GetP();
        if (op == '*') val *= val2;
        else val /= val2;
    }

    return val;
}

int GetP(void)
{
    if (string[p] == '(')
    {
        p++;
        int val = GetE();
        if (string[p] != ')') SyntaxError();
        p++;
        return val;
    }
    else
        return GetN();
}

int SyntaxError()
{
    fprintf(stderr, "ERROR");
    assert(0);
}
