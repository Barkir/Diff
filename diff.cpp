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

int SyntaxError(char exp, char real, const char * func, int line);

Node * GetG(const char * string, int * p);
Node * GetE(const char * string, int * p);
Node * GetT(const char * string, int * p);
Node * GetPow(const char * string, int * p);
Node * GetP(const char * string, int * p);
Node * GetN(const char * string, int * p);
Node * GetX(const char * string, int * p);

void * DiffConst(void * node);
void * DiffX(void * node);
void * DiffPlus(void * node);
void * DiffMul(void * node);
void * DiffDiv(void * node);
void * DiffPow(void * node);

Node * _copy_branch(Node * node);
Node * _copy_node(Node * node);
Field * _copy_field(Field * field);
Node * _create_node(Field * val, Node * left, Node * right);
Field * _create_field(field_t val, enum types type, Diff diff);

Node * _diff_tree(Node * node);

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

char * _tex_dump_func(Tree * tree, Node ** node)
{
    enum types type = ((Field*)((*node)->value))->type;
    field_t value = ((Field*)((*node)->value))->value;
    char * oper = NULL;
    char * number = NULL;
    char * variable = NULL;
    char * left = NULL;
    char * right = NULL;

    switch((int) type)
    {
        case NUM:
            number = (char*) calloc(DEF_SIZE, 1);
            sprintf(number, "%lg", value);
            return number;

        case VAR:
            variable = (char*) calloc(DEF_SIZE, 1);
            sprintf(variable, "%c", (int) value);
            return variable;

        case OPER:

            left = _tex_dump_func(tree, &(*node)->left);
            right = _tex_dump_func(tree, &(*node)->right);
            char * oper = (char*) calloc(DEF_SIZE + strlen(left) + strlen(right), 1);

            switch((int) value)
            {
                case '+':   sprintf(oper, "{%s} + {%s}", left, right);
                            free(left);
                            free(right);
                            return oper;

                case '-':   sprintf(oper, "{%s} - {%s}", left, right);
                            free(left);
                            free(right);
                            return oper;

                case '*':   sprintf(oper, "{%s} \\cdot {%s}", left, right);
                            free(left);
                            free(right);
                            return oper;

                case '^':   sprintf(oper, "{%s}^{%s}", left, right);
                            free(left);
                            free(right);
                            return oper;

                case '/':   sprintf(oper, "\\frac{%s}{%s}", left, right);
                            free(left);
                            free(right);
                            return oper;

                default:    return NULL;
            }
    }

    return oper;
}

Tree * TexDump(Tree * tree, const char * filename)
{
    FILE * Out = fopen(filename, "wb");

    fprintf(Out, "\n\\[");
    char * expression = _tex_dump_func(tree, &tree->root);
    fprintf(Out, expression);
    free(expression);
    fprintf(Out, "\\]\n");

    fclose(Out);

    return tree;
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

    DESTROY("SUBTREE %p value = %lg (%c) %p. Destroying.", n, ((Field*) n->value)->value, (int)((Field*) n->value)->value, &((Field*) n->value)->value);

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


// PARSER

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

Field * _create_field(field_t val, enum types type, Diff diff)
{
    Field * field = (Field*) calloc(1, sizeof(Field));
    if (!field) return NULL;

    field->value = val;
    field->type = type;
    field->diff = diff;

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

Field * _copy_field(Field * field)
{
    if (!field) return NULL;
    Field * copy_field = (Field*) calloc(1, sizeof(Field));
    if (!copy_field) return NULL;

    copy_field->value = field->value;
    copy_field->type = field->type;
    copy_field->diff = field->diff;

    return copy_field;
}

Node * _copy_node(Node * node)
{
    if (!node) return NULL;
    PARSER("Copying node %p with value %lg...", node, ((Field*)node->value)->value);
    Field * copy_field = _copy_field((Field*)node->value);
    if (!copy_field) return NULL;
    Node * copy_node = (Node*) calloc(1, sizeof(Node));
    if (!copy_node) return NULL;

    copy_node->value = copy_field;
    PARSER("Created node with value %lg", copy_field->value);
    if (node->left)  copy_node->left = node->left;
    if (node->right) copy_node->right = node->right;

    return copy_node;
}

Node * _copy_branch(Node * node)
{
    if (!node) return NULL;
    PARSER("Copying branch %p with value %lg...", node, ((Field*)node->value)->value);
    Node * result = _copy_node(node);
    if (!result) return NULL;

    result->left = _copy_branch(node->left);
    result->right = _copy_branch(node->right);

    return result;
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
        if (!(operation = _create_field((field_t) op, OPER, DiffPlus))) return NULL;

        (*p)++;

        Node * val2 = GetT(string, p);

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    return val1;
}

Node * GetT(const char * string, int * p)
{
    PARSER("Got string %s", string + (*p));
    Node * val1 = GetPow(string, p);

    while (string[(*p)] == '*' || string[(*p)] == '/')
    {
        int op = string[(*p)];

        Field * operation = NULL;

        if (string[(*p)] == '*')
            operation = _create_field((field_t) op, OPER, DiffMul);
        else if (string[(*p)] == '/')
            operation = _create_field((field_t) op, OPER, DiffDiv);

        if (!operation) return NULL;


        (*p)++;

        Node * val2 = GetPow(string, p);

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    return val1;
}

Node * GetPow(const char * string, int * p)
{
    PARSER("Got string %s", string + (*p));
    Node * val1 = GetP(string, p);

    while (string[(*p)] == '^')
    {
        int op = string[(*p)];

        Field * operation = NULL;
                if (!(operation = _create_field((field_t) op, OPER, DiffPow))) return NULL;

        (*p)++;

        Node * val2 = GetP(string, p);

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    return val1;
}

Node * GetP(const char * string, int * p)
{
    PARSER("string = %s. Getting P...", string + (*p));
    if (string[*p] == '(')
    {
        (*p)++;
        PARSER("Got '('");
        Node * val = GetE(string, p);
        PARSER("Got string %s", string + (*p));
        if (string[(*p)] != ')') SYNTAX_ERROR(')', string[(*p)]);
        (*p)++;
        return val;
    }
    else if (string[(*p)] >= 'A' && string[(*p)] <= 'z') return GetX(string, p);
    else return GetN(string, p);
}

Node * GetN(const char * string, int * p)
{
    PARSER("Got string %s", string + (*p));

    const char * old_string = string + (*p);

    Field * number = NULL;
    if (!(number = _create_field(0, NUM, DiffConst))) return NULL;

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

Node * GetX(const char * string, int * p)
{
    PARSER("Got string %s", string + (*p));

    const char * old_string = string + (*p);

    Field * var = NULL;
    if (!(var = _create_field(0, VAR, DiffX))) return NULL;

    if (string[*p] >= 'A' && string[*p] <= 'z')
    {
        var->value = string[*p];
        (*p)++;
    }

    if (string + (*p) == old_string) SYNTAX_ERROR(string[(*p)], *old_string);

    Node * result = NULL;
    if (!(result = _create_node(var, NULL, NULL))) return NULL;
    return result;
}


// Differentiate Functions

Node * _diff_tree(Node * node)
{
    PARSER("Calling subfunction...");
    PARSER("left value = %lg, right value = %lg", ((Field*)node->left->value)->value, ((Field*)(node->right->value))->value);
    Node * result = (Node*)((Field*)(node->value))->diff(node);
    return result;
}

Tree * DiffTree(Tree * tree)
{
    if (!tree) return NULL;
    if (!tree->root) return NULL;

    PARSER("\n<<<DIFFERENTIATING TREE START>>>\n");

    Tree * new_tree = CreateTree(tree->init, tree->cmp, tree->free);
    PARSER("Created new tree %p...", new_tree);
    if (!new_tree) return NULL;

    new_tree->root = _diff_tree(tree->root);
    PARSER("Differentiated tree root %p", new_tree->root);
    if (!tree->root) return NULL;

    PARSER("\n<<<DIFFERENTIATING TREE END>>>\n");
    return new_tree;
}

void * DiffConst(void * node)
{
    if (!node) return NULL;
    PARSER("<DIFFERENTIATING CONSTANT %lg.>", ((Field*)((Node*)node)->value)->value);

    Field * field = _create_field(0, NUM, DiffConst);
    if (!field) return NULL;

    PARSER("Created field %p...", field);

    Node * result = _create_node(field, NULL, NULL);
    if (!result) return NULL;

    PARSER("Created node %p...", result);

    PARSER("<DIFFERENTIATING CONSTANT ENDED.>");
    return (void*) result;
}

void * DiffX(void * node)
{
    if (!node) return NULL;
    PARSER("<DIFFERENTIATING VARIABLE %c %p.>", (int)((Field*)((Node*)node)->value)->value, node);

    Field * field = _create_field(1, NUM, DiffConst);
    if (!field) return NULL;

    PARSER("Created field %p...", field);

    Node * result = _create_node(field, NULL, NULL);
    if (!result) return NULL;

    PARSER("Created node %p...", result);

    PARSER("<DIFFERENTIATING VARIABLE ENDED.>");
    return (void*)result;
}

void * DiffPlus(void * node)
{
    if (!node) return NULL;
    PARSER("<DIFFERENTIATING %c.>", (int)((Field*)((Node*)node)->value)->value);

    Node * diff_left = (Node*)((Field*)((Node*)node)->left->value)->diff(((Node*)node)->left);
    if (!diff_left) return NULL;
    PARSER("Created differentiated subnode %p...", diff_left);

    Node * diff_right = (Node*)((Field*)((Node*)node)->right->value)->diff(((Node*)node)->right);
    if (!diff_right) return NULL;
    PARSER("Created differentiated subnode %p...", diff_right);

    Node * plus = _copy_node((Node*)node);
    PARSER("Created node plus %p", plus);

    plus->left = diff_left;
    plus->right = diff_right;


    PARSER("<DIFFERENTIATING %c ENDED.>", (int)((Field*)((Node*)node)->value)->value);

    return (void*)plus;

}

void * DiffMul(void * node)
{
    // u and v
    Node * left_cp = _copy_branch(((Node*)node)->left);
    if (!left_cp) return NULL;

    Node * right_cp = _copy_branch(((Node*)node)->right);
    if (!right_cp) return NULL;

    // u' and v'

    Node * diff_left = (Node*) ((Field*)(left_cp->value))->diff(left_cp);
    if (!diff_left) return NULL;
    Node * diff_right = (Node*) ((Field*)(right_cp->value))->diff(right_cp);
    if (!diff_right) return NULL;

    // '+' field
    Field * plus = _create_field((field_t) '+', OPER, DiffPlus);
    if (!plus) return NULL;

    // '*' fields
    Field * mul_left = _create_field((field_t) '*', OPER, DiffMul);
    if (!mul_left) return NULL;

    Field * mul_right = _create_field((field_t) '*', OPER, DiffMul);
    if (!mul_right) return NULL;


    // u'v and v'u
    Node * left = _create_node(mul_left, diff_left, right_cp);
    if (!left) return NULL;

    Node * right = _create_node(mul_right, diff_right, left_cp);
    if (!right) return NULL;

    // u'v + v'u
    Node * result = _create_node(plus, left, right);
    if (!result) return NULL;

    return (void*)result;

}

void * DiffDiv(void * node)
{
    if (!node) return NULL;

    PARSER("<DIFFERENTIATING %c %p.>", (int)((Field*)(((Node*)node)->value))->value, node);

    // u and v
    if (!((Node*)node)->left) return NULL;
    Node * left_cp = _copy_branch(((Node*)node)->left);
    if (!left_cp) return NULL;
    PARSER("Created copy of left node %p: %p", ((Node*)node)->left, left_cp)

    if (!((Node*)node)->right) return NULL;
    Node * right_cp = _copy_branch(((Node*)node)->right);
    if (!right_cp) return NULL;
    PARSER("Created copy of right node %p: %p", ((Node*)node)->right, right_cp)

    PARSER("Differentiating node %p...", left_cp);
    Node * diff_left = (Node*)((Field*)(left_cp->value))->diff(left_cp);
    if (!diff_left) return NULL;
    PARSER("Differentiated node %p: %p", left_cp, diff_left);

    PARSER("Differentiating node %p...", right_cp);
    Node * diff_right = (Node*)((Field*)(right_cp->value))->diff(right_cp);
    if (!diff_right) return NULL;
    PARSER("Differentiated node %p: %p", right_cp, diff_right);

    // '-' field and two '*' fields
    Field * minus = _create_field((field_t) '-', OPER, DiffPlus);
    if (!minus) return NULL;
    PARSER("Created '-' field %p", minus);

    Field * mul_left = _create_field((field_t) '*', OPER, DiffMul);
    if (!mul_left) return NULL;
    PARSER("Created left '*' field %p", mul_left);

    Field * mul_right = _create_field((field_t) '*', OPER, DiffMul);
    if (!mul_right) return NULL;
    PARSER("Created right '*' field %p", mul_right);

    // u'v and v'u
    Node * left = _create_node(mul_left, diff_left, right_cp);
    if (!left) return NULL;
    PARSER("Created left node %p with '*' field and left:%p, right:%p", left, mul_left, diff_left, right_cp);

    Node * right = _create_node(mul_right, diff_right, left_cp);
    if (!right) return NULL;
    PARSER("Created left node %p with '*' field and left:%p, right:%p", right, mul_right, diff_right, left_cp);

    // u'v - v'u
    Node * higher = _create_node(minus, left, right);
    if (!higher) return NULL;
    PARSER("Created node %p with '-' field and left:%p. right:%p", higher, minus, left, right);


    // v and field of number 2
    Node * lower_right_cp = _copy_branch(((Node*)node)->right);
    if (!lower_right_cp) return NULL;
    PARSER("Created lower copy of right node %p: %p", ((Node*)node)->right, lower_right_cp)

    Field * two = _create_field(2, NUM, DiffConst);
    if (!two) return NULL;
    PARSER("Created field %p with number 2", two);

    // '^' field and v^2
    Field * pw = _create_field((field_t) '^', OPER, DiffPow);
    if (!pw) return NULL;
    PARSER("Created field %p with '^'", pw);

    Node * twof = _create_node(two, NULL, NULL);
    if (!twof) return NULL;
    PARSER("Created node %p with number 2", twof);
    Node * lower = _create_node(pw, lower_right_cp, twof);
    PARSER("Created node %p with field '^' and left:%p, right:%p", lower, lower_right_cp, twof);
    if (!lower) return NULL;

    // '/' field and (u'v - v'u) / (v^2)
    Field * div = _create_field((field_t) '/', OPER, DiffDiv);
    if (!div) return NULL;
    PARSER("Created field %p with '/'", div);

    Node * result = _create_node(div, higher, lower);
    if (!result) return NULL;

    PARSER("Created node %p with field '/' and left:%p, right:%p", result, higher, lower);

    PARSER("<DIFFERENTIATING %c ENDED.>", (int)((Field*)((Node*)node)->value)->value);
    return (void*) result;
}

void * DiffPow(void * node)
{
    PARSER("<DIFFERENTIATING %c, func = %p.>", (int)((Field*)((Node*)node)->value)->value, ((Field*)((Node*)node)->value)->diff);
    PARSER("Copying node %p with value %lg and field %p...", node, ((Field*)((Node*)node)->right->value)->value, ((Node*)node)->right->value);

    Field * n1 = _copy_field((Field*)((Node*)node)->right->value);
    if (!n1) return NULL;


    PARSER("Copying node %p with value %lg and field %p...", node, ((Field*)((Node*)node)->right->value)->value, ((Node*)node)->right->value);
    Field * n2 = _copy_field((Field*)((Node*)node)->right->value);
    if (!n2) return NULL;
    n2->value -= 1;

    PARSER("Copying node %p...", ((Node*)node)->left);
    Node * x_node = _copy_node(((Node*)node)->left);
    if (!x_node) return NULL;
    PARSER("Copyied node %p: %p", ((Node*)node)->left, x_node);

    Field * mul = _create_field((field_t) '*', OPER, DiffMul);
    if (!mul) return NULL;

    Field * pow = _create_field((field_t) '^', OPER, DiffPow);
    if (!pow) return NULL;

    Node * n1_node = _create_node(n1, NULL, NULL);
    if (!n1_node) return NULL;

    Node * n2_node = _create_node(n2, NULL, NULL);
    if (!n2_node) return NULL;

    Node * pow_node = _create_node(pow, x_node, n2_node);
    if (!pow_node) return NULL;

    Node * mul_node = _create_node(mul, n1_node, pow_node);
    if (!mul_node) return NULL;

    Node * x_diff = (Node*)((Field*)((x_node)->value))->diff(x_node);
    Node * result = _copy_node(mul_node);
    result->left = mul_node;
    result->right = x_diff;

    return (void*)result;
}


