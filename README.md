# CSC412 Course Project Part 2: Seminal Input Features Detection
## Overview
The goal of this part is to build a static analysis tool based on LLVM that can automatically figure out the part of the inputs to a C program that determine the behaviors of a program at its key points. To achieve this, we have leveraged LibClang, a sub-tool from the Clang C-Family frontend under the larger LLVM Compiler Infrastructure Project. Heres a brief overview of how this program works:
1. Grabs the vector of cursors and map of variable declarations from part 1 of the project.
2. Recursively searches through each of the cursors for variables.
3. Collects and adds the variables to a vector if it exists in the variable declarations map and if it does not exist in the vector already.

# Example
Here is an example for this C program:
```bash
int main(){
   int id;
   int n;
   scanf("%d, %d", &id, &n);
   int s = 0;
   for ( int i = 0; i < n; i++ ) {
      s += rand();
      if ( s > 10 ) {
         break;
      }
   }
   printf( "id=%d; sum=%d\n", id, n );
}
```
The SeminalInputFeatureDetector will output:
Line 6: n
Line 8: s

# Build and Usage
To build this project, ensure you have the following items on your system. (These should all be installed on NCSU Ubuntu 22.04 LTS Image)<br>
- LibClang, this is a part of the LLVM Project. To build correctly, run the build script [here](https://github.com/NCSU-CSC512-Course-Project/part1-dev/blob/main/build_llvm.sh)
- Python 3.10

To build:<br>
```bash
./build_llvm.sh
make
```
To run:
```bash
make run
```
You will be given a series of prompts to run the program, and all the files will be written to the ```out``` directory. Below is an example of the full shell output for the above program:<br>
```
$ part2-dev git:(main) make run
...
```
# Testing (For Grader)
All our chosen test files are prefixed with TF in the root directory, TF_3_SPEC.c is the chosen SPEC program for our testing.
