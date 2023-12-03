# CSC412 Course Project Part 2: Seminal Input Features Detection
## Overview
The goal of this part is to build a static analysis tool based on LLVM that can automatically figure out the part of the inputs to a C program that determine the behaviors of a program at its key points. To achieve this, we have leveraged LibClang, a sub-tool from the Clang C-Family frontend under the larger LLVM Compiler Infrastructure Project. Heres a brief overview of how this program works:
1. Grabs the vector of cursors and map of variable declarations from part 1 of the project.
2. Recursively searches through each of the cursors for variables.
3. Collects and adds the variables to a vector if it exists in the variable declarations map and if it does not exist in the vector already.

# Example
Here is an example for this C program:<br>
```C
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
The SeminalInputFeatureDetector will output:<br>
```bash
Line 6: n
Line 8: s
```

# Build and Usage
To build this project, ensure you have the following items on your system. (These should all be installed on NCSU Ubuntu 22.04 LTS Image)<br>
- LibClang, this is a part of the LLVM Project. To build correctly, run the build script [here](https://github.com/NCSU-CSC512-Course-Project/part1-dev/blob/main/build_llvm.sh)
- Python 3.10

To build: Run the build_llvm script and get_kpc_deps script to grab the updated KeyPointsCollecter files<br>
```bash
./build_llvm.sh
./get_kpc_deps.sh
make
```
To run:
```bash
make run
```
You will be given a series of prompts to run the program, and all the files will be written to the ```out``` directory. Below is an example of the full shell output for the above program with the debugger on:<br>
```
$ part2-dev git:(main) make run
mkdir -p bin bin/objs out
g++ -O0 -g3 -std=c++17 -o bin/objs/SeminalInputFeatureDetector.o -c src/SeminalInputFeatureDetector.cpp
g++  bin/objs/KeyPointsCollector.o  bin/objs/main.o  bin/objs/SeminalInputFeatureDetector.o -O0 -g3 -std=c++17 -lclang  -o bin/SeminalInputFeatureDetector
bin/SeminalInputFeatureDetector
Enter a file name for analysis: test-files/TF_1_rand.c
Would you like the debugger on? (y/n): y
Translation unit for file: test-files/TF_1_rand.c successfully parsed.
Variable Declarations:
31: __u_char
9: i
5: id
6: n
8: s

Kind: ForStmt
  Kind: DeclStmt
    Kind: VarDecl
      Type: int
      Token: i
      Line 9

  Kind: BinaryOperator
    Kind: UnexposedExpr
      Type: int
      Token: i
      Line 9

  Kind: BinaryOperator
    Kind: UnexposedExpr
      Type: int
      Token: n
      Line 9


Kind: IfStmt
  Kind: IfStmt
    Kind: BinaryOperator
      Type: int
      Token: s
      Line 11


Line 6: n
Line 8: s
```
# Testing (For Grader)
All our chosen test files are prefixed with TF in the root directory, TF_4_SPEC.c is the chosen SPEC program for our testing.
