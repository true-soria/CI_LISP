#include "ciLisp.h"


void yyerror(char *s) {
    fprintf(stderr, "\nERROR: %s\n", s);
    // note stderr that normally defaults to stdout, but can be redirected: ./src 2> src.log
    // CLion will display stderr in a different color from stdin and stdout
}

// Array of string values for operations.
// Must be in sync with funcs in the OPER_TYPE enum in order for resolveFunc to work.
char *funcNames[] = {
        "neg",
        "abs",
        "exp",
        "sqrt",
        "add",
        "sub",
        "mult",
        "div",
        "remainder",
        "log",
        "pow",
        "max",
        "min",
        "exp2",
        "cbrt",
        "hypot",
        "read",
        "rand",
        "print",
        "equal",
        "less",
        "greater",
        ""
};

OPER_TYPE resolveFunc(char *funcName)
{
    int i = 0;
    while (funcNames[i][0] != '\0')
    {
        if (strcmp(funcNames[i], funcName) == 0)
            return i;
        i++;
    }
    return CUSTOM_OPER;
}

// Called when an INT or DOUBLE token is encountered (see ciLisp.l and ciLisp.y).
// Creates an AST_NODE for the number.
// Sets the AST_NODE's type to number.
// Populates the value of the contained NUMBER_AST_NODE with the argument value.
// SEE: AST_NODE, NUM_AST_NODE, AST_NODE_TYPE.
AST_NODE *createNumberNode(double value, NUM_TYPE type)
{
    AST_NODE *node;
    size_t nodeSize;

    // allocate space for the fixed sie and the variable part (union)
    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
        yyerror("Memory allocation failed!");

    // TODO set the AST_NODE's type, assign values to contained NUM_AST_NODE

    node->type = NUM_NODE_TYPE;
    node->parent = NULL;

    switch (type)
    {
        case INT_TYPE:
            node->data.number.type = INT_TYPE;
            node->data.number.value.ival = (long) value;
            break;
        case DOUBLE_TYPE:
            node->data.number.type = DOUBLE_TYPE;
            node->data.number.value.dval = value;
            break;
        default:
            printf("You don't belong in this world, monster!");
    }


    return node;
}

// Called when an f_expr is created (see ciLisp.y).
// Creates an AST_NODE for a function call.
// Sets the created AST_NODE's type to function.
// Populates the contained FUNC_AST_NODE with:
//      - An OPER_TYPE (the enum identifying the specific function being called)
//      - 2 AST_NODEs, the operands
// SEE: AST_NODE, FUNC_AST_NODE, AST_NODE_TYPE.
AST_NODE *createFunctionNode(char *funcName, AST_NODE *op1, AST_NODE *op2)
{
    AST_NODE *node;
    size_t nodeSize;

    // allocate space (or error)
    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
        yyerror("Memory allocation failed!");

    // TODO set the AST_NODE's type, populate contained FUNC_AST_NODE
    // NOTE: you do not need to populate the "ident" field unless the function is type CUSTOM_OPER.
    // When you do have a CUSTOM_OPER, you do NOT need to allocate and strcpy here.
    // The funcName will be a string identifier for which space should be allocated in the tokenizer.
    // For CUSTOM_OPER functions, you should simply assign the "ident" pointer to the passed in funcName.
    // For functions other than CUSTOM_OPER, you should free the funcName after you're assigned the OPER_TYPE.

    node->type = FUNC_NODE_TYPE;
    node->data.function.oper = resolveFunc(funcName);
    node->parent = NULL;

    // later: if oper is CUSTOM_OPER store funcName to ident

    free(funcName);

    // now adds this node as parent of op1 and op2
    node->data.function.op1 = op1;
    op1->parent = node;

    if (op2 != NULL)
    {
        node->data.function.op2 = op2;
        op2->parent = node;
    }

    return node;
}

AST_NODE *createSymbolNode(char *ident)
{
    AST_NODE *node;
    size_t nodeSize;

    // allocate space (or error)
    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
        yyerror("Memory allocation failed!");

    node->type = SYMBOL_NODE_TYPE;
    node->parent = NULL;
//    node->data.symbol.ident = calloc(strlen(ident), 1);
//    strcpy(node->data.symbol.ident, ident);
    node->data.symbol.ident = ident;
//    free(ident);

    return node;
}

AST_NODE *linkASTtoLetList(SYMBOL_TABLE_NODE *letList, AST_NODE *op)
{
    op->symbolTable = letList;

    SYMBOL_TABLE_NODE *node = letList;

    // Make all symbol table value's parents this s-expression
    while (node != NULL)
    {
         node->val->parent = op;
        node = node->next;
    }

    return op;

}

/*
 * Creates one variable node that will later be linked to a list
 */
SYMBOL_TABLE_NODE *createSymbolTableNode(char *ident, AST_NODE *val)
{
    SYMBOL_TABLE_NODE *node;
    size_t nodeSize;

    // allocate space (or error)
    nodeSize = sizeof(SYMBOL_TABLE_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
        yyerror("Memory allocation failed!");

    // copy identifier name
//    node->ident = calloc(strlen(ident), 1);
//    strcpy(node->ident, ident);
    node->ident = ident;
//    free(ident);

    // node val points to the s-expression that follows
    node->val = val;
    node->next = NULL;

    return node;
}

SYMBOL_TABLE_NODE *linkLetSection(SYMBOL_TABLE_NODE *head, SYMBOL_TABLE_NODE *newVal)
{
    newVal->next = head;
    return newVal;
}


// Called after execution is done on the base of the tree.
// (see the program production in ciLisp.y)
// Recursively frees the whole abstract syntax tree.
// You'll need to update and expand freeNode as the project develops.
void freeNode(AST_NODE *node)
{
    // TODO time to expand (maybe?)

    if (!node)
        return;

    if (node->type == FUNC_NODE_TYPE)
    {
        // Recursive calls to free child nodes
        freeNode(node->data.function.op1);
        freeNode(node->data.function.op2);

        // Free up identifier string if necessary
        if (node->data.function.oper == CUSTOM_OPER)
        {
            free(node->data.function.ident);
        }
    }

    free(node);
}

// Evaluates an AST_NODE.
// returns a RET_VAL storing the the resulting value and type.
// You'll need to update and expand eval (and the more specific eval functions below)
// as the project develops.
RET_VAL eval(AST_NODE *node)
{
    if (!node)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN}; // see NUM_AST_NODE, because RET_VAL is just an alternative name for it.

    // Make calls to other eval functions based on node type.
    // Use the results of those calls to populate result.
    switch (node->type)
    {
        case FUNC_NODE_TYPE:
            result = evalFuncNode(&node->data.function);
            break;
        case NUM_NODE_TYPE:
            result = evalNumNode(&node->data.number);
            break;
        case SYMBOL_NODE_TYPE:
            // TODO make an eval function for Symbol Nodes - done
            result = evalSymbolNode(node);
            break;
        default:
            yyerror("Invalid AST_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}  

// returns a pointer to the NUM_AST_NODE (aka RET_VAL) referenced by node.
// DOES NOT allocate space for a new RET_VAL.
RET_VAL evalNumNode(NUM_AST_NODE *numNode)
{
    if (!numNode)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    // TODO populate result with the values stored in the node.
    // SEE: AST_NODE, AST_NODE_TYPE, NUM_AST_NODE

    result.type = numNode->type;
    switch (numNode->type)
    {
        case INT_TYPE:
            result.value.ival = numNode->value.ival;
            break;
        case DOUBLE_TYPE:
            result.value.dval = numNode->value.dval;
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }


    return result;
}


RET_VAL evalFuncNode(FUNC_AST_NODE *funcNode)
{
    if (!funcNode)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    // TODO populate result with the result of running the function on its operands.
    // SEE: AST_NODE, AST_NODE_TYPE, FUNC_AST_NODE


    RET_VAL op1;
    RET_VAL op2;

    switch (funcNode->oper)
    {
        case NEG_OPER:
            op1 = eval(funcNode->op1);
            result = helperNegOper(&op1);
            break;
        case ABS_OPER:
            op1 = eval(funcNode->op1);
            result = helperAbsOper(&op1);
            break;
        case EXP_OPER:
            op1 = eval(funcNode->op1);
            result = helperExpOper(&op1);
            break;
        case SQRT_OPER:
            op1 = eval(funcNode->op1);
            result = helperSqrtOper(&op1);
            break;
        case ADD_OPER:
            op1 = eval(funcNode->op1);
            op2 = eval(funcNode->op2);
            result = helperAddOper(&op1, &op2);
            break;
        case SUB_OPER:
            op1 = eval(funcNode->op1);
            op2 = eval(funcNode->op2);
            result = helperSubOper(&op1, &op2);
            break;
        case MULT_OPER:
            op1 = eval(funcNode->op1);
            op2 = eval(funcNode->op2);
            result = helperMultOper(&op1, &op2);
            break;
        case DIV_OPER:
            op1 = eval(funcNode->op1);
            op2 = eval(funcNode->op2);
            result = helperDivOper(&op1, &op2);
            break;
        case REMAINDER_OPER:
            op1 = eval(funcNode->op1);
            op2 = eval(funcNode->op2);
            result = helperRemainderOper(&op1, &op2);
            break;
        case LOG_OPER:
            op1 = eval(funcNode->op1);
            result = helperLogOper(&op1);
            break;
        case POW_OPER:
            op1 = eval(funcNode->op1);
            op2 = eval(funcNode->op2);
            result = helperPowOper(&op1, &op2);
            break;
        case MAX_OPER:
            op1 = eval(funcNode->op1);
            op2 = eval(funcNode->op2);
            result = helperMaxOper(&op1, &op2);
            break;
        case MIN_OPER:
            op1 = eval(funcNode->op1);
            op2 = eval(funcNode->op2);
            result = helperMinOper(&op1, &op2);
            break;
        case EXP2_OPER:
            op1 = eval(funcNode->op1);
            result = helperExp2Oper(&op1);
            break;
        case CBRT_OPER:
            op1 = eval(funcNode->op1);
            result = helperCbrtOper(&op1);
            break;
        case HYPOT_OPER:
            op1 = eval(funcNode->op1);
            op2 = eval(funcNode->op2);
            result = helperHypotOper(&op1, &op2);
            break;

            // how did we get here?
        case READ_OPER:
        case RAND_OPER:
        case PRINT_OPER:
        case EQUAL_OPER:
        case LESS_OPER:
        case GREATER_OPER:
            printf("How did we get here?");
            break;

        default:
            break;
    }

    return result;
}

RET_VAL evalSymbolNode(AST_NODE *symbolNode)
{

    if (!symbolNode)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    char *symbol = symbolNode->data.symbol.ident;
    AST_NODE *currNode = symbolNode;
    SYMBOL_TABLE_NODE *currTable;

    while (currNode != NULL)
    {
        currTable = currNode->symbolTable;
        while (currTable != NULL)
        {
            if (strcmp(symbol, currTable->ident) == 0)
            {
                result = eval(currTable->val);
                return result;
            }
            currTable = currTable->next;
        }
        currNode = currNode->parent;
    }

    return result;
}

// prints the type and value of a RET_VAL
void printRetVal(RET_VAL val)
{
    // TODO print the type and value of the value passed in.


    switch (val.type)
    {
        case INT_TYPE:
            printf("Int Type: %ld\n", val.value.ival);
            break;
        case DOUBLE_TYPE:
            printf("Double Type: %lf\n", val.value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

}

/*
       evalFuncNode Helper methods
     */

RET_VAL helperNegOper(RET_VAL *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            result.value.ival = -op1->value.ival;
            break;
        case DOUBLE_TYPE:
            result.type = DOUBLE_TYPE;
            result.value.dval = -op1->value.dval;
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperAbsOper(RET_VAL *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            result.value.ival = labs(op1->value.ival);
            break;
        case DOUBLE_TYPE:
            result.type = DOUBLE_TYPE;
            result.value.dval = fabs(op1->value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperExpOper(RET_VAL *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            result.value.dval = exp( (double) op1->value.ival);
            break;
        case DOUBLE_TYPE:
            result.value.dval = exp(op1->value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperSqrtOper(RET_VAL *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            result.value.dval = sqrt( (double) op1->value.ival);
            break;
        case DOUBLE_TYPE:
            result.value.dval = sqrt(op1->value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperAddOper(RET_VAL *op1, RET_VAL *op2)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.ival = op1->value.ival + op2->value.ival;
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = (double) op1->value.ival + op2->value.dval;
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            result.type = DOUBLE_TYPE;
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.dval = op1->value.dval + (double) op2->value.ival;
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = op1->value.dval + op2->value.dval;
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperSubOper(RET_VAL *op1, RET_VAL *op2)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.ival = op1->value.ival - op2->value.ival;
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = (double) op1->value.ival - op2->value.dval;
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            result.type = DOUBLE_TYPE;
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.dval = op1->value.dval - (double) op2->value.ival;
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = op1->value.dval - op2->value.dval;
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperMultOper(RET_VAL *op1, RET_VAL *op2)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.ival = op1->value.ival * op2->value.ival;
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = (double) op1->value.ival * op2->value.dval;
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            result.type = DOUBLE_TYPE;
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.dval = op1->value.dval * (double) op2->value.ival;
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = op1->value.dval * op2->value.dval;
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperDivOper(RET_VAL *op1, RET_VAL *op2)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.ival = op1->value.ival / op2->value.ival;
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = (double) op1->value.ival / op2->value.dval;
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            result.type = DOUBLE_TYPE;
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.dval = op1->value.dval / (double) op2->value.ival;
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = op1->value.dval / op2->value.dval;
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperRemainderOper(RET_VAL *op1, RET_VAL *op2)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.ival = op1->value.ival % op2->value.ival;
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = fmod((double) op1->value.ival, op2->value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            result.type = DOUBLE_TYPE;
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.dval = fmod(op1->value.dval, (double) op2->value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = fmod(op1->value.dval, op2->value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperLogOper(RET_VAL *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            result.value.dval = log( (double) op1->value.ival);
            break;
        case DOUBLE_TYPE:
            result.value.dval = log(op1->value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperPowOper(RET_VAL *op1, RET_VAL *op2)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.ival = lround(pow( (double) op1->value.ival, (double) op2->value.ival));
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = pow((double) op1->value.ival, op2->value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            result.type = DOUBLE_TYPE;
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.dval = pow(op1->value.dval, (double) op2->value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = pow( op1->value.dval, op2->value.dval );
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperMaxOper(RET_VAL *op1, RET_VAL *op2)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.ival = lround( fmax( (double) op1->value.ival, (double) op2->value.ival));
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = fmax((double) op1->value.ival, op2->value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            result.type = DOUBLE_TYPE;
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.dval = fmax(op1->value.dval, (double) op2->value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = fmax( op1->value.dval, op2->value.dval );
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperMinOper(RET_VAL *op1, RET_VAL *op2)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.ival = lround( fmin( (double) op1->value.ival, (double) op2->value.ival));
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = fmin((double) op1->value.ival, op2->value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            result.type = DOUBLE_TYPE;
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.dval = fmin(op1->value.dval, (double) op2->value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = fmin( op1->value.dval, op2->value.dval );
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperExp2Oper(RET_VAL *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            result.value.dval = exp2( (double) op1->value.ival);
            break;
        case DOUBLE_TYPE:
            result.value.dval = exp2(op1->value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperCbrtOper(RET_VAL *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            result.value.dval = cbrt( (double) op1->value.ival);
            break;
        case DOUBLE_TYPE:
            result.value.dval = cbrt(op1->value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}


RET_VAL helperHypotOper(RET_VAL *op1, RET_VAL *op2)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    switch (op1->type)
    {
        case INT_TYPE:
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.dval = hypot( (double) op1->value.ival, (double) op2->value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = hypot( (double) op1->value.ival, op2->value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            switch (op2->type)
            {
                case INT_TYPE:
                    result.value.dval = hypot( op1->value.dval, (double) op2->value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = hypot( op1->value.dval, op2->value.dval );
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    return result;
}
