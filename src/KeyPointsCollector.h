// KeyPointsCollector.h
// ~~~~~~~~~~~~~~~~~~~~
// Defines the KeyPointsCollector interface.
#ifndef KEY_POINTS_COLLECTOR__H
#define KEY_POINTS_COLLECTOR__H

#include "Common.h"
#include <clang-c/Index.h>

#include <iostream>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <vector>

class KeyPointsCollector {

  // Name of file we are analyzing
  const std::string filename;

  // CXFile object of analysis file.
  CXFile cxFile;

  // Vector of CXCursor objs pointing to node of interest
  std::vector<CXCursor> cursorObjs;

  // Add a cursor to cursorObjs
  void addCursor(CXCursor const &C) { cursorObjs.push_back(C); }

  // Index - set of translation units that would be linked together as an exe
  // Ref ^ https://clang.llvm.org/docs/LibClang.html
  // Marked as static as there could be multiple KPC objects for files that need
  // to be grouped together.
  inline static CXIndex KPCIndex = clang_createIndex(0, 0);

  // Top level translation unit of the source file.
  CXTranslationUnit translationUnit;

  // Root cursor of the translation unit.
  CXCursor rootCursor;

  // Debug option
  bool debug;

  // Map to hold include directives
  std::map<unsigned, std::string> includeDirectives;

  // Adds include directive to map
  void addIncludeDirective(unsigned lineNum, std::string includeDirective) {
    includeDirectives[lineNum] = includeDirective;
  }

  // Method to remove include directives from source file before parsing.
  void removeIncludeDirectives();

  // Method to reinsert include directives into source file.
  void reInsertIncludeDirectives();

  // This is a weird one, since clang_visitChildren requires a function ptr
  // for its second argument without any signature, its not possible to capture
  // 'this' with a lambda. Consequently, we mark this function as static and
  // pass 'this' for the clientdata parameter in the clang_visitChildren call to
  // retrieve the KPC instance.
  static CXChildVisitResult
  VisitorFunctionCore(CXCursor current, CXCursor parent, CXClientData kpc);

  // Visit compound statment, get line number of first child, and get line
  // number of end.
  static CXChildVisitResult VisitCompoundStmt(CXCursor current, CXCursor parent,
                                              CXClientData kpc);

  // Visitor for a CallExpr, to collect the function signature.
  static CXChildVisitResult VisitCallExpr(CXCursor current, CXCursor parent,
                                          CXClientData kpc);

  // Visitor for a FuncDecl, to collect name and defintion location.
  static CXChildVisitResult VisitFuncDecl(CXCursor current, CXCursor parent,
                                          CXClientData kpc);

  // Visitor for a VarDecl, to collect name and location
  static CXChildVisitResult
  VisitVarOrParamDecl(CXCursor current, CXCursor parent, CXClientData kpc);

  // Visitor for a function pointer, just to extract the name of the function it
  // is pointing to.
  static CXChildVisitResult VisitFuncPtr(CXCursor current, CXCursor parent,
                                         CXClientData kpc);

  // Map of function pointers, their ids mapped to the name of the function they
  // represent.
  std::map<std::string, std::string> funcPtrs;

  // Current func ptr id being looked at.
  std::string currFuncPtrId;

  // See if an Id maps to a function pointer
  std::string isFunctionPtr(const std::string &id) {
    if (MAP_FIND(funcPtrs, id)) {
      return funcPtrs[id];
    }
    return nullptr;
  }

  // Add a function pointer to the map.
  void addFuncPtr(const std::string &id, const std::string &func) {
    funcPtrs[id] = func;
  }

  // Struct to hold information about a function
  struct FunctionDeclInfo {
    // Location of function defintion and the end of its body
    unsigned defLoc;
    unsigned endLoc;
    // Function name
    const std::string name;
    // Return type of function
    const std::string type;
    // Is it a recursive function?
    bool recursive;

    FunctionDeclInfo(unsigned defLoc, unsigned endLoc, const std::string &name,
                     const std::string &type)
        : defLoc(defLoc), endLoc(endLoc), name(std::move(name)),
          type(std::move(type)) {}

    // Sets the recursive marker
    void setRecursive() { recursive = true; }

    // Is this line number inside this func body?
    bool isInBody(unsigned lineNum) {
      return lineNum >= defLoc && lineNum <= endLoc;
    }
  };

  // Add func decl to maps.
  void addFuncDecl(std::shared_ptr<FunctionDeclInfo> decl) {
    funcDecls[decl->defLoc] = decl;
    funcDeclsString[decl->name] = decl;
  }

  // Functions are stored being mapped from their definition line number to
  // their respective structs.
  std::map<unsigned, std::shared_ptr<FunctionDeclInfo>> funcDecls;

  // Additional map for function lookup by name
  std::map<std::string, std::shared_ptr<FunctionDeclInfo>> funcDeclsString;
  //
  // Function getter by string
  std::shared_ptr<FunctionDeclInfo> getFunctionByName(const std::string &name) {
    if (MAP_FIND(funcDeclsString, name)) {
      return funcDeclsString[name];
    }
    return nullptr;
  }

  // Current function being traversed.
  std::shared_ptr<FunctionDeclInfo> currentFunction;

  // Checks to see if the line number is in the current function
  bool inCurrentFunction(unsigned lineNumber) const {
    return currentFunction->isInBody(lineNumber);
  }

  // Map of line numbers mapped to the function being called
  std::map<unsigned, std::string> functionCalls;

  // Add a call to the call map
  void addCall(unsigned lineNum, const std::string &calleeName) {
    functionCalls[lineNum] = calleeName;
  }

  // Map of variable names (VarDecls) mapped to their declaration location
  std::map<std::string, unsigned> varDecls;

  // Adds a found varable declaration to the map
  void addVarDeclToMap(const std::string name, unsigned lineNum) {
    varDecls[name] = lineNum;
  }

  struct BranchPointInfo {
    unsigned branchPoint;
    std::vector<unsigned> targetLineNumbers;

    // The end target of a branch point is generally the end of its associated
    // compound statement. When encountered, keep a reference to this location
    // and check against the current child during visitation to see if it is
    // after this location, if it is, we know we have reached the end of a
    // branch point and can pop from the stack.
    unsigned compoundEndLineNum;
    // Save the column number too for cases where thee child is on the same
    // line.
    unsigned compoundEndColumnNum;

    BranchPointInfo()
        : branchPoint(0), compoundEndLineNum(0), compoundEndColumnNum(0) {}

    unsigned *getBranchPointOut() { return &branchPoint; }
    void addTarget(unsigned target) { targetLineNumbers.push_back(target); }
  };

  // Amount of branches, initialized to 0 in ctor
  unsigned branchCount;
  // Stack of branch points being analyzed.
  std::stack<BranchPointInfo> branchPointStack;

  // Vector of completed branch points
  std::vector<BranchPointInfo> branchPoints;

  // Push a new BP onto the stack
  void pushNewBranchPoint() { branchPointStack.push(BranchPointInfo()); }

  // Core branch dictionary
  // The outer map holds the initial branch point line number,
  // whilst the inner map holds the target line number for the branch,
  // mapped to the branch id e.g. 'br_2'
  std::map<unsigned, std::map<unsigned, std::string>> branchDictionary;

  // Called once branch analysis has completed.
  void addBranchesToDictionary();

  // Print found branch point
  void printFoundBranchPoint(const CXCursorKind K);

  // Print found target point
  void printFoundTargetPoint();

  // Simply print the cursor
  void printCursorKind(const CXCursorKind K);

  // Checks to see if the current cursor is a point in the program
  // that could be a branch
  bool isBranchPointOrCallExpr(const CXCursorKind K);

  // Checks to see if the current VarDecl is a function ptr;
  bool isFunctionPtr(const CXCursor C);

  // Checks to see if the stack is empty to ensure we have found a
  // compound statement before checking against further children.
  bool compoundStmtFoundYet() const { return !branchPointStack.empty(); }

  // Check if the current childs source location is on the same line or after
  // the stacks top compound end location.
  bool checkChildAgainstStackTop(CXCursor child);

  // Get pointer to current branch point info struct
  BranchPointInfo *getCurrentBranch() { return &branchPointStack.top(); }

  // Add completed  branch to vector of branches and pop from stack;
  void addCompletedBranch();

  // Creates dictionary file of branch points.
  void createDictionaryFile();

  // Iterates through the branch points and declares a flag for each one at the
  // top of the program: e.g int br_1 = 0
  void
  insertFunctionBranchPointDecls(std::ofstream &program,
                                 std::shared_ptr<FunctionDeclInfo> function,
                                 int *branchCount);

public:
  // KPC ctor, takes file name in, ownership is transfered to KPC.
  // Inits the translation unit, invoking the clang parser.
  KeyPointsCollector(const std::string &fileName, bool debug = false);

  // Dispose of necessary CX elements.
  ~KeyPointsCollector();

  // Returns a reference to collected cursor objects.
  const std::vector<CXCursor> &getCursorObjs() const { return cursorObjs; }

  // Returns a reference to map of function defintions
  const std::map<unsigned, std::shared_ptr<FunctionDeclInfo>> &
  getFuncDecls() const {
    return funcDecls;
  }

  // Returns a reference the map of known function calls.
  const std::map<unsigned, std::string> &getFuncCalls() const {
    return functionCalls;
  }

  // Returns a reference to map of variable defintions
  const std::map<std::string, unsigned> &getVarDecls() const {
    return varDecls;
  }

  // Return pointer to CXFile
  CXFile *getCXFile() { return &cxFile; }

  // Return reference to the translation unit
  CXTranslationUnit &getTU() { return translationUnit; }

  // Get branch dictionary
  const std::map<unsigned, std::map<unsigned, std::string>> &
  getBranchDictionary() {
    return branchDictionary;
  }

  // Invokes Valgrind toolchain to analyze the original programs executed
  // instructions.
  void invokeValgrind();

  // Does everything needed to get the branch pointer trace as a string.
  std::string getBPTrace();
  //
  // Once the transformed program has been created, compile it with system C
  // compiler.
  void compileModified();

  // Performs the transformation of the program so it can be compiled with
  // branch statements.
  void transformProgram();

  // Core AST traversal function, once the translation unit has been parsed,
  // recursively visit nodes and add to cursorObjs if they are of interest.
  void collectCursors();

  // Runs all necessary functions for part 1
  void executeToolchain();
};

#endif // KEY_POINTS_COLLECTOR__H
