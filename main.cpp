#include <stdio.h>

#include "diff.h"
#include "buff.h"

int main(void)
{
    Node * node = NULL;
    NodeParse(&node, "toparse.txt");

    NodeDump(node, "res");

    field_t result = NodeCount(node, 3);
    printf("RESULT =  %lg\n", result);

    DestroyNode(node);
}
