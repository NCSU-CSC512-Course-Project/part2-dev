# CSC412 Course Project Part 2: Seminal Input Features Detection
## Overview
The goal of this part is to build a static analysis tool based on LLVM that can automatically figure out the part of the inputs to a C program that determine the behaviors of a program at its key points. To achieve this, we have leveraged LibClang, a sub-tool from the Clang C-Family frontend under the larger LLVM Compiler Infrastructure Project. Heres a brief overview of how this program works:
1. Grabs the vector of cursors and map of variable declarations from part 1 of the project.
2. Recursively searches through each of the cursors for variables.
3. Collects and adds the variables to a vector if it exists in the variable declarations map and if it does not exist in the vector already.
