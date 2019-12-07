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

char *numTypeNames[] = {
        "int",
        "double",
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


NUM_TYPE resolveNum(char *numName)
{
    int i = 0;
    while (numTypeNames[i][0] != '\0')
    {
        if (strcmp(numTypeNames[i], numName) == 0)
        {
            free(numName);
            return i;
        }
        i++;
    }
    return NO_TYPE;
}

// Called when an INT or DOUBLE token is encountered (see ciLisp.l and ciLisp.y).
// Creates an AST_NODE for the number.
// Sets the AST_NODE's type to number.
// Populates the value of the contained NUMBER_AST_NODE with the argument value.
// SEE: AST_NODE, NUM_AST_NODE, AST_NODE_TYPE.
AST_NODE *createNumberNode(double value, NUM_TYPE type)
{
    AST_NODE *node = newNode(NUM_NODE_TYPE);

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
            printf("You don't belong in this world, monster!\n");
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
AST_NODE *createFunctionNode(char *funcName, AST_NODE *op1)
{
    AST_NODE *node = newNode(FUNC_NODE_TYPE);

    // set the AST_NODE's type, populate contained FUNC_AST_NODE
    // NOTE: you do not need to populate the "ident" field unless the function is type CUSTOM_OPER.
    // When you do have a CUSTOM_OPER, you do NOT need to allocate and strcpy here.
    // The funcName will be a string identifier for which space should be allocated in the tokenizer.
    // For CUSTOM_OPER functions, you should simply assign the "ident" pointer to the passed in funcName.
    // For functions other than CUSTOM_OPER, you should free the funcName after you're assigned the OPER_TYPE.

    node->data.function.oper = resolveFunc(funcName);

    // Now: if oper is CUSTOM_OPER store funcName to ident
    if (node->data.function.oper == CUSTOM_OPER)
        node->data.function.ident = funcName;
    else
        free(funcName);

    // now adds this node as parent of op1 and op2
    node->data.function.opList = op1;

    // add parent to all op nodes
    AST_NODE *currOp = op1;
    while (currOp != NULL)
    {
        currOp->parent = node;
        currOp = currOp->next;
    }

    return node;
}

AST_NODE *createSymbolNode(char *ident)
{
    AST_NODE *node = newNode(SYMBOL_NODE_TYPE);

    node->data.symbol.ident = ident;

    return node;
}

AST_NODE *linkSexprToSexprList(AST_NODE *newNode, AST_NODE *nodeChainHead)
{
    newNode->next = nodeChainHead;
    return newNode;
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
SYMBOL_TABLE_NODE *createSymbolTableNode(char *type, char *ident, AST_NODE *val)
{
    SYMBOL_TABLE_NODE *node;
    size_t nodeSize;

    // allocate space (or error)
    nodeSize = sizeof(SYMBOL_TABLE_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
        yyerror("Memory allocation failed!");

    // copy identifier name
    node->ident = ident;

    // node val points to the s-expression that follows
    node->val = val;
    node->next = NULL;
    node->sym_type = VARIABLE_TYPE;
    node->val_type = resolveNum(type);

    return node;
}

SYMBOL_TABLE_NODE *linkLetSection(SYMBOL_TABLE_NODE *head, SYMBOL_TABLE_NODE *newVal)
{
    newVal->next = head;
    return newVal;
}

AST_NODE *createCondNode(AST_NODE *conditionsExpr, AST_NODE *truthExpr, AST_NODE *falseExpr)
{

    AST_NODE *node = newNode(COND_NODE_TYPE);

    // Assign nodes to their respective places.
    node->data.condition.condNode = conditionsExpr;
    conditionsExpr->parent = node;
    node->data.condition.trueNode = truthExpr;
    truthExpr->parent = node;
    node->data.condition.falseNode = falseExpr;
    falseExpr->parent = node;

    return node;
}

ARG_TABLE_NODE *createArgTableList(char *headName, ARG_TABLE_NODE *list)
{

    ARG_TABLE_NODE *node;
    size_t nodeSize;

    // allocate space (or error)
    nodeSize = sizeof(ARG_TABLE_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
        yyerror("Memory allocation failed!");

    // copy identifier name and attach new head to the list
    node->ident = headName;
    node->next = list;
    node->argVal = (RET_VAL) {DOUBLE_TYPE, NAN};

    return node;
}

SYMBOL_TABLE_NODE *createLambdaSymbolTableNode(char *type, char *ident, ARG_TABLE_NODE *argList, AST_NODE *val)
{
    SYMBOL_TABLE_NODE *node;
    size_t nodeSize;

    // allocate space (or error)
    nodeSize = sizeof(SYMBOL_TABLE_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
        yyerror("Memory allocation failed!");

    // same assignments from variable Symbol Table Node
    node->ident = ident;
    node->val_type = resolveNum(type);
    node->next = NULL;
    node->val = val;

    // change: this is instead a lambda, and its value carries the arguments in its argList
    node->sym_type = LAMBDA_TYPE;
    val->argTable = argList;

    return node;
}



AST_NODE *newNode(AST_NODE_TYPE type)
{
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
        yyerror("Memory allocation failed!");

    node->type = type;
    node->parent = NULL;
    node->next = NULL;
    node->symbolTable = NULL;
    node->argTable = NULL;

    return node;
}

// Called after execution is done on the base of the tree.
// (see the program production in ciLisp.y)
// Recursively frees the whole abstract syntax tree.
// You'll need to update and expand freeNode as the project develops.
void freeNode(AST_NODE *node)
{
    // TODO time to expand (maybe?)
    //  1) now if an AST node carries an argList, the identifier and the node itself must be freed - done
    if (!node)
        return;

    AST_NODE *currOp;

    switch (node->type)
    {
        case NUM_NODE_TYPE:
            break;

        case FUNC_NODE_TYPE:
            // Recursive calls to free child Ops
            currOp = node->data.function.opList;
            while (currOp != NULL)
            {
                freeNode(currOp);
                currOp = currOp->next;
            }

            // Free up identifier string if necessary
            if (node->data.function.oper == CUSTOM_OPER) {
                free(node->data.function.ident);
            }
            break;

        case SYMBOL_NODE_TYPE:
            free(node->data.symbol.ident);
            break;
        case COND_NODE_TYPE:
            freeNode(node->data.condition.condNode);
            freeNode(node->data.condition.trueNode);
            freeNode(node->data.condition.falseNode);
            break;
    } // END of switch statement

    // free associated symbol table node chain
    SYMBOL_TABLE_NODE *currNode = node->symbolTable;
    SYMBOL_TABLE_NODE *prevNode;
    while (currNode !=NULL)
    {
        prevNode = currNode;
        currNode = currNode->next;

        free(prevNode->ident);
        free(prevNode);
    }

    ARG_TABLE_NODE *currArg = node->argTable;
    ARG_TABLE_NODE *prevArg;
    while (currArg !=NULL)
    {
        prevArg = currArg;
        currArg = currArg->next;

        free(prevArg->ident);
        free(prevArg);
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
            result = evalFuncNode(node);
            break;
        case NUM_NODE_TYPE:
            result = evalNumNode(&node->data.number);
            break;
        case SYMBOL_NODE_TYPE:
            result = evalSymbolNode(node);
            break;
        case COND_NODE_TYPE:
            result = evalCondNode(&node->data.condition);
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

    // populate result with the values stored in the node.
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


RET_VAL evalFuncNode(AST_NODE *node)
{
    if (!node)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    FUNC_AST_NODE *funcNode = &(node->data.function);

    RET_VAL result = {DOUBLE_TYPE, NAN};

    // populate result with the result of running the function on its operands.
    // SEE: AST_NODE, AST_NODE_TYPE, FUNC_AST_NODE


    switch (funcNode->oper)
    {
        case NEG_OPER:
            result = helperNegOper(funcNode->opList);
            break;
        case ABS_OPER:
            result = helperAbsOper(funcNode->opList);
            break;
        case EXP_OPER:
            result = helperExpOper(funcNode->opList);
            break;
        case SQRT_OPER:
            result = helperSqrtOper(funcNode->opList);
            break;
        case ADD_OPER:
            result = helperAddOper(funcNode->opList);
            break;
        case SUB_OPER:
            result = helperSubOper(funcNode->opList);
            break;
        case MULT_OPER:
            result = helperMultOper(funcNode->opList);
            break;
        case DIV_OPER:
            result = helperDivOper(funcNode->opList);
            break;
        case REMAINDER_OPER:
            result = helperRemainderOper(funcNode->opList);
            break;
        case LOG_OPER:
            result = helperLogOper(funcNode->opList);
            break;
        case POW_OPER:
            result = helperPowOper(funcNode->opList);
            break;
        case MAX_OPER:
            result = helperMaxOper(funcNode->opList);
            break;
        case MIN_OPER:
            result = helperMinOper(funcNode->opList);
            break;
        case EXP2_OPER:
            result = helperExp2Oper(funcNode->opList);
            break;
        case CBRT_OPER:
            result = helperCbrtOper(funcNode->opList);
            break;
        case HYPOT_OPER:
            result = helperHypotOper(funcNode->opList);
            break;
        case PRINT_OPER:
            result = helperPrintOper(funcNode->opList);
            break;
        case READ_OPER:
            result = helperReadOper(node);
            break;
        case RAND_OPER:
            result = helperRandOper(node);
            break;
        case EQUAL_OPER:
            result = helperEqualOper(funcNode->opList);
            break;
        case LESS_OPER:
            result = helperLessOper(funcNode->opList);
            break;
        case GREATER_OPER:
            result = helperGreaterOper(funcNode->opList);
            break;
        case CUSTOM_OPER:
            result = helperCustomOper(node);
            break;
        default:
            printf("How did we get here?");
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
    SYMBOL_TABLE_NODE *currSymbol;
    ARG_TABLE_NODE *currArg;


    while (currNode != NULL)
    {
        currSymbol = currNode->symbolTable;
        while (currSymbol != NULL)
        {
            if (!strcmp(symbol, currSymbol->ident) && (currSymbol->sym_type == VARIABLE_TYPE))
            {
                result = evalSymbolNodeHelper(currSymbol);

                return result;
            }
            currSymbol = currSymbol->next;
        } // END of Symbol Table Search

        // Since args are now distinct from symbols, this checks the argTable after checking the symbol table
        currArg = currNode->argTable;
        while (currArg != NULL)
        {
            if (!strcmp(symbol, currArg->ident))
            {
                result = currArg->argVal;

                return result;
            }

            currArg = currArg->next;
        } // END of Arg Table Search


        currNode = currNode->parent;
    } // END of Search

    return result;
}


RET_VAL evalSymbolNodeHelper(SYMBOL_TABLE_NODE *symbol)
{

    if (!symbol)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = eval(symbol->val);

    // This whole block changes the returned result depending on the casted type of this symbol AST Node
    switch (symbol->val_type)
    {
        case NO_TYPE:
            return result;
        case INT_TYPE:
            switch (result.type)
            {
                case INT_TYPE:
                    break;
                case DOUBLE_TYPE:
                    printf("WARNING: precision loss in the assignment for variable \"%s\"\n", symbol->ident);
                    result.type = INT_TYPE;
                    result.value.ival = lround(result.value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;

        case DOUBLE_TYPE:
            switch (result.type)
            {
                case INT_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = (double) result.value.ival;
                    break;
                case DOUBLE_TYPE:
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;

        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    } // END of type cast adjustments

    return result;
}


RET_VAL evalCondNode(COND_AST_NODE *condAstNode)
{

    if (!condAstNode)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = eval(condAstNode->condNode);

    switch (result.type)
    {
        case INT_TYPE:
            if (result.value.ival)
                result = eval(condAstNode->trueNode);
            else
                result = eval(condAstNode->falseNode);
            break;
        case DOUBLE_TYPE:
            if (result.value.dval)
                result = eval(condAstNode->trueNode);
            else
                result = eval(condAstNode->falseNode);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            break;
    }

    return result;

}

// prints the type and value of a RET_VAL
void printRetVal(RET_VAL val)
{
    // print the type and value of the value passed in.


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

RET_VAL helperNegOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = eval(op1);



    switch (result.type)
    {
        case INT_TYPE:
            result.value.ival = -result.value.ival;
            break;
        case DOUBLE_TYPE:
            result.value.dval = -result.value.dval;
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next != NULL)
    {
        yyerror("Too many parameters for the function \"neg\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}


RET_VAL helperAbsOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = eval(op1);

    switch (result.type)
    {
        case INT_TYPE:
            result.value.ival = labs(result.value.ival);
            break;
        case DOUBLE_TYPE:
            result.value.dval = fabs(result.value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }


    if (op1->next != NULL)
    {
        yyerror("Too many parameters for the function \"abs\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}


RET_VAL helperExpOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = eval(op1);

    switch (result.type)
    {
        case INT_TYPE:
            result.type = DOUBLE_TYPE;
            result.value.dval = exp( (double) result.value.ival);
            break;
        case DOUBLE_TYPE:
            result.value.dval = exp(result.value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next != NULL)
    {
        yyerror("Too many parameters for the function \"exp\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}


RET_VAL helperSqrtOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = eval(op1);

    switch (result.type)
    {
        case INT_TYPE:
            result.type = DOUBLE_TYPE;
            result.value.dval = sqrt( (double) result.value.ival);
            break;
        case DOUBLE_TYPE:
            result.value.dval = sqrt(result.value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next != NULL)
    {
        yyerror("Too many parameters for the function \"sqrt\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}


RET_VAL helperAddOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};
    else if (!op1->next)
    {
        yyerror("Too few parameters for the function \"add\".\n");
        return (RET_VAL){DOUBLE_TYPE, NAN};
    }

    RET_VAL result = eval(op1);
    AST_NODE *currOp = op1->next;
    RET_VAL op2; // the follow up Operator value

    while (currOp != NULL)
    {
        op2 = eval(currOp);

        switch (result.type) {
            case INT_TYPE:
                switch (op2.type) {
                    case INT_TYPE:
                        result.value.ival += op2.value.ival;
                        break;
                    case DOUBLE_TYPE:
                        result.type = DOUBLE_TYPE;
                        result.value.dval = (double)result.value.ival + op2.value.dval;
                        break;
                    default:
                        yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
                }
                break;
            case DOUBLE_TYPE:
                switch (op2.type) {
                    case INT_TYPE:
                        result.value.dval += (double) op2.value.ival;
                        break;
                    case DOUBLE_TYPE:
                        result.value.dval += op2.value.dval;
                        break;
                    default:
                        yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
                }
                break;
            default:
                yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
        } // END of switch

        currOp = currOp->next;

    } // END of while

    return result;
}


RET_VAL helperSubOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};
    else if (!op1->next)
    {
        yyerror("Too few parameters for the function \"sub\".\n");
        return (RET_VAL){DOUBLE_TYPE, NAN};
    }

    RET_VAL result = eval(op1);
    AST_NODE *currOp = op1->next;
    RET_VAL op2; // the follow up Operator value

    while (currOp != NULL)
    {

        op2 = eval(currOp);

        switch (result.type) {
            case INT_TYPE:
                switch (op2.type) {
                    case INT_TYPE:
                        result.value.ival -= op2.value.ival;
                        break;
                    case DOUBLE_TYPE:
                        result.type = DOUBLE_TYPE;
                        result.value.dval = (double) result.value.ival - op2.value.dval;
                        break;
                    default:
                        yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
                }
                break;
            case DOUBLE_TYPE:
                switch (op2.type) {
                    case INT_TYPE:
                        result.value.dval -= (double) op2.value.ival;
                        break;
                    case DOUBLE_TYPE:
                        result.value.dval -= op2.value.dval;
                        break;
                    default:
                        yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
                }
                break;
            default:
                yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
        }

        currOp = currOp->next;

    }

    return result;
}


RET_VAL helperMultOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};
    else if (!op1->next)
    {
        yyerror("Too few parameters for the function \"mult\".\n");
        return (RET_VAL){DOUBLE_TYPE, NAN};
    }

    RET_VAL result = eval(op1);
    AST_NODE *currOp = op1->next;
    RET_VAL op2; // the follow up Operator value


    while (currOp != NULL)
    {

        op2 = eval(currOp);

        switch (result.type) {
            case INT_TYPE:
                switch (op2.type) {
                    case INT_TYPE:
                        result.value.ival *= op2.value.ival;
                        break;
                    case DOUBLE_TYPE:
                        result.type = DOUBLE_TYPE;
                        result.value.dval = (double) result.value.ival * op2.value.dval;
                        break;
                    default:
                        yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
                }
                break;
            case DOUBLE_TYPE:
                switch (op2.type) {
                    case INT_TYPE:
                        result.value.dval *= (double) op2.value.ival;
                        break;
                    case DOUBLE_TYPE:
                        result.value.dval *= op2.value.dval;
                        break;
                    default:
                        yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
                }
                break;
            default:
                yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
        }

        currOp = currOp->next;

    }

    return result;
}


RET_VAL helperDivOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};
    else if (!op1->next)
    {
        yyerror("Too few parameters for the function \"div\".\n");
        return (RET_VAL){DOUBLE_TYPE, NAN};
    }

    RET_VAL result = eval(op1);
    AST_NODE *currOp = op1->next;
    RET_VAL op2; // the follow up Operator value


    while (currOp != NULL)
    {

        op2 = eval(currOp);

        switch (result.type) {
            case INT_TYPE:
                switch (op2.type) {
                    case INT_TYPE:
                        result.value.ival /= op2.value.ival;
                        break;
                    case DOUBLE_TYPE:
                        result.type = DOUBLE_TYPE;
                        result.value.dval = (double) result.value.ival / op2.value.dval;
                        break;
                    default:
                        yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
                }
                break;
            case DOUBLE_TYPE:
                switch (op2.type) {
                    case INT_TYPE:
                        result.value.dval /= (double) op2.value.ival;
                        break;
                    case DOUBLE_TYPE:
                        result.value.dval /= op2.value.dval;
                        break;
                    default:
                        yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
                }
                break;
            default:
                yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
        }

        currOp = currOp->next;

    }

    return result;
}


RET_VAL helperRemainderOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};
    else if (!op1->next)
    {
        yyerror("Too few parameters for the function \"remainder\".\n");
        return (RET_VAL){DOUBLE_TYPE, NAN};
    }


    RET_VAL result = eval(op1);
    RET_VAL op2 = eval(op1->next);

    switch (result.type)
    {
        case INT_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.value.ival = result.value.ival % op2.value.ival;
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = fmod((double) result.value.ival, op2.value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.value.dval = fmod(result.value.dval, (double) op2.value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = fmod(result.value.dval, op2.value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next->next != NULL)
    {
        yyerror("Too many parameters for the function \"remainder\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}


RET_VAL helperLogOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};


    RET_VAL result = eval(op1);

    switch (result.type)
    {
        case INT_TYPE:
            result.type = DOUBLE_TYPE;
            result.value.dval = log( (double) result.value.ival);
            break;
        case DOUBLE_TYPE:
            result.value.dval = log(result.value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next != NULL)
    {
        yyerror("Too many parameters for the function \"log\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}


RET_VAL helperPowOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};
    else if (!op1->next)
    {
        yyerror("Too few parameters for the function \"pow\".\n");
        return (RET_VAL){DOUBLE_TYPE, NAN};
    }

    RET_VAL result = eval(op1);
    RET_VAL op2 = eval(op1->next);

    switch (result.type)
    {
        case INT_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.value.ival = lround(pow( (double) result.value.ival, (double) op2.value.ival));
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = pow((double) result.value.ival, op2.value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.value.dval = pow(result.value.dval, (double) op2.value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = pow( result.value.dval, op2.value.dval );
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next->next != NULL)
    {
        yyerror("Too many parameters for the function \"pow\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}


RET_VAL helperMaxOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};
    else if (!op1->next)
    {
        yyerror("Too few parameters for the function \"max\".\n");
        return (RET_VAL){DOUBLE_TYPE, NAN};
    }

    RET_VAL result = eval(op1);
    RET_VAL op2 = eval(op1->next);

    switch (result.type)
    {
        case INT_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.value.ival = lround( fmax( (double) result.value.ival, (double) op2.value.ival));
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = fmax((double) result.value.ival, op2.value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.value.dval = fmax(result.value.dval, (double) op2.value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = fmax( result.value.dval, op2.value.dval );
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next->next != NULL)
    {
        yyerror("Too many parameters for the function \"max\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}


RET_VAL helperMinOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};
    else if (!op1->next)
    {
        yyerror("Too few parameters for the function \"min\".\n");
        return (RET_VAL){DOUBLE_TYPE, NAN};
    }

    RET_VAL result = eval(op1);
    RET_VAL op2 = eval(op1->next);

    switch (op1->type)
    {
        case INT_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.value.ival = lround( fmin( (double) result.value.ival, (double) op2.value.ival));
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = fmin((double) result.value.ival, op2.value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.value.dval = fmin(result.value.dval, (double) op2.value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = fmin( result.value.dval, op2.value.dval );
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next->next != NULL)
    {
        yyerror("Too many parameters for the function \"min\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}


RET_VAL helperExp2Oper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = eval(op1);

    switch (result.type)
    {
        case INT_TYPE:
            result.type = DOUBLE_TYPE;
            result.value.dval = exp2( (double) result.value.ival);
            break;
        case DOUBLE_TYPE:
            result.value.dval = exp2(result.value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next != NULL)
    {
        yyerror("Too many parameters for the function \"exp2\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}


RET_VAL helperCbrtOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = eval(op1);

    switch (op1->type)
    {
        case INT_TYPE:
            result.type = DOUBLE_TYPE;
            result.value.dval = cbrt( (double) result.value.ival);
            break;
        case DOUBLE_TYPE:
            result.value.dval = cbrt(result.value.dval);
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next != NULL)
    {
        yyerror("Too many parameters for the function \"cbrt\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}


RET_VAL helperHypotOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){DOUBLE_TYPE, NAN};
    else if (!op1->next)
    {
        yyerror("Too few parameters for the function \"hypot\".\n");
        return (RET_VAL){DOUBLE_TYPE, NAN};
    }

    RET_VAL result = eval(op1);
    RET_VAL op2 = eval(op1->next);

    switch (result.type)
    {
        case INT_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = hypot( (double) result.value.ival, (double) op2.value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.type = DOUBLE_TYPE;
                    result.value.dval = hypot( (double) result.value.ival, op2.value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.value.dval = hypot( result.value.dval, (double) op2.value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.dval = hypot( result.value.dval, op2.value.dval );
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next->next != NULL)
    {
        yyerror("Too many parameters for the function \"hypot\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}

RET_VAL helperPrintOper(AST_NODE *op1)
{
    // Most recent helper function yes I put it at the bottom.

    if (!op1)
    {
        printf("Warning: This operation did not retrieve a number\n");
        return (RET_VAL) {DOUBLE_TYPE, NAN};
    }

    RET_VAL result = {DOUBLE_TYPE, NAN};

    AST_NODE *currOp = op1;
    char buffer[CHAR_BUFFER];
    int index = 0;

    while (currOp != NULL)
    {
        result = eval(currOp);

        switch (result.type) {
            case INT_TYPE:
                index += snprintf(buffer + index, CHAR_BUFFER - index, " %ld,", result.value.ival);
                break;
            case DOUBLE_TYPE:
                index += snprintf(buffer + index, CHAR_BUFFER - index, " %.2lf,", result.value.dval);
                break;
            default:
                yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!\n");
        }

        currOp = currOp->next;

    }

    printf("print:");
    puts(buffer);

    if (op1->next != NULL) {
        printf("WARNING: only the last item in this list is returned.\n");
    }

    return result;
}


RET_VAL helperReadOper(AST_NODE *root)
{
    RET_VAL result = {DOUBLE_TYPE, NAN};

    // read from user and store to result
    char numString[CHAR_BUFFER];
    printf("read := ");
    scanf("%s", numString);
    getchar();

    bool isDouble = false;

    // read the input and verify that it is a number
    for (int i = 0; numString[i] != '\0'; ++i) {
        switch (numString[i])
        {
            case '0'...'9':
                break;
            case '.':
                if (!isDouble)
                {
                    isDouble = true;
                    break;
                }
                else
                {
                    // the flag for double was already set. Error out.
                    yyerror("Extra decimal was entered.\n");
                }
            case '-':
                if (i == 0)
                {
                    break;
                }
            default:
                yyerror("Invalid input for a number entered\n");
                root->type = NUM_NODE_TYPE;
                root->data.number = result;
                return result;
        }
    }

    // store the value entered from the user to the result
    if (isDouble)
    {
        result.value.dval = strtod(numString, NULL);
    }
    else
    {
        result.type = INT_TYPE;
        result.value.ival = strtol(numString, NULL, 10);
    }

    // Change the function node to a number node
    // this is to ensure that the number is the same the next time it is called by the program.
    root->type = NUM_NODE_TYPE;
    root->data.number = result;

    return result;
}

RET_VAL helperRandOper(AST_NODE *root)
{
    RET_VAL result = {DOUBLE_TYPE, {(double) rand() / RAND_MAX}};

    // Change the function node to a number node
    // this is to ensure that the number is the same the next time it is called by the program.
    root->type = NUM_NODE_TYPE;
    root->data.number = result;

    return result;
}

RET_VAL helperEqualOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){INT_TYPE, 0};
    else if (!op1->next)
    {
        yyerror("Too few parameters for the function \"equal\".\n");
        return (RET_VAL){INT_TYPE, 0};
    }

    RET_VAL result = eval(op1);
    RET_VAL op2 = eval(op1->next);

    switch (result.type)
    {
        case INT_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.value.ival = (result.value.ival == op2.value.ival) ? 1 : 0;
                    break;
                case DOUBLE_TYPE:
                    result.value.ival = (fabs( (double) result.value.ival - op2.value.dval) < BUFFER_DOUBLE) ? 1 : 0;
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.type = INT_TYPE;
                    result.value.ival = (fabs(  result.value.dval - (double) op2.value.ival) < BUFFER_DOUBLE) ? 1 : 0;
                    break;
                case DOUBLE_TYPE:
                    result.type = INT_TYPE;
                    result.value.ival = (fabs( result.value.dval - op2.value.dval) < BUFFER_DOUBLE) ? 1 : 0;                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next->next != NULL)
    {
        yyerror("Too many parameters for the function \"equal\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;

}

RET_VAL helperLessOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){INT_TYPE, 0};
    else if (!op1->next)
    {
        yyerror("Too few parameters for the function \"less\".\n");
        return (RET_VAL){INT_TYPE, 0};
    }

    RET_VAL result = eval(op1);
    RET_VAL op2 = eval(op1->next);

    switch (result.type)
    {
        case INT_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.value.ival = (result.value.ival < op2.value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.ival = ( (double) result.value.ival < op2.value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.type = INT_TYPE;
                    result.value.ival = ( result.value.dval < (double) op2.value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.type = INT_TYPE;
                    result.value.ival = (result.value.dval < op2.value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next->next != NULL)
    {
        yyerror("Too many parameters for the function \"less\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}

RET_VAL helperGreaterOper(AST_NODE *op1)
{

    if (!op1)
        return (RET_VAL){INT_TYPE, 0};
    else if (!op1->next)
    {
        yyerror("Too few parameters for the function \"greater\".\n");
        return (RET_VAL){INT_TYPE, 0};
    }

    RET_VAL result = eval(op1);
    RET_VAL op2 = eval(op1->next);

    switch (result.type)
    {
        case INT_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.value.ival = (result.value.ival > op2.value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.value.ival = ( (double) result.value.ival > op2.value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        case DOUBLE_TYPE:
            switch (op2.type)
            {
                case INT_TYPE:
                    result.type = INT_TYPE;
                    result.value.ival = ( result.value.dval > (double) op2.value.ival);
                    break;
                case DOUBLE_TYPE:
                    result.type = INT_TYPE;
                    result.value.ival = (result.value.dval > op2.value.dval);
                    break;
                default:
                    yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
            }
            break;
        default:
            yyerror("Invalid NUM_NODE_TYPE, probably invalid writes somewhere!");
    }

    if (op1->next->next != NULL)
    {
        yyerror("Too many parameters for the function \"greater\".\n\t\tExtra parameters will be ignored\n");
    }

    return result;
}

// TODO newest helper function: helper for Lambda Functions

RET_VAL helperCustomOper(AST_NODE *root)
{

    if (!root)
        return (RET_VAL){DOUBLE_TYPE, NAN};

    RET_VAL result = {DOUBLE_TYPE, NAN};

    SYMBOL_TABLE_NODE *lambdaSeeker;
    AST_NODE *lambdaFunctionSeeker = root;
    char *lambdaName = root->data.function.ident;

    // Step 1: find the Symbol Table Node for the given lambda and its respective function (same as eval Symbol AST)
    while (lambdaFunctionSeeker != NULL)
    {
        lambdaSeeker = lambdaFunctionSeeker->symbolTable;
        while (lambdaSeeker != NULL)
        {
            // when lambda Symbol table node is found
            // Step 2: Evaluate all necessary parameters for the function
            if (!strcmp(lambdaSeeker->ident, lambdaName) && (lambdaSeeker->sym_type == LAMBDA_TYPE))
            {
                lambdaFunctionSeeker = lambdaSeeker->val;
                STACK_NODE *argValues = createStackNodes(lambdaFunctionSeeker, root->data.function.opList);
                if (argValues == NULL)
                    return (RET_VAL){DOUBLE_TYPE, NAN};
                
                attachStackNodes(lambdaFunctionSeeker->argTable, argValues);

                // Step 3: evaluate lambda's function
                result = eval(lambdaFunctionSeeker);
                return result;
            }
            lambdaSeeker = lambdaSeeker->next;
        } // END of Symbol Table traversal

        lambdaFunctionSeeker = lambdaFunctionSeeker->parent;
    } // END of AST node traversal

    return result;
}

STACK_NODE *createStackNodes(AST_NODE *lambdaFunc, AST_NODE *paramList)
{
    if (paramList == NULL) {
        yyerror("No parameters entered for lambda function\n");
        return NULL;
    }

    if (lambdaFunc == NULL) {
        yyerror("lambda function contains no parameters. Invalid writes somewhere\n");
        return NULL;
    }

    STACK_NODE *head;
    size_t nodeSize;

    // allocate space (or error)
    nodeSize = sizeof(STACK_NODE);
    if ((head = calloc(nodeSize, 1)) == NULL)
        yyerror("Memory allocation failed!");

    // evaluate the first node
    head->val = eval(paramList);


    // evaluate one parameter per lambda argument and create a stack node for it
    ARG_TABLE_NODE *currArg = lambdaFunc->argTable->next;
    AST_NODE * currOp = paramList->next;

    STACK_NODE *tail = head;

    while ((currArg != NULL) && (currOp != NULL))
    {

        if ((tail->next = calloc(nodeSize, 1)) == NULL)
            yyerror("Memory allocation failed!");

        tail = tail->next;
        tail->next = NULL;
        tail->val = eval(currOp);

        currArg = currArg->next;
        currOp = currOp->next;
    }


    // If there are too few or too many arguments, print an error
    if ((currArg == NULL) && (currOp != NULL))
    {
        yyerror("Too many parameters for lambda function.\n\t\tExtra parameters will be ignored\n");
    }
    else if (currArg != NULL)
    {
        yyerror("Too few parameters for lambda function.\t\tMissing parameters will be defaulted to 1\n");
        while (currArg != NULL)
        {

            if ((tail->next = calloc(nodeSize, 1)) == NULL)
                yyerror("Memory allocation failed!");

            tail = tail->next;
            tail->next = NULL;
            tail->val = (RET_VAL) {INT_TYPE, 1};

            currArg = currArg->next;
        }
    }

    return head;
}

void attachStackNodes(ARG_TABLE_NODE *lambdaArgs, STACK_NODE *paramVals)
{
    // assign retrieved values to their respective args
    ARG_TABLE_NODE *currArg = lambdaArgs;
    STACK_NODE *currStackNode = paramVals;
    STACK_NODE *prevStackNode;

    while ((currArg != NULL))
    {
        prevStackNode = currStackNode;

        currArg->argVal = currStackNode->val;

        // move to next and free the stack node
        currArg = currArg->next;
        currStackNode = currStackNode->next;
        free(prevStackNode);
    }
}
