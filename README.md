WTF language
===

[![Build Status](https://travis-ci.org/altschuler/cmplr.png?branch=master)](https://travis-ci.org/altschuler/cmplr)

This is a learning project and has no real usage. Mostly made by following the [LLVM tutorial](http://llvm.org/docs/tutorial/LangImpl1.html).

WTF is a functional, intepreted language backed by LLVM. Current implementation supports
 + Import of .wtf files
 + Function definitions
 + Mutable variables
 + Control flow basics like for loops and conditionals
 + Definition of new operators at runtime
 + Declaration of native externs
 + Printing numbers and ascii values to output

Most of the standard operators are implemented in WTF itself.

## Semantics
### Data types

Currently only doubles are supported.

### Expressions
Can be one of 
 + A double literal
 + A conditional
 + A for loop
 + An assignment
 + A unary or binary operation

For binary operations, infix notation is used. Operator precedence follow standard arithmetic rules but can (and must) be declared when defining new operators.

Expressions are generally terminated with `;` or the `end` keyword, though `;` can be omitted in some cases (for instance when inlining a binary operation as a function argument).

### Blocks
A blocks is simply an ordered list of expressions. Generally, the last expression of a block will serve as its return value. That means early returns are not possible.

## Syntax
### Assignment
Assignment is done as usual, with the `=` operator. Note that `~` is used as the comparison operator, since multi-character operators are not supported. Assignment uses the `var` keyword, and has the usual syntax

    var <variable_id> = <initial_value_expression>

This will declare `variable_id` with a value of `initial_value_expression` in the current function scope.

### Functions
    func ([<param_identifier>[,] ...])
      <block_expression>
    end
Note that comma separating the parameter identifiers is optional as there can be no spaces in identifier names.

### For loop
    for <id> = <value>, <step_expression>, [<increment_expression>] in
      <block_expression>
    end
 
### Conditional
    if <if_cond_expression> then
    	<if_block>
    [elsif <elsif_cond_expression> then
    	<elsif_block>]
    else
		<else_block>
    end
 
### Operators
    op <operator_char> <operator_precedence> ([<left_operand_identifier>] <right_operand_identifier>)
		<operator_block>
    end
	
### Externs
    extern <extern_identifier>([<param_identifier>[,] ...])
