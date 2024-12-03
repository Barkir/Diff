#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <assert.h>

#include "buff.h"
#include "diff.h"

typedef struct _node
{
    void * value;
    struct _node * left;
    struct _node * right;

} Node;

struct _tree
{
    Node * root;
    TreeInit init;
    TreeCmp cmp;
    TreeFree free;
};

static int _create_node(Tree * t, Node ** node, const void * pair);
int _tree_parse(Tree* tree, Node ** node, const char ** string);
Tree * _tree_dump_func(Tree * tree, Node ** node, FILE * Out);
Node * _insert_tree(Tree * t, Node ** root, const void * pair);
void _destroy_tree(Tree * t, Node * n);

Node * GetG(const char * string, int * p);
Node * GetE(const char * string, int * p);
Node * GetT(const char * string, int * p);
Node * GetP(const char * string, int * p);
Node * GetN(const char * string, int * p);


field_t _node_count(Node * node, field_t val, field_t var);

unsigned int NodeColor(Node * node);
const char * NodeType(Node * node);

int FindVar(Node * node);

Tree * CreateTree(TreeInit init, TreeCmp cmp, TreeFree free)
{
    Tree * t = (Tree*) malloc(sizeof(Tree));
    if (!t) return NULL;
    *t = (Tree) {NULL, init, cmp, free};
    return t;
}

static int _create_node(Tree * t, Node ** node, const void * pair)
{
    if (!*node)
    {
        if (((*node) = (Node *) malloc(sizeof(Node))) == NULL) return 0;
        **node = (Node) {t->init ? t->init(pair) : (void*) pair, NULL, NULL};
        return 0;
    }

    if (t->cmp(pair, (*node)->value) > 0)
        return _create_node(t, &(*node)->right, pair);
    else
        return _create_node(t, &(*node)->left, pair);

    (*node)->left = (*node)->right = NULL;

    return 0;
}

int CreateNode(Tree * t, const void * pair)
{
    return _create_node(t, &t->root, pair);
}

Node * _insert_tree(Tree * t, Node ** root, const void * pair)
{
    if (!*root)
    {
        if ((*root = (Node*) malloc(sizeof(Node))) == NULL) return NULL;
        (*root)->value = t->init ? t->init(pair) : (void*) pair;
        (*root)->right = NULL;
        (*root)->left = NULL;
        return *root;
    }

    if (t->cmp(pair, (*root)->value) > 0)
        return _insert_tree(t, &(*root)->right, pair);
    else
        return _insert_tree(t, &(*root)->left, pair);
}

int InsertTree(Tree * t, const void * pair)
{
    return !!_insert_tree(t, &t->root, pair);
}


Tree * _tree_dump_func(Tree * tree, Node ** node, FILE * Out)
{

    if (!tree)
        return NULL;

    if (!*node) return NULL;

    unsigned int color = NodeColor(*node);
    field_t field = ((Field*)(*node)->value)->value;

    switch (((Field*)(*node)->value)->type)
    {
        case OPER:  fprintf(Out, "node%p [shape = Mrecord; label = \"{%c}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, (int) field, color);
                    break;

        case VAR:   fprintf(Out, "node%p [shape = Mrecord; label = \"{%c}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, (int) field, color);
                    break;
        case NUM:   fprintf(Out, "node%p [shape = Mrecord; label = \"{%lg}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, field, color);
                    break;

        default:    fprintf(Out, "node%p [shape = Mrecord; label = \"{}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, color);
                    break;

    }

    if ((*node)->left)
        fprintf(Out, "node%p -> node%p\n", *node, (*node)->left);

    if ((*node)->right)
        fprintf(Out, "node%p -> node%p\n", *node, (*node)->right);

    _tree_dump_func(tree, &(*node)->left, Out);
    _tree_dump_func(tree, &(*node)->right, Out);

    return tree;
}

Tree * TreeDump(Tree * tree, const char * FileName)
{
    FILE * Out = fopen(FileName, "wb");

    fprintf(Out, "digraph\n{\n");
    _tree_dump_func(tree, &tree->root, Out);
    fprintf(Out, "}\n");

    char command[DEF_SIZE] = "";
    sprintf(command, "dot %s -T png -o %s.png", FileName, FileName);

    fclose(Out);

    system(command);

    return tree;
}

#define PRINT(...)                                                               \
    {                                                                            \
    fprintf(stderr, ">>> %s:%d: [%d]%*s", __FILE__, __LINE__, deep, deep*4, ""); \
    fprintf(stderr, __VA_ARGS__);                                                \
    fprintf(stderr, " Cur = '%.20s'...\n", *string);                             \
    }

int _tree_parse(Tree* tree, Node ** node, const char ** string)
{
    static int deep = 0;
    deep++;

    PRINT ("Starting...");

    fprintf(stderr, "\n______________\n");

    Field field = {};

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
        field.value = strtod(ptr, &end);
        if (!end)
        {
            deep--;
            return -1;
        }
        (*string) = end;


        field.type = NUM;

        PRINT ("Got field = %lg. Type = NUM. ptr = '%.20s'", field.value, ptr);
    }
    else if (isalpha(*ptr))
    {
        PRINT ("ISALPHA state.");

        field.value = (field_t)(*ptr);
        field.type = VAR;

        (*string)++;

        PRINT ("Got field = %lg (%c). Type = VAR. ptr = '%.20s'", field.value, (int) field.value, ptr);
    }
    else if (*ptr == '+' || *ptr == '-' || *ptr == '*' || *ptr == '/' || *ptr == '^')
    {
        PRINT ("OPER state.");

        field.value = (field_t)(*ptr);
        field.type = OPER;

        (*string)++;

        PRINT ("Got field = %lg (%c). Type = OPER. ptr = '%.20s'", field.value, (int) field.value, ptr);
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

    if (_insert_tree(tree, node, &field) == NULL)
        {
        PRINT ("Creating node FAILED. Return -1.");
        deep--;
        return -1;
        }

    PRINT ("Created NODE [%p]: type = %d, field = %lg (%c)", *node, field.type, field.value, (int) field.value);

    PRINT ("Skipping WSs before possible left subtree");
    while (isspace(**string)) (*string)++;
    PRINT ("WSs SKIPPED. Testing for '(' -- possible left subtree.")

    if (**string == '(')
        {
        PRINT ("...FOUND '('. Did NOT skipped '('. Calling LEFT subtree read.");
        _tree_parse(tree, &(*node)->left, string);
        }

    PRINT ("Now searching for WSs...");
    while (isspace(**string)) (*string)++;
    PRINT ("SKIPPED WSs. Checking for '(' -- possible right subtree...");

    if (**string == '(')
        {
        PRINT ("...FOUND '('. Did NOT skipped '('. Calling RIGHT subtree read.");
        _tree_parse (tree, &(*node)->right, string);
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

int TreeParse(Tree * tree, const char * filename)
{
    FILE * file = fopen(filename, "rb");
    if (ferror(file)) return FOPEN_ERROR;

    char * expression = CreateBuf(file);
    if (!expression) return ALLOCATE_MEMORY_ERROR;

    const char * ptr = expression;
    int pointer = 0;
    tree->root = GetG(ptr, &pointer);

    if (fclose(file) == EOF) return FCLOSE_ERROR;
    free(expression);

    return 1;
}

field_t _node_count(Node * node, field_t val, field_t var)
{
    enum types type = ((Field*)node->value)->type;
    field_t field = ((Field*) node->value)->value;
    if (type == NUM) return ((Field*)node->value)->value;
    if (type == VAR) return var;

    if (type == OPER)
    {
        switch((int) field)
        {
            case '+': val = _node_count(node->left, val, var) + _node_count(node->right, val, var); break;
            case '-': val = _node_count(node->left, val, var) - _node_count(node->right, val, var); break;
            case '*': val = _node_count(node->left, val, var) * _node_count(node->right, val, var); break;
            case '/': val = _node_count(node->left, val, var) / _node_count(node->right, val, var); break;
            case '^': val = pow(_node_count(node->left, val, var), _node_count(node->right, val, var)); break;
            default: return 0;
        }
    }

    return val;
}

field_t CountTree(Tree * tree)
{
    return _node_count(tree->root, 0, 0);
}

#define DESTROY(...)                                                             \
    {                                                                            \
    fprintf(stderr, ">>> %s:%d: ", __FILE__, __LINE__);                          \
    fprintf(stderr, __VA_ARGS__);                                                \
    fprintf(stderr, "\n");                                                       \
    }

void _destroy_tree(Tree * t, Node * n)
{
    if (!n) return;

    DESTROY("SUBTREE %p value = %lg. Destroying.", n, ((Field*) n->value)->value);

    _destroy_tree(t, n->left);
    _destroy_tree(t, n->right);

    if (t->free) t->free(n->value);
    DESTROY("SUBTREE %p. Destroyed.", n);
    free(n);
}

void DestroyTree(Tree * t)
{
    DESTROY("STARTED TREE DESTROY");
    _destroy_tree(t, t->root);
    free(t);

}

unsigned int NodeColor(Node * node)
{
    unsigned int color = 0;
        switch (((Field*) (node->value))->type)
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
    const char * stype = 0;
    enum types type = ((Field*)node->value)->type;

        switch (type)
    {
        case OPER:
            stype = "Operation";
            break;

        case VAR:
            stype = "Variable";
            break;

        case NUM:
            stype = "Number";
            break;

        default:
            break;
    }

    return stype;
}


int FindVar(Node * node)
{
    if (!node) return 0;
    if (((Field*)(node->value))->type == VAR) return 1;

    int result = FindVar(node->left);
    if (!result) result = FindVar(node->right);
    return result;
}


#define SYNTAX_ERROR(exp, real)                     \
    {                                               \
        SyntaxError(exp, real, __func__, __LINE__); \
    }                                               \

#define PARSER(...)                                         \
    {                                                       \
        fprintf(stderr, ">>> %s %d: ", __func__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                       \
        fprintf(stderr, "\n");                              \
    }                                                       \

Field * _create_field(field_t val, enum types type)
{
    Field * field = (Field*) calloc(1, sizeof(Field));
    if (!field) return NULL;
    field->value = val;
    field->type = type;
    return field;
}

Node * _create_node(Field * val, Node * left, Node * right)
{
    Node * node = (Node*) calloc(1, sizeof(Node));
    if (!node) return NULL;
    node->value = val;

    if (left) node->left = left;
    if (right) node->right = right;
    return node;
}

int SyntaxError(char exp, char real, const char * func, int line)
{
    fprintf(stderr, ">>> SyntaxError: <expected %c> <got %c>", exp, real);
    assert(0);
}

Node * GetG(const char * string, int * p)
{
    PARSER("Got string %s", string + *p);
    Node * result = GetE(string, p);
    if (string[*p] != '$') SYNTAX_ERROR('$', string[*p]);
    (*p)++;
    return result;
}

Node * GetE(const char * string, int * p)
{
    PARSER("Got string %s", string + (*p));
    Node * val1 = GetT(string, p);

    while (string[(*p)] == '+' || string[(*p)] == '-')
    {
        PARSER("Got string %s", string + (*p));
        int op = string[(*p)];

        Field * operation = NULL;
        if (!(operation = _create_field((field_t) op, OPER))) return NULL;

        (*p)++;

        Node * val2 = GetT(string, p);

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    return val1;
}

Node * GetT(const char * string, int * p)
{
    PARSER("Got string %s", string + (*p));
    Node * val1 = GetP(string, p);

    while (string[(*p)] == '*' || string[(*p)] == '/')
    {
        int op = string[(*p)];

        Field * operation = NULL;
        if (!(operation = _create_field((field_t) op, OPER))) return NULL;

        (*p)++;

        Node * val2 = GetP(string, p);

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    return val1;
}

Node * GetP(const char * string, int * p)
{
    PARSER("string = %s. Getting P...", string + (*p));
    if (string[(*p)] == '(')
    {
        (*p)++;
        PARSER("Got '('");
        Node * val = GetE(string, p);
        PARSER("Got string %s", string + (*p));
        if (string[(*p)] != ')') SYNTAX_ERROR(')', string[(*p)]);
        (*p)++;
        return val;
    }
    else return GetN(string, p);
}

Node * GetN(const char * string, int * p)
{
    PARSER("Got string %s", string + (*p));

    const char * old_string = string + (*p);

    Field * number = NULL;
    if (!(number = _create_field(0, NUM))) return NULL;

    while (string[(*p)] >= '0' && string[(*p)] <= '9')
    {
        number->value = number->value * 10 + string[(*p)] - '0';
        (*p)++;
    }

    PARSER("Got number %lg", number->value);

    if ((string + (*p)) == old_string) SYNTAX_ERROR((*old_string), *string);

    Node * result = NULL;
    if (!(result = _create_node(number, NULL, NULL))) return NULL;
    return result;
}
