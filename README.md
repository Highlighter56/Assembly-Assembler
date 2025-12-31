# Assembly-Assembler

This project began as part of my Assembly course, based on the text book C and C++ Under the Hood, Second Edition.
The assignment was to create an Assembler, based off the assembly compiler we use in class.

## Overview
Build upon the shell that has been given to me, and design a program that can take in an assembly file (.a), and
translate it to machine code (.e)

Supported Instructions:
 - br
 - add
 - ld
 - st
 - bl/call/jsr
 - blr/jsrr
 - and
 - ldr
 - str
 - not
 - lea
 - halt
 - nl
 - dout
 - jmp
 - ret
 - brz/bre
 - brn
 - brp
 - .word
 - .zero

## Usage
Compile:        gcc a1.c -o a1

Run Programs:   ./a1 a1test.a

## Future Plans
Expand to handel all assembly instructions cover in class, not just the ones required to handel.
Combine this with my interpreter project.
