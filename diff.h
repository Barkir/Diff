#ifndef DIFF_H
#define DIFF_H

typedef double field_t;

const int DEF_SIZE = 1024;

enum colors
{
    OPER_COLOR = 0XEFF94F,
    NUM_COLOR = 0X5656EC,
    VAR_COLOR = 0X70Df70
};

enum types
{
    OPER = 0,
    VAR = 1,
    NUM = 2
};

enum operations
{
    ADD,
    SUB,
    MUL,
    DIV,
    POW,
};

enum errors
{
    SUCCESS,
    ALLOCATE_MEMORY_ERROR,
    MEMCPY_ERROR,
    FOPEN_ERROR,
    FCLOSE_ERROR
};

struct Node
{
    enum types type;
    field_t field;

    Node * left;
    Node * right;
};

int CreateNode(Node ** node, enum types type, field_t field);
void DestroyNode(Node * node);


unsigned int NodeColor(Node * node);
const char * NodeType(Node * node);

Node * _node_dump_func(Node * node, FILE * Out);
Node * NodeDump(Node * node, const char * FileName);


int NodeParse(Node ** node, const char * filename);
int _node_parse(Node ** tree, const char ** string);

int FindVar(Node * node);

field_t NodeCount(Node * node, field_t var);
field_t _node_count(Node * node, field_t val, field_t var);

#endif
