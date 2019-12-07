## BUGS ##
1) If there are any syntactical errors I don't know them. The Lex and Yacc files should work appropriately
    i.e. are grammatically correct. I feel like superfluous parentheses around an s-expression shouldn't
    work, but it didn't produce any errors when running

2) Valgrind says callocing for Number AST Nodes and Symbol AST Nodes causes leaks, which is weird because
    by that logic so should the other AST Nodes, but they don't. It isn't like Number AST Nodes have extra
    pointers to other places in memory that other AST Nodes don't have. What's the deal with that?

3) Lastly, there may be some segmentation faults somewhere. I don't know where, but one could still be crawling
    around. ( Try printing a MASSIVE list of doubles).
    
## Comments ##
1) I put as many comments as I felt necessary. For the most part, each submission had TODOs attached to 
    changes implemented for each task. I didn't delete any for subsequent tasks, but did delete the actual
    TODO word.
    
2) This file has over 1800 lines in the c file alone. A lot of it was comments, but much more of it was
    slightly repeated code that was just different enough to not be doable in a subroutine. This was mostly due
    to the fact that I kept the ival/dval union. It made it more verbose to say the least.
