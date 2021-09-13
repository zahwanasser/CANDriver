# CAN-
CAN driver for Tiva C
While designing this driver there was a trade off between Code Size and abstraction where by increasing the configurability of the driver (by enabling the user to use any can module and any interface for the desired functionality), this increased the code size greatly.
However there was a solution: which is making #if preprocessor directives to remove unused code parts but this would need the interference of the user to edit the macros used with these preprocessor directives and thus decrease the abstraction of the design.
The used approach was to prefer abstraction over code size.

In this driver, we have two files:

An H file that includes: configuration pointer to structs and enums that are passed to the functions and Function prototypes.
C file that includes the function definitions.

