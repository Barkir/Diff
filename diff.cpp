#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>

#include "buff.h"
#include "diff.h"

#define DEBUG

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

const char * enum_to_name(int name);

int SyntaxError(char exp, char real, const char * func, int line);
Node ** StringTokenize(const char * string, int * p);

Node * GetG(Node ** nodes, int * p);
Node * GetE(Node ** nodes, int * p);
Node * GetT(Node ** nodes, int * p);
Node * GetPow(Node ** nodes, int * p);
Node * GetP(Node ** nodes, int * p);
Node * GetFunc(Node ** nodes, int * p);
Node * GetN(Node ** nodes, int * p);
Node * GetX(Node ** nodes, int * p);

void * DiffCONST(void * node);
void * DiffX(void * node);
void * DiffPLUS(void * node);
void * DiffMUL(void * node);
void * DiffDIV(void * node);
void * DiffPOW(void * node);
void * DiffSIN(void * node);
void * DiffCOS(void * node);
void * DiffTG(void * node);
void * DiffCTG(void * node);
void * DiffSH(void * node);
void * DiffCH(void * node);
void * DiffTH(void * node);
void * DiffCTH(void * node);
void * DiffEX(void * node);
void * DiffAX(void * node);
void * DiffLN(void * node);
void * DiffLOG(void * node);
void * DiffHARDPOW(void * node);
// void * DiffARCSIN(void * node);
// void * DiffARCCOS(void * node);
// void * DiffARCTG(void * node);
// void * DiffARCCTG(void * node);

static Field NameTable[DEF_SIZE]
{
    {.type = FUNC,      .value = SIN,       .diff = DiffSIN},
    {.type = FUNC,      .value = COS,       .diff = DiffCOS},
    {.type = FUNC,      .value = TG,        .diff = DiffTG},
    {.type = FUNC,      .value = CTG,       .diff = DiffCTG},
    {.type = FUNC,      .value = SH,        .diff = DiffSH},
    {.type = FUNC,      .value = CH,        .diff = DiffCH},
    {.type = FUNC,      .value = TH,        .diff = DiffTH},
    {.type = FUNC,      .value = CTH,       .diff = DiffCTH},
    {.type = FUNC,      .value = EX,        .diff = DiffEX},
    {.type = FUNC,      .value = AX,        .diff = DiffAX},
    {.type = FUNC,      .value = LN,        .diff = DiffLN},
    {.type = FUNC,      .value = LOG,       .diff = DiffLOG},
    // {.type = FUNC,      .value = ARCSIN,    .diff = DiffARCSIN},
    // {.type = FUNC,      .value = ARCCOS,    .diff = DiffARCCOS},
    // {.type = FUNC,      .value = ARCTG,     .diff = DiffARCTG},
    // {.type = FUNC,      .value = ARCCTG,    .diff = DiffARCCTG}
};

Node * _copy_branch(Node * node);
Node * _copy_node(Node * node);
Field * _copy_field(Field * field);
Node * _create_node(Field * val, Node * left, Node * right);
Field * _create_field(field_t val, enum types type, Diff diff);

Node * _diff_tree(Node * node);

field_t _node_count(Node * node, field_t val, field_t var);

unsigned int NodeColor(Node * node);
field_t NodeValue(Node * node);
enum types NodeType(Node * node);
Diff NodeDiff(Node * node);

#ifdef DEBUG
#define DESTROY(...)                                                             \
    {                                                                            \
    fprintf(stderr, ">>> %s:%d: ", __func__, __LINE__);                          \
    fprintf(stderr, __VA_ARGS__);                                                \
    fprintf(stderr, "\n");                                                       \
    }
#else
#define DESTROY(...)
#endif


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
    field_t field = NodeValue(*node);

    switch (((Field*)(*node)->value)->type)
    {
        case OPER:  fprintf(Out, "node%p [shape = Mrecord; label = \"{%c | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, (int) field, *node, color);
                    break;

        case VAR:   fprintf(Out, "node%p [shape = Mrecord; label = \"{%c | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, (int) field, *node, color);
                    break;

        case NUM:   fprintf(Out, "node%p [shape = Mrecord; label = \"{%lg | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, *node, field, color);
                    break;

        case FUNC:  fprintf(Out, "node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, enum_to_name((int) field), *node, color);
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
    enum types type = NodeType(*node);
    field_t value = NodeValue(*node);
    char * oper = NULL;
    char * number = NULL;
    char * variable = NULL;
    char * func = NULL;
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

        case FUNC:
        {
            left = _tex_dump_func(tree, &(*node)->left);
            char * func = (char*) calloc(DEF_SIZE + strlen(left), 1);
            DESTROY("FUNC = %d(%c), %s", (int)NodeValue((*node)->left), (int)NodeValue((*node)), enum_to_name((int)NodeValue((*node))));
            sprintf(func, "%s(%s)", enum_to_name((int)NodeValue((*node))), left);
            free(left);
            return func;
        }

        case OPER:

            left = _tex_dump_func(tree, &(*node)->left);
            right = _tex_dump_func(tree, &(*node)->right);
            char * oper = (char*) calloc(DEF_SIZE + strlen(left) + strlen(right), 1);

            switch((int) value)
            {
                case '+':   sprintf(oper, "({%s} + {%s})", left, right);
                            free(left);
                            free(right);
                            return oper;

                case '-':   sprintf(oper, "({%s} - {%s})", left, right);
                            free(left);
                            free(right);
                            return oper;

                case '*':   sprintf(oper, "({%s} \\cdot {%s})", left, right);
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

    fprintf(Out,    "\\documentclass[12]{article}\n"
                    "\\usepackage{amsmath}\n"
                    "\\usepackage{amssymb}\n"
                    "\\usepackage{graphicx} % Required for inserting images\n"
                    "\\usepackage[utf8]{inputenc}\n"
                    "\\usepackage[russian]{babel}\n"
                    "\\begin{document}\n"
                    "\\begin{small}\n");

    fprintf(Out, "\n\\[");
    char * expression = _tex_dump_func(tree, &tree->root);
    fprintf(Out, expression);

    fprintf(Out, "\\]\n");
    fprintf(Out,    "\\end{small}\n"
                    "\\end{document}\n");

    fclose(Out);

    char command[DEF_SIZE] = "";
    sprintf(command, "pdflatex --output-directory=./tmp %s", filename);

    system(command);

    free(expression);
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

    Node ** array = StringTokenize(ptr, &pointer);

    printf("%p\n", (*array));

    pointer = 0;
    tree->root = GetG(array, &pointer);

    pointer = 0;
    while (array[pointer])
    {
        free(array[pointer]->value);
        free(array[pointer]);
        pointer++;
    }

    free(array);

    if (fclose(file) == EOF) return FCLOSE_ERROR;
    free(expression);

    return 1;
}

field_t _node_count(Node * node, field_t val)
{
    enum types type = NodeType(node);
    field_t field = NodeValue(node);
    if (type == NUM) return field;

    if (type == OPER)
    {
        switch((int) field)
        {
            case '+': val = _node_count(node->left, val) + _node_count(node->right, val); break;
            case '-': val = _node_count(node->left, val) - _node_count(node->right, val); break;
            case '*': val = _node_count(node->left, val) * _node_count(node->right, val); break;
            case '/': val = _node_count(node->left, val) / _node_count(node->right, val); break;
            case '^': val = pow(_node_count(node->left, val), _node_count(node->right, val)); break;
            default: return 0;
        }
    }

    return val;
}

field_t CountTree(Tree * tree)
{
    return _node_count(tree->root, 0);
}

void _destroy_tree(Tree * t, Node * n)
{
    if (!n) return;

    DESTROY("SUBTREE %p value = %lg (%c) %p. Destroying.", n, NodeValue(n), (int) NodeValue(n), ((Field*) n->value));

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

field_t NodeValue(Node * node)
{
    if (!node) return NULL;
    DESTROY("Getting node value %p %lg", node, ((Field*)(node->value))->value);
    return ((Field*)(node->value))->value;
}

enum types NodeType(Node * node)
{
    if (!node) return ERROR;
    return ((Field*)(node->value))->type;
}

Diff NodeDiff(Node * node)
{
    if (!node) return NULL;
    return ((Field*)(node->value))->diff;
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

        case FUNC:
            color = FUNC_COLOR;
            break;

        default:
            break;
        }

    return color;
}

int FindVar(Node * node)
{
    if (!node) return 0;
    if (NodeType(node) == VAR) return VAR;


    int result = FindVar(node->left);
    if (!result) result = FindVar(node->right);
    return result;
}

int _need_to_simplify(Node ** node)
{
    // if (FindVar(*node) != VAR) return 1;
    if ((int)NodeValue((*node)) == '*' && (NodeValue((*node)->left) == 1) && NodeType((*node)->left) == NUM)  return 1;
    if ((int)NodeValue((*node)) == '*' && (NodeValue((*node)->left) == 0) && NodeType((*node)->left) == NUM)  return 1;
    if ((int)NodeValue((*node)) == '*' && (NodeValue((*node)->right) == 0) && NodeType((*node)->right) == NUM) return 1;
    if ((int)NodeValue((*node)) == '*' && (NodeValue((*node)->right) == 1) && NodeType((*node)->right) == NUM) return 1;
    return 0;
}

int _tree_simplify(Tree * tree, Node ** node)
{
    if (!tree) return -1;
    if (!(*node)) return -1;

    if (FindVar(*node) != VAR)
    {
        field_t count = _node_count((*node), 0);
        _destroy_tree(tree, (*node));
        Field * field = _create_field(count, NUM, DiffCONST);
        if (!field) return -1;
        Node * new_node = _create_node(field, NULL, NULL);
        if (!new_node) return -1;
        *node = new_node;
    }

    if ((int)NodeValue((*node)) == '*' && NodeValue((*node)->left) == 1 && NodeType((*node)->left) == NUM)
    {
        Node * new_node = _copy_branch((*node)->right);
        if (!new_node) return -1;
        _destroy_tree(tree, (*node));
        *node = new_node;
    }
    if ((int)NodeValue((*node)) == '*' && NodeValue((*node)->left) == 0 && NodeType((*node)->left) == NUM)
    {
        Node * new_node = _copy_node((*node)->left);
        if (!new_node) return -1;
        _destroy_tree(tree, (*node));
        *node = new_node;
    }

    if ((int)NodeValue((*node)) == '*' && NodeValue((*node)->right) == 0 && NodeType((*node)->right) == NUM)
    {
        Node * new_node = _copy_node((*node)->right);
        if (!new_node) return -1;
        _destroy_tree(tree, (*node));
        *node = new_node;
    }

    if ((int)NodeValue((*node)) == '*' && NodeValue((*node)->right) == 1 && NodeType((*node)->right) == NUM)
    {
        Node * new_node = _copy_branch((*node)->left);
        if (!new_node) return -1;
        _destroy_tree(tree, (*node));
        *node = new_node;
    }
    if ((int)NodeValue((*node)) == '/' && NodeValue((*node)->left) == 0 && NodeType((*node)->left) == NUM)
        {
            Node * new_node = _copy_node((*node)->left);
            if (!new_node) return -1;
            _destroy_tree(tree, (*node));
            *node = new_node;
        }

    if ((int)NodeValue((*node)) == '/' && NodeValue((*node)->right) == 0 && NodeType((*node)->right) == NUM)
        return -1;


    if ((int)NodeValue((*node)) == '/' && NodeValue((*node)->right) == 1 && NodeType((*node)->right) == NUM)
        {
            Node * new_node = _copy_branch((*node)->left);
            if (!new_node) return -1;
            _destroy_tree(tree, (*node));
            *node = new_node;
        }

        if ((int)NodeValue((*node)) == '+' && NodeValue((*node)->left) == 0 && NodeType((*node)->left) == NUM)
        {
            Node * new_node = _copy_branch((*node)->right);
            if (!new_node) return -1;
            _destroy_tree(tree, (*node));
            *node = new_node;
        }

        if ((int)NodeValue((*node)) == '+' && NodeValue((*node)->right) == 0 && NodeType((*node)->right) == NUM)
        {
            Node * new_node = _copy_branch((*node)->left);
            if (!new_node) return -1;
            _destroy_tree(tree, (*node));
            *node = new_node;
        }

        if ((int)NodeValue((*node)) == '-' && NodeValue((*node)->right) == 0 && NodeType((*node)->right) == NUM)
        {
            Node * new_node = _copy_branch((*node)->left);
            if (!new_node) return -1;
            _destroy_tree(tree, (*node));
            *node = new_node;
        }

    if (_need_to_simplify(&(*node))) _tree_simplify(tree, &(*node));
    if ((*node)->left && ((Field*)((*node)->left->value))->type == OPER) _tree_simplify(tree, &(*node)->left);
    if ((*node)->right && ((Field*)((*node)->right->value))->type == OPER) _tree_simplify(tree, &(*node)->right);

    return 1;
}

int TreeSimplify(Tree * tree)
{
    return _tree_simplify(tree, &tree->root);
}

// PARSER

#define SYNTAX_ERROR(exp, real)                     \
    {                                               \
        SyntaxError(exp, real, __func__, __LINE__); \
    }                                               \

#ifdef DEBUG
#define PARSER(...)                                                             \
    {                                                                            \
    fprintf(stderr, ">>> %s:%d: ", __func__, __LINE__);                          \
    fprintf(stderr, __VA_ARGS__);                                                \
    fprintf(stderr, "\n");                                                       \
    }
#else
#define PARSER(...)
#endif

#define N_FUNC(NAME) _create_field((field_t)NAME, FUNC, Diff##NAME)

#define N_MUL _create_field((field_t) MUL, OPER, DiffMUL)
#define N_DIV _create_field((field_t) DIV, OPER, DiffDIV)
#define N_ADD _create_field((field_t) ADD, OPER, DiffPLUS)
#define N_SUB _create_field((field_t) SUB, OPER, DiffPLUS)
#define N_POW _create_field((field_t) POW, OPER, DiffPOW)
#define N_E   _create_field((field_t) EX, VAR, DiffAX)

#define DIFF(node, todiff) (Node*)((Field*)(((Node*)node)->value))->diff((todiff))
#define LEFT(node) ((Node*)node)->left
#define RIGHT(node) ((Node*)node)->right

#define NUM_NODE(num) _create_node(_create_field((field_t) num, NUM, DiffCONST), NULL, NULL)

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

#define SKIPSPACE while(isspace(string[*p])) (*p)++;

int SyntaxError(char exp, char real, const char * func, int line)
{
    fprintf(stderr, ">>> SyntaxError: <expected %c> <got %c>", exp, real);
    assert(0);
}


Node * _oper_token(const char * string, int * p)
{
    SKIPSPACE

    Field * field = _create_field((field_t)string[*p], OPER, NULL);
    if  (!field) return NULL;
    Node * result = _create_node(field, NULL, NULL);
    if (!result) return NULL;
    (*p)++;
    return result;
}

Field _name_table(int name)
{
    Field ret = {.type = NUM, .value = -1, .diff = NULL};
    for (int i = 0; i < DEF_SIZE; i++)
        if (NameTable[i].value == (field_t) name) return NameTable[i];
    return ret;
}

int _name_to_enum(char * name)
{
    if (strcmp(name, "sin") == 0)       return SIN;
    if (strcmp(name, "cos") == 0)       return COS;
    if (strcmp(name, "tg") == 0)        return TG;
    if (strcmp(name, "ctg") == 0)       return CTG;
    if (strcmp(name, "e") == 0)         return EX;
    if (strcmp(name, "sh") == 0)        return SH;
    if (strcmp(name, "ch") == 0)        return CH;
    if (strcmp(name, "ln") == 0)        return LN;
    if (strcmp(name, "log") == 0)       return LOG;
    if (strcmp(name, "arcsin") == 0)    return ARCSIN;
    if (strcmp(name, "arccos") == 0)    return ARCCOS;
    if (strcmp(name, "arctg") == 0)     return ARCTG;
    if (strcmp(name, "arcctg") == 0)    return ARCCTG;
    if (strcmp(name, "e") == 0)         return EX;

    return -1;
}

const char * enum_to_name(int name)
{
    if (name == SIN) return "sin";
    if (name == COS) return "cos";
    if (name == SH)  return "sh";
    if (name == CH) return "ch";
    if (name == TG)  return "tg";
    if (name == LOG) return "log";
    if (name == CTG) return "ctg";
    if (name == LN)  return "ln";
    if (name == ARCSIN) return "arcsin";
    if (name == ARCCOS) return "arccos";
    if (name == ARCTG)  return "arctg";
    if (name == ARCCTG) return "arcctg";
    if (name == TH)     return "th";
    if (name == CTH)    return "cth";
    if (name == EX)      return "e";

    return "notfound";
}

Node * _find_name(char * result)
{
    printf("Need to find name %s... ", result);
    Field name = _name_table(_name_to_enum(result));

    Field * field = NULL;
    if (name.value < 0) field = _create_field((field_t) result[0], VAR, DiffX);
    else field = _create_field(name.value, name.type, name.diff);
    if (!field) return NULL;
    Node * node = _create_node(field, NULL, NULL);
    if (!node) return NULL;
    return node;
}

Node * _name_token(const char * string, int * p)
{
    SKIPSPACE
    int start_p = *p;

    while(isalpha(string[*p])) (*p)++;

    char * result = (char*) calloc((*p) - start_p + 1, 1);
    memcpy(result, &string[start_p], (*p) - start_p);

    Node * name = _find_name(result);
    free(result);
    if (!name) return NULL;
    return name;
}

Node * _number_token(const char * string, int * p)
{
    SKIPSPACE
    char * end = NULL;
    field_t number = strtod(&(string[*p]), &end);
    if (!end) return NULL;
    *p += (int)(end - &string[*p]);
    Field * field = _create_field(number, NUM, DiffCONST);
    if (!field) return NULL;
    Node * num = _create_node(field, NULL, NULL);
    if (!num) return NULL;
    return num;
}

Node * _get_token(const char * string, int * p)
{
    SKIPSPACE
    if (string[*p] == '(' ||
        string[*p] == ')' ||
        string[*p] == '+' ||
        string[*p] == '-' ||
        string[*p] == '/' ||
        string[*p] == '*' ||
        string[*p] == '^')
        {printf("operator %c! ", string[*p]); return _oper_token(string, p);}

    if (isalpha(string[*p])) {printf("name %c! ", string[*p]); return _name_token(string, p);}

    if (isdigit(string[*p])) {printf("number %c! ", string[*p]); return _number_token(string, p);}

}

Node ** StringTokenize(const char * string, int * p)
{
    SKIPSPACE
    int size = 0;
    size_t arr_size = DEF_SIZE;
    Node ** nodes = (Node**) calloc(arr_size, sizeof(Node*));
    while (string[*p] != 0)
    {
        *(nodes + size) = _get_token(string, p);
        printf("got node %u %p with value %lg!\n", size+1, *(nodes + size), NodeValue(*nodes));
        size++;
    }
    printf("%p\n");
    return nodes;
}

Node * GetG(Node ** nodes, int * p)
{
    PARSER("Got node %p", nodes[*p]);
    Node * result = GetE(nodes, p);
    PARSER("Got result!");
    if (!nodes[*p]) return result;
    if (nodes[*p]) SYNTAX_ERROR(0, NodeValue(nodes[*p]));
    PARSER("PARSING ENDED");
    (*p)++;
    return result;
}

Node * GetE(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Getting E... Got node %p", nodes[*p]);
    Node * val1 = GetT(nodes, p);
    if (!nodes[*p]) return val1;

    while ((int)NodeValue(nodes[*p]) == '+' || (int)NodeValue(nodes[*p]) == '-')
    {
        PARSER("Got node %p", nodes[*p]);
        int op = (int) NodeValue(nodes[*p]);

        Field * operation = NULL;
        if (!(operation = _create_field((field_t) op, OPER, DiffPLUS))) return NULL;

        (*p)++;
        if (!nodes[*p]) return NULL;

        Node * val2 = GetT(nodes, p);
        PARSER("Got val2");

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    PARSER("GetE Finished");
    return val1;
}

Node * GetT(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Getting T... Got node %p", nodes[*p]);
    PARSER("Getting pow in T val1..."); Node * val1 = GetPow(nodes, p);
    if (!nodes[*p]) {PARSER("GetT Finished!"); return val1;}

    while ((int)NodeValue(nodes[*p]) == '*' || (int)NodeValue(nodes[*p]) == '/')
    {
        int op = (int)NodeValue(nodes[*p]);

        Field * operation = NULL;

        if (op == '*') operation = _create_field((field_t) op, OPER, DiffMUL);
        else if (op == '/') operation = _create_field((field_t) op, OPER, DiffDIV);

        if (!operation) return NULL;


        (*p)++;
        if (!nodes[*p]) return val1;

        PARSER("Getting pow in T val2..."); Node * val2 = GetPow(nodes, p);

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    PARSER("GetT Finished");
    return val1;
}

Node * GetPow(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Getting pow... Got node %p", nodes[*p]);
    Node * val1 = GetP(nodes, p);
    if (!nodes[*p]) {PARSER("GetPow Finished"); return val1;}
    while ((int)NodeValue(nodes[(*p)]) == '^')
    {
        PARSER("Got '^'");
        int op = (int) NodeValue(nodes[(*p)]);

        Field * operation = NULL;
        Diff diff = NULL;
        if (NodeType(val1) == NUM) diff = DiffAX;

        (*p)++;
        if (!nodes[*p]) return val1;

        Node * val2 = GetP(nodes, p);

        if (NodeType(val1) == NUM) diff = DiffAX;
        else if (NodeType(val2) == NUM) diff = DiffPOW;
        else diff = DiffHARDPOW;
        if (!(operation = _create_field((field_t) op, OPER, diff))) return NULL;

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    PARSER("GetPow Finished");
    return val1;
}


Node * GetP(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("node = %p. Getting P...", nodes[*p]);
    if ((int) NodeValue(nodes[*p]) == '(')
    {
        (*p)++;
        PARSER("Got '('");
        Node * val = GetE(nodes, p);
        PARSER("Got node %p", nodes[*p]);
        if (!nodes[*p]) return val;
        if ((int) NodeValue(nodes[(*p)]) != ')') SYNTAX_ERROR(')', (int) NodeValue(nodes[(*p)]));
        PARSER("Got ')'");
        (*p)++;
        return val;
    }

    else if (NodeType(nodes[*p]) == FUNC) return GetFunc(nodes, p);
    else if (NodeType(nodes[*p]) == VAR) return GetX(nodes, p);
    else return GetN(nodes, p);
}

Node * GetFunc(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Got node %p", nodes[*p]);
    Node * result = _copy_node(nodes[*p]);
    (*p)++;

    Node * val = GetP(nodes, p);
    if (!val) return NULL;
    result->left = val;
    return result;
}

Node * GetN(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Got node %p", nodes[*p]);
    Node * result = _copy_node(nodes[*p]);
    (*p)++;
    if (!result) return NULL;
    return result;
}

Node * GetX(Node ** nodes, int * p)
{
    PARSER("Got node %p", nodes[*p]);
    Node * result = _copy_node(nodes[*p]);
    (*p)++;
    if (!result) return NULL;
    return result;
}


// Differentiate Functions

Node * _diff_tree(Node * node)
{
    PARSER("Calling subfunction...");
    // PARSER("left value = %lg, right value = %lg", ((Field*)node->left->value)->value, ((Field*)(node->right->value))->value);
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

void * DiffCONST(void * node)
{
    if (!node) return NULL;
    PARSER("<DIFFERENTIATING CONSTANT %lg.>", ((Field*)((Node*)node)->value)->value);

    Field * field = _create_field(0, NUM, DiffCONST);
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

    Field * field = _create_field(1, NUM, DiffCONST);
    if (!field) return NULL;

    PARSER("Created field %p...", field);

    Node * result = _create_node(field, NULL, NULL);
    if (!result) return NULL;

    PARSER("Created node %p...", result);

    PARSER("<DIFFERENTIATING VARIABLE ENDED.>");
    return (void*)result;
}

void * DiffPLUS(void * node)
{
    if (!node) return NULL;
    PARSER("<DIFFERENTIATING %c.>", (int)((Field*)((Node*)node)->value)->value);

    Node * diff_left = (Node*)((Field*)((Node*)node)->left->value)->diff(((Node*)node)->left);
    if (!diff_left) return NULL;
    PARSER("Created differentiated subnode %p...", diff_left);
    PARSER("%p ", (Node*)((Field*)((Node*)node)->right->value)->diff);
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

void * DiffMUL(void * node)
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
    Field * plus = _create_field((field_t) '+', OPER, DiffPLUS);
    if (!plus) return NULL;

    // '*' fields
    Field * mul_left = _create_field((field_t) '*', OPER, DiffMUL);
    if (!mul_left) return NULL;

    Field * mul_right = _create_field((field_t) '*', OPER, DiffMUL);
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

void * DiffDIV(void * node)
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
    Field * minus = _create_field((field_t) '-', OPER, DiffPLUS);
    if (!minus) return NULL;
    PARSER("Created '-' field %p", minus);

    Field * mul_left = _create_field((field_t) '*', OPER, DiffMUL);
    if (!mul_left) return NULL;
    PARSER("Created left '*' field %p", mul_left);

    Field * mul_right = _create_field((field_t) '*', OPER, DiffMUL);
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

    Field * two = _create_field(2, NUM, DiffCONST);
    if (!two) return NULL;
    PARSER("Created field %p with number 2", two);

    // '^' field and v^2
    Field * pw = _create_field((field_t) '^', OPER, DiffPOW);
    if (!pw) return NULL;
    PARSER("Created field %p with '^'", pw);

    Node * twof = _create_node(two, NULL, NULL);
    if (!twof) return NULL;
    PARSER("Created node %p with number 2", twof);
    Node * lower = _create_node(pw, lower_right_cp, twof);
    PARSER("Created node %p with field '^' and left:%p, right:%p", lower, lower_right_cp, twof);
    if (!lower) return NULL;

    // '/' field and (u'v - v'u) / (v^2)
    Field * div = _create_field((field_t) '/', OPER, DiffDIV);
    if (!div) return NULL;
    PARSER("Created field %p with '/'", div);

    Node * result = _create_node(div, higher, lower);
    if (!result) return NULL;

    PARSER("Created node %p with field '/' and left:%p, right:%p", result, higher, lower);

    PARSER("<DIFFERENTIATING %c ENDED.>", (int)((Field*)((Node*)node)->value)->value);
    return (void*) result;
}

void * DiffPOW(void * node)
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
    Node * x_node = _copy_branch(((Node*)node)->left);
    if (!x_node) return NULL;
    PARSER("Copyied node %p: %p", ((Node*)node)->left, x_node);

    Field * mul = _create_field((field_t) '*', OPER, DiffMUL);
    if (!mul) return NULL;

    Field * pow = _create_field((field_t) '^', OPER, DiffPOW);
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

void * DiffSIN(void * node)
{
    return _create_node((N_MUL),
        _create_node(N_FUNC(COS), _copy_branch(LEFT(node)), NULL),
        (DIFF(LEFT(node), LEFT(node))));
}

void * DiffCOS(void * node)
{
    return _create_node((N_MUL),
            _create_node(N_SUB, NUM_NODE(0),
            _create_node(N_FUNC(SIN), _copy_branch(LEFT(node)), NULL)),
            (DIFF(LEFT(node), LEFT(node))));
}

void * DiffLN(void * node)
{
    return _create_node(N_MUL,
            _create_node(N_DIV, NUM_NODE(1), _copy_branch(LEFT(node))),
            (DIFF(LEFT(node), LEFT(node))));
}

void * DiffEX(void * node)
{
    return _create_node(N_MUL,
            _copy_branch((Node*)node),
            (DIFF(LEFT(node), LEFT(node))));
}

void * DiffTG(void * node)
{
    return _create_node(N_MUL,
                        _create_node(N_DIV, NUM_NODE(1), _create_node(N_POW, _copy_branch(LEFT(node)), NUM_NODE(2))),
                        DIFF(LEFT(node), LEFT(node)));
}

void * DiffCTG(void * node)
{
    return _create_node(N_MUL,
                        (_create_node(N_DIV, _create_node(N_SUB, NUM_NODE(0), NUM_NODE(1)), _create_node(N_POW, _copy_branch(LEFT(node)), NUM_NODE(2)))),
                        DIFF(LEFT(node), LEFT(node)));
}

void * DiffSH(void * node)
{
    return _create_node(N_MUL,
            _create_node(N_FUNC(CH), _copy_branch(LEFT(node)), NULL),
            DIFF(LEFT(node), LEFT(node)));
}

void * DiffCH(void * node)
{
    return _create_node(N_MUL,
            _create_node(N_FUNC(SH), _copy_branch(LEFT(node)), NULL),
            DIFF(LEFT(node), LEFT(node)));
}

void * DiffAX(void * node)
{
    return _create_node(N_MUL,
            _create_node(N_MUL, _create_node(N_FUNC(LN), _copy_branch(LEFT(node)), NULL), _copy_branch((Node*)node)),
            DIFF(RIGHT(node), RIGHT(node)));
}

void * DiffTH(void * node)
{
    return _create_node(N_MUL,
                _create_node(N_DIV, NUM_NODE(1), _create_node(N_POW, _create_node(N_FUNC(CH), LEFT(node), NULL), NUM_NODE(2))),
                DIFF(LEFT(node), LEFT(node)));
}

void * DiffCTH(void * node)
{
    return _create_node(N_MUL,
                _create_node(N_DIV, NUM_NODE(1), _create_node(N_POW, _create_node(N_FUNC(SH), LEFT(node), NULL), NUM_NODE(2))),
                DIFF(LEFT(node), LEFT(node)));
}

void * DiffLOG(void * node)
{
    return _create_node(N_MUL,
            (_create_node(N_DIV, NUM_NODE(1), _create_node(N_MUL, _create_node(N_FUNC(LN), LEFT(node), NULL), _copy_node(RIGHT(node))))),
            DIFF(LEFT(node), LEFT(node)));
}

void * DiffHARDPOW(void * node)
{
    return _create_node(N_MUL, _create_node(N_POW, _create_node(N_E, NULL, NULL), _create_node(N_MUL, _create_node(N_FUNC(LN), _copy_branch(LEFT(node)), NULL), _copy_branch(RIGHT(node)))),
        (Node*)DiffMUL(_create_node(N_MUL, _create_node(N_FUNC(LN), _copy_branch(LEFT(node)), NULL), _copy_branch(RIGHT(node)))));
}




