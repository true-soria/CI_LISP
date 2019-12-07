#ifndef __cilisp_h_
#define __cilisp_h_
#define BUFFER_DOUBLE 0.000001
#define CHAR_BUFFER 128

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "ciLispParser.h"

int yyparse(void);

int yylex(void);

void yyerror(char *);

// Enum of all operators.
// must be in sync with funcs in resolveFunc()
typedef enum oper {
    NEG_OPER, // 0
    ABS_OPER,
    EXP_OPER,
    SQRT_OPER,
    ADD_OPER,
    SUB_OPER,
    MULT_OPER,
    DIV_OPER,
    REMAINDER_OPER,
    LOG_OPER,
    POW_OPER,
    MAX_OPER,
    MIN_OPER,
    EXP2_OPER,
    CBRT_OPER,
    HYPOT_OPER,
    READ_OPER,
    RAND_OPER,
    PRINT_OPER,
    EQUAL_OPER,
    LESS_OPER,
    GREATER_OPER,
    CUSTOM_OPER =255
} OPER_TYPE;

OPER_TYPE resolveFunc(char *);

// Types of Abstract Syntax Tree nodes.
// Initially, there are only numbers and functions.
// You will expand this enum as you build the project.
typedef enum {
    NUM_NODE_TYPE,
    FUNC_NODE_TYPE,
    SYMBOL_NODE_TYPE,
    COND_NODE_TYPE
} AST_NODE_TYPE;

// Types of numeric values
typedef enum {
    INT_TYPE,
    DOUBLE_TYPE,
    NO_TYPE
} NUM_TYPE;

// TODO new:
//  Types of symbols (e.g. a user defined function or a variable)
typedef enum {
    VARIABLE_TYPE,
    LAMBDA_TYPE
} SYMBOL_TYPE;

// decides NUM_TYPE for symbols
NUM_TYPE resolveNum(char *);

// Node to store a number.
typedef struct {
    NUM_TYPE type;
    union{
        double dval;
        long ival;
    } value;
} NUM_AST_NODE;

// Values returned by eval function will be numbers with a type.
// They have the same structure as a NUM_AST_NODE.
// The line below allows us to give this struct another name for readability.
typedef NUM_AST_NODE RET_VAL;

// Node to store a function call with its inputs
typedef struct {
    OPER_TYPE oper;
    char* ident; // only needed for custom functions
    struct ast_node *opList;
} FUNC_AST_NODE;

// Symbol table node chain for storing values of variables to a knowledge base
// CHAIN OF NODES

// TODO edited:
//  be sure to go back and edit functions that use this
typedef struct symbol_table_node {
    SYMBOL_TYPE sym_type;
    NUM_TYPE val_type;
    char *ident;
    struct ast_node *val;
    struct symbol_table_node *next;
} SYMBOL_TABLE_NODE;

// Symbol Abstract Syntax Tree Node. Node to store a defined variable.

typedef struct symbol_ast_node {
    char *ident;
} SYMBOL_AST_NODE;

// Condition Abstract Syntax Tree Node. This node works like an if/else statement
// if the evaluation of condNode leads to 0, the falseNode is evaluated and returned
// if the evaluation leads to any non-0 value, the trueNode is evaluated and returned
typedef struct {
    struct ast_node *condNode;
    struct ast_node *trueNode; // to eval if cond is nonzero
    struct ast_node *falseNode; // to eval if cond is zero
} COND_AST_NODE;

// Generic Abstract Syntax Tree node. Stores the type of node,
// and reference to the corresponding specific node (initially a number or function call).
typedef struct ast_node {
    AST_NODE_TYPE type;
    SYMBOL_TABLE_NODE *symbolTable;
    struct arg_table_node *argTable;
    struct ast_node *parent;
    union {
        NUM_AST_NODE number;
        FUNC_AST_NODE function;
        COND_AST_NODE condition;
        SYMBOL_AST_NODE symbol;
    } data;
    struct ast_node *next;
} AST_NODE;

// TODO new:
//  a convenience function that allocates memory for AST nodes
AST_NODE *newNode(AST_NODE_TYPE type);

// TODO new:
//  Stack Node: a link list that stores the result of processing each argument passed to a custom function
typedef struct stack_node {
    RET_VAL val;
    struct stack_node *next;
} STACK_NODE;

// TODO new:
//  Argument Node: the arguments taken in by the user defined function
typedef struct arg_table_node {
    char *ident;
    RET_VAL argVal;
    struct arg_table_node *next;
} ARG_TABLE_NODE;

AST_NODE *createNumberNode(double value, NUM_TYPE type);

AST_NODE *createFunctionNode(char *funcName, AST_NODE *op1);

// Creates symbol type AST node
AST_NODE *createSymbolNode(char *ident);

// Attaches S-expr to the rest of the S-expr list
AST_NODE *linkSexprToSexprList(AST_NODE *newNode, AST_NODE *nodeChainHead);

// Attaches symbol table node chain to parent AST node
AST_NODE *linkASTtoLetList(SYMBOL_TABLE_NODE *letList, AST_NODE *op);

SYMBOL_TABLE_NODE *createSymbolTableNode(char *type, char *ident, AST_NODE *val);

// links Symbol Table Node Chain
SYMBOL_TABLE_NODE *linkLetSection(SYMBOL_TABLE_NODE *head, SYMBOL_TABLE_NODE *newVal);

// Creates condition type AST node
AST_NODE *createCondNode(AST_NODE *conditionsExpr, AST_NODE *truthExpr, AST_NODE *falseExpr);

// TODO new section:
//  1) create createArgTableList function that creates a list of args - done
//  2) create createLambdaSymbolTableNode that creates a user defined function as a Symbol Table Node - done
//      a) value of lambda function points to s-expr
//      b) s-expr argtable points to arglist
//      c) don't worry about parenting, that still happens in linkASTtoLetList
//      d) move name into ident pointer of the node
//      e) symbol type is lambda type

ARG_TABLE_NODE *createArgTableList(char *headName, ARG_TABLE_NODE *list);
SYMBOL_TABLE_NODE *createLambdaSymbolTableNode(char *type, char *ident, ARG_TABLE_NODE *argList, AST_NODE *val);

void freeNode(AST_NODE *node);

RET_VAL eval(AST_NODE *node);
RET_VAL evalNumNode(NUM_AST_NODE *numNode);
RET_VAL evalFuncNode(AST_NODE *node);

// TODO definitely needs to be updated - done
RET_VAL evalSymbolNode(AST_NODE *symbolNode);

// A helper for addressing typecasting for symbols
RET_VAL evalSymbolNodeHelper(SYMBOL_TABLE_NODE *symbol);

RET_VAL evalCondNode(COND_AST_NODE *condAstNode);

void printRetVal(RET_VAL val);

// evalFuncNode helper methods

RET_VAL helperNegOper(AST_NODE *op1);
RET_VAL helperAbsOper(AST_NODE *op1);
RET_VAL helperExpOper(AST_NODE *op1);
RET_VAL helperSqrtOper(AST_NODE *op1);
RET_VAL helperAddOper(AST_NODE *op1);
RET_VAL helperSubOper(AST_NODE *op1);
RET_VAL helperMultOper(AST_NODE *op1);
RET_VAL helperDivOper(AST_NODE *op1);
RET_VAL helperRemainderOper(AST_NODE *op1);
RET_VAL helperLogOper(AST_NODE *op1);
RET_VAL helperPowOper(AST_NODE *op1);
RET_VAL helperMaxOper(AST_NODE *op1);
RET_VAL helperMinOper(AST_NODE *op1);

RET_VAL helperExp2Oper(AST_NODE *op1);
RET_VAL helperCbrtOper(AST_NODE *op1);
RET_VAL helperHypotOper(AST_NODE *op1);

RET_VAL helperPrintOper(AST_NODE *op1);


// functions of part 6

RET_VAL helperReadOper(AST_NODE *root);
RET_VAL helperRandOper(AST_NODE *root);
RET_VAL helperEqualOper(AST_NODE *op1);
RET_VAL helperLessOper(AST_NODE *op1);
RET_VAL helperGreaterOper(AST_NODE *op1);

// TODO task 7/8 Custom Oper helper
RET_VAL helperCustomOper(AST_NODE *root);

// This evaluates the necessary amount of parameters for a custom function
// and passes them back to helperCustomOper
STACK_NODE *createStackNodes(AST_NODE *lambdaFunc, AST_NODE *paramList);

// Attaches the stack nodes RET_VALs to the lambda's arguments and frees the stack nodes
void attachStackNodes(ARG_TABLE_NODE *lambdaArgs, STACK_NODE *paramVals);

#endif
