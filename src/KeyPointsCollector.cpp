// KeyPointsCollector.cpp
// ~~~~~~~~~~~~~~~~~~~~~~
// Implementation of KeyPointsCollector interface.
#include "KeyPointsCollector.h"
#include "Common.h"
#include <fstream>
#include <iostream>
#include <sstream>

// Ctor Implementation
KeyPointsCollector::KeyPointsCollector(const std::string &filename, bool debug)
    : filename(std::move(filename)), debug(debug) {

  // Check if file exists
  std::ifstream file(filename);
  if (file.good()) {
    file.close();

    // If good, try to parse the translation unit.
    translationUnit =
        clang_parseTranslationUnit(KPCIndex, filename.c_str(), nullptr, 0,
                                   nullptr, 0, CXTranslationUnit_None);
    // Check if parsed properly
    if (translationUnit == nullptr) {
      std::cerr << "There was an error parsing the translation unit! Exiting..."
                << std::endl;
      exit(EXIT_FAILURE);
    }
    std::cout << "Translation unit for file: " << filename
              << " successfully parsed." << std::endl;

    // Init cursor and branch count
    rootCursor = clang_getTranslationUnitCursor(translationUnit);
    cxFile = clang_getFile(translationUnit, filename.c_str());
    branchCount = 0;

    // Traverse
    collectCursors();
  } else {

    std::cerr << "File with name: " << filename
              << ", does not exist! Exiting..." << std::endl;
    exit(EXIT_FAILURE);
  }
}

KeyPointsCollector::~KeyPointsCollector() {
  clang_disposeTranslationUnit(translationUnit);
  clang_disposeIndex(KPCIndex);
}

bool KeyPointsCollector::isBranchPointOrFunctionPtr(const CXCursorKind K) {
  switch (K) {
  case CXCursor_IfStmt:
  case CXCursor_ForStmt:
  case CXCursor_DoStmt:
  case CXCursor_WhileStmt:
  case CXCursor_GotoStmt:
  case CXCursor_SwitchStmt:
  case CXCursor_CallExpr:
    return true;
  default:
    return false;
  }
}

bool KeyPointsCollector::checkChildAgainstStackTop(CXCursor child) {
  unsigned childLineNum;
  unsigned childColNum;
  BranchPointInfo *currBranch = getCurrentBranch();
  CXSourceLocation childLoc = clang_getCursorLocation(child);
  clang_getSpellingLocation(childLoc, getCXFile(), &childLineNum, &childColNum,
                            nullptr);

  if (childLineNum > currBranch->compoundEndLineNum ||
      (childLineNum == currBranch->compoundEndLineNum &&
       childColNum > currBranch->compoundEndColumnNum)) {
    getCurrentBranch()->addTarget(childLineNum);
    if (debug) {
      printFoundTargetPoint();
    }
    return true;
  } else {
    return false;
  }
}

CXChildVisitResult KeyPointsCollector::VisitorFunctionCore(CXCursor current,
                                                           CXCursor parent,
                                                           CXClientData kpc) {
  // Retrieve required data from call
  KeyPointsCollector *instance = static_cast<KeyPointsCollector *>(kpc);
  const CXCursorKind currKind = clang_getCursorKind(current);
  const CXCursorKind parrKind = clang_getCursorKind(parent);

  // If branch point, push new BP to stack, add cursor to list,  and recurse
  // through children.
  if (instance->isBranchPointOrFunctionPtr(currKind)) {

    // If it is a call expression, recurse on children with special visitor.
    if (currKind == CXCursor_CallExpr) {
      clang_visitChildren(current, &KeyPointsCollector::VisitCallExpr, kpc);
      return CXChildVisit_Continue;
    }

    instance->addCursor(current);
    instance->pushNewBranchPoint();
    CXSourceLocation loc = clang_getCursorLocation(current);
    clang_getSpellingLocation(loc, instance->getCXFile(),
                              instance->getCurrentBranch()->getBranchPointOut(),
                              nullptr, nullptr);
    if (instance->debug) {
      instance->printFoundBranchPoint(currKind);
    }
  }

  // If parent is a branch point and current is a compound statement,
  // visit first child of compound to get target.
  if (instance->isBranchPointOrFunctionPtr(parrKind) &&
      currKind == CXCursor_CompoundStmt) {
    clang_visitChildren(current, &KeyPointsCollector::VisitCompoundStmt, kpc);

    // Save end of compound statement
    BranchPointInfo *currBranch = instance->getCurrentBranch();
    CXSourceLocation compoundEnd =
        clang_getRangeEnd(clang_getCursorExtent(current));

    clang_getSpellingLocation(compoundEnd, instance->getCXFile(),
                              &(currBranch->compoundEndLineNum), nullptr,
                              nullptr);
  }
  // Check to see if child is after the current saved compound statement end '}'
  // location, add to completed.
  if (instance->compoundStmtFoundYet() &&
      instance->getCurrentBranch()->compoundEndLineNum != 0 &&
      instance->checkChildAgainstStackTop(current)) {
    instance->addCompletedBranch();
  }

  // If check to see if it is a FuncDecl
  if (currKind == CXCursor_FunctionDecl) {
    clang_visitChildren(parent, &KeyPointsCollector::VisitFuncDecl, kpc);
  }

  // If check to see if it is a VarDecl
  if (currKind == CXCursor_VarDecl) {
    clang_visitChildren(parent, &KeyPointsCollector::VisitVarDecl, kpc);
    return CXChildVisit_Continue;
  }

  return CXChildVisit_Recurse;
}

CXChildVisitResult KeyPointsCollector::VisitCompoundStmt(CXCursor current,
                                                         CXCursor parent,
                                                         CXClientData kpc) {
  KeyPointsCollector *instance = static_cast<KeyPointsCollector *>(kpc);
  const CXCursorKind currKind = clang_getCursorKind(current);
  const CXCursorKind parrKind = clang_getCursorKind(parent);
  if (parrKind != CXCursor_CompoundStmt) {
    std::cerr
        << "Compound statement visitor called when cursor is not compound stmt!"
        << std::endl;
    exit(EXIT_FAILURE);
  }
  // Get line number of first child
  unsigned targetLineNumber;
  CXSourceLocation loc = clang_getCursorLocation(current);
  clang_getSpellingLocation(loc, instance->getCXFile(), &targetLineNumber,
                            nullptr, nullptr);

  // Append line number to targets
  instance->getCurrentBranch()->addTarget(targetLineNumber);
  if (instance->debug) {
    instance->printFoundTargetPoint();
  }
  return CXChildVisit_Continue;
}

CXChildVisitResult KeyPointsCollector::VisitCallExpr(CXCursor current,
                                                     CXCursor parent,
                                                     CXClientData kpc) {
  KeyPointsCollector *instance = static_cast<KeyPointsCollector *>(kpc);
  const CXCursorKind currKind = clang_getCursorKind(current);
  const CXCursorKind parrKind = clang_getCursorKind(parent);
  return CXChildVisit_Recurse;
}

CXChildVisitResult KeyPointsCollector::VisitVarDecl(CXCursor current,
                                                    CXCursor parent,
                                                    CXClientData kpc) {
  KeyPointsCollector *instance = static_cast<KeyPointsCollector *>(kpc);

  // First retrive the line number
  unsigned varDeclLineNum;
  CXSourceLocation varDeclLoc = clang_getCursorLocation(current);
  clang_getSpellingLocation(varDeclLoc, instance->getCXFile(), &varDeclLineNum,
                            nullptr, nullptr);

  // Get token and its spelling
  CXToken *varDeclToken = clang_getToken(instance->getTU(), varDeclLoc);
  std::string varName =
      CXSTR(clang_getTokenSpelling(instance->getTU(), *varDeclToken));

  // Get reference to map for checking
  std::map<std::string, unsigned> varMap = instance->getVarDecls();

  // Add to map of FuncDecls
  if (varMap.find(varName) == varMap.end()) {
    if (instance->debug) {
      std::cout << "Found VarDecl: " << varName << " at line # "
                << varDeclLineNum << std::endl;
    }
    instance->addVarDeclToMap(varName, varDeclLineNum);
  }
  clang_disposeTokens(instance->getTU(), varDeclToken, 1);
  return CXChildVisit_Break;
}

CXChildVisitResult KeyPointsCollector::VisitFuncDecl(CXCursor current,
                                                     CXCursor parent,
                                                     CXClientData kpc) {
  KeyPointsCollector *instance = static_cast<KeyPointsCollector *>(kpc);

  // First retrive the line number
  unsigned funcDeclLineNum;
  CXSourceLocation funcDeclLoc = clang_getCursorLocation(current);
  clang_getSpellingLocation(funcDeclLoc, instance->getCXFile(),
                            &funcDeclLineNum, nullptr, nullptr);

  // Get token and its spelling
  CXToken *funcDeclToken = clang_getToken(instance->getTU(), funcDeclLoc);
  std::string funcName =
      CXSTR(clang_getTokenSpelling(instance->getTU(), *funcDeclToken));

  // Get reference to map for checking
  std::map<std::string, unsigned> funcMap = instance->getFuncDecls();

  // Add to map of FuncDecls
  if (funcMap.find(funcName) == funcMap.end()) {
    if (instance->debug) {
      std::cout << "Found FuncDecl: " << funcName << " at line # "
                << funcDeclLineNum << std::endl;
    }
    instance->addFuncDeclToMap(funcName, funcDeclLineNum);
  }
  clang_disposeTokens(instance->getTU(), funcDeclToken, 1);
  return CXChildVisit_Break;
}

void KeyPointsCollector::collectCursors() {
  clang_visitChildren(rootCursor, this->VisitorFunctionCore, this);
  addBranchesToDictionary();
}

void KeyPointsCollector::printFoundBranchPoint(const CXCursorKind K) {
  std::cout << "Found branch point: " << CXSTR(clang_getCursorKindSpelling(K))
            << " at line#: " << getCurrentBranch()->branchPoint << std::endl;
}

void KeyPointsCollector::printFoundTargetPoint() {
  BranchPointInfo *currentBranch = getCurrentBranch();
  std::cout << "Found target for line branch #: " << currentBranch->branchPoint
            << " at line#: " << currentBranch->targetLineNumbers.back()
            << std::endl;
}

void KeyPointsCollector::printCursorKind(const CXCursorKind K) {
  std::cout << "Found cursor: " << CXSTR(clang_getCursorKindSpelling(K))
            << std::endl;
}

void KeyPointsCollector::outputBranchPtrTrace() {}

void KeyPointsCollector::addCompletedBranch() {
  branchPoints.push_back(branchPointStack.top());
  branchPointStack.pop();
}

void KeyPointsCollector::addBranchesToDictionary() {
  for (const BranchPointInfo &branchPoint : branchPoints) {
    std::map<unsigned, std::string> targetsAndIds;
    for (const unsigned &target : branchPoint.targetLineNumbers) {
      targetsAndIds[target] = "br_" + std::to_string(++branchCount);
    }
    branchDictionary[branchPoint.branchPoint] = targetsAndIds;
  }
}

void KeyPointsCollector::invokeValgrind() {

  // First, we need to compile the code we just parsed into an executable.

  // See what compiler we are working with on the machine.
  std::string c_compiler;
#if defined(__clang__)
  c_compiler = "clang";
#elif defined(__GNUC__)
  c_compiler = "gcc";
#endif
  if (c_compiler.empty()) {
    c_compiler = std::getenv("CC");
    if (c_compiler.empty()) {
      std::cerr << "No viable C compiler found on system!" << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  std::cout << "C compiler is: " << c_compiler << std::endl;

  // Construct compilation command.
  std::stringstream compilationCommand;
  compilationCommand << c_compiler << " " << filename << " -o " << EXE_OUT;

  // Compile
  bool compiled = static_cast<bool>(system(compilationCommand.str().c_str()));

  if (compiled == EXIT_SUCCESS) {
    std::remove(EXE_OUT);
  } else {
    std::cerr << "There was an error with compilation, exiting!" << std::endl;
    exit(EXIT_FAILURE);
  }
}