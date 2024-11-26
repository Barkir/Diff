#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>

#include "buff.h"
#include "diff.h"

int CreateNode(Node ** node, enum types type, field_t field)
{
    (*node) = (Node *) calloc(1, sizeof(Node));
    (*node)->type = type;
    (*node)->field = field;

    (*node)->left = (*node)->right = NULL;

    return 0;
}

void DestroyNode(Node * node)
{
    if (node)
    {
        DestroyNode(node->left);
        DestroyNode(node->right);
    }

    free(node);
}

Node * _node_dump_func(Node * node, FILE * Out)
{

    if (!node)
        return node;

    unsigned int color = NodeColor(node);

    switch (node->type)
    {
        case OPER:  fprintf(Out, "node%p [shape = Mrecord; label = \"{%c}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    node, (int) node->field, color);
                    break;

        case VAR:   fprintf(Out, "node%p [shape = Mrecord; label = \"{%c}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    node, (int) node->field, color);
                    break;
        case NUM:   fprintf(Out, "node%p [shape = Mrecord; label = \"{%lg}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    node, node->field, color);
                    break;

        default:    fprintf(Out, "node%p [shape = Mrecord; label = \"{}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    node, color);
                    break;

    }

    if (node->left)
        fprintf(Out, "node%p -> node%p\n", node, node->left);

    if (node->right)
        fprintf(Out, "node%p -> node%p\n", node, node->right);

    _node_dump_func(node->left, Out);
    _node_dump_func(node->right, Out);

    return node;
}

Node * NodeDump(Node * node, const char * FileName)
{
    FILE * Out = fopen(FileName, "wb");

    fprintf(Out, "digraph\n{\n");
    _node_dump_func(node, Out);
    fprintf(Out, "}\n");

    char command[DEF_SIZE] = "";
    sprintf(command, "dot %s -T png -o %s.png", FileName, FileName);

    fclose(Out);

    system(command);

    return node;
}

int NodeParse(Node ** node, const char * filename)
{
    FILE * file = fopen(filename, "rb");
    if (ferror(file)) return FOPEN_ERROR;

    char * expression = CreateBuf(file);
    if (!expression) return ALLOCATE_MEMORY_ERROR;

    const char * ptr = expression;

    _node_parse(node, &ptr);

    if (fclose(file) == EOF) return FCLOSE_ERROR;
    free(expression);

    return 1;
}

#define PRINT(...)                                                               \
    {                                                                            \
    fprintf(stderr, ">>> %s:%d: [%d]%*s", __FILE__, __LINE__, deep, deep*4, ""); \
    fprintf(stderr, __VA_ARGS__);                                                \
    fprintf(stderr, " Cur = '%.20s'...\n", *string);                             \
    }

int _node_parse(Node ** node, const char ** string)
{
    static int deep = 0;
    deep++;

    PRINT ("Starting...");

    fprintf(stderr, "\n______________\n");
    enum types type = VAR;
    field_t field = 0;

    PRINT ("Start skipping WS");

    while (isspace(**string)) (*string)++;

    PRINT ("WS skipped, searching for '('.");

    if (**string == '(')
    {
        PRINT ("FOUND '('.");

        (*string)++;

        PRINT ("SKIPPED '('");
    }
    else
    {
        PRINT("NOT FOUND '('. ERROR creating node, return -1");
        return -1;
    }

    const char * ptr = *string;
    PRINT ("ptr = '%.20s'", ptr);

    if (isdigit(*ptr))
    {
        PRINT ("ISDIGIT state.");

        char * end = NULL;
        field = strtod(ptr, &end);
        if (!end)
        {
            deep--;
            return -1;
        }
        (*string) = end;


        type = NUM;

        PRINT ("Got field = %lg. Type = NUM. ptr = '%.20s'", field, ptr);
    }
    else if (isalpha(*ptr))
    {
        PRINT ("ISALPHA state.");

        field = (field_t)(*ptr);
        type = VAR;

        (*string)++;

        PRINT ("Got field = %lg (%c). Type = VAR. ptr = '%.20s'", field, (int) field, ptr);
    }
    else if (*ptr == '+' || *ptr == '-' || *ptr == '*' || *ptr == '/' || *ptr == '^')
    {
        PRINT ("OPER state.");

        field = (field_t)(*ptr);
        type = OPER;

        (*string)++;

        PRINT ("Got field = %lg (%c). Type = OPER. ptr = '%.20s'", field, (int) field, ptr);
    }

    else
        {
        PRINT ("Terminal ELSE reached");

        if (!(*ptr))
            {
            PRINT ("!*ptr detected. ptr-3 = '%.20s'. Return -1.", ptr-3);
            deep--;
            return -1;
            }
        }

    PRINT ("Creating a node...");

    if (CreateNode(node, type, field) == -1)
        {
        PRINT ("Creating node FAILED. Return -1.");
        deep--;
        return -1;
        }

    PRINT ("Created NODE [%p]: type = %d, field = %lg (%c)", *node, type, field, (int) field);

    PRINT ("Skipping WSs before possible left subtree");
    while (isspace(**string)) (*string)++;
    PRINT ("WSs SKIPPED. Testing for '(' -- possible left subtree.")

    if (**string == '(')
        {
        PRINT ("...FOUND '('. Did NOT skipped '('. Calling LEFT subtree read.");
        _node_parse (&(*node)->left, string);
        }

    PRINT ("Now searching for WSs...");
    while (isspace(**string)) (*string)++;
    PRINT ("SKIPPED WSs. Checking for '(' -- possible right subtree...");

    if (**string == '(')
        {
        PRINT ("...FOUND '('. Did NOT skipped '('. Calling RIGHT subtree read.");
        _node_parse (&(*node)->right, string);
        }

    PRINT ("COMPLETELY read the node. Searching for ')'...");

    PRINT ("Now searching for WSs...");
    while (isspace(**string)) (*string)++;
    PRINT ("SKIPPED WSs. Checking for ')' -- possible end of node...");

    if (**string == ')')
        {
        (*string)++;

        PRINT ("...FOUND ')' and SKIPPED. Node FINISHED. Return 0.");
        deep--;
        return 0;
        }
    else
        {
        (*string)++;

        PRINT ("...NOT FOUND ')'!!! ERROR! Return -1.");
        deep--;
        return -1;
        }
}

unsigned int NodeColor(Node * node)
{
    unsigned int color = 0;
        switch (node->type)
    {
        case OPER:
            color = OPER_COLOR;
            break;

        case VAR:
            color = VAR_COLOR;
            break;

        case NUM:
            color = NUM_COLOR;
            break;

        default:
            break;
    }

    return color;
}

const char * NodeType(Node * node)
{
    const char * type = 0;

        switch (node->type)
    {
        case OPER:
            type = "Operation";
            break;

        case VAR:
            type = "Variable";
            break;

        case NUM:
            type = "Number";
            break;

        default:
            break;
    }

    return type;
}


int FindVar(Node * node)
{
    if (!node) return 0;
    if (node->type == VAR) return 1;

    int result = FindVar(node->left);
    if (!result) result = FindVar(node->right);
    return result;

}

field_t _node_count(Node * node, field_t val, field_t var)
{
    if (node->type == NUM) return node->field;
    if (node->type == VAR) return var;

    if (node->type == OPER)
    {
        switch((int) node->field)
        {
            case '+': val = _node_count(node->left, val, var) + _node_count(node->right, val, var); break;
            case '-': val = _node_count(node->left, val, var) - _node_count(node->right, val, var); break;
            case '*': val = _node_count(node->left, val, var) * _node_count(node->right, val, var); break;
            case '/': val = _node_count(node->left, val, var) / _node_count(node->right, val, var); break;
            case '^': val = pow(_node_count(node->left, val, var), _node_count(node->right, val, var)); break;
        }
    }

    return val;
}

field_t NodeCount(Node * node, field_t var)
{
    return _node_count(node, 0, var);
}

