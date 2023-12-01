// KeyPointsCollector.cpp
// ~~~~~~~~~~~~~~~~~~~~~~
// Implementation of KeyPointsCollector interface.
#include "KeyPointsCollector.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Common.h"
// Ctor Implementation
KeyPointsCollector::KeyPointsCollector(const std::string &filename, bool debug)
    : filename(std::move(filename)), debug(debug) {
  // Check if file exists
  std::ifstream file(filename);
  if (file.good()) {
    file.close();

    // Format the file
    std::stringstream formatCommand;
    formatCommand << "clang-format -i --style=file:file_format_style "
                  << filename;
    system(formatCommand.str().c_str());

    // Remove include directives. We do this before parsing the translation unit
    // as LibClang with parse ALL included files. For the sake of this project,
    // we are only looking at user defined functions, so we dont need to parse
    // any included files.
    removeIncludeDirectives();

    // If good, try to parse the translation unit.
    translationUnit = clang_createTranslationUnitFromSourceFile(
        KPCIndex, filename.c_str(), 0, nullptr, 0, nullptr);

    // Reinsert the include directives. After the TU has been parsed, we can
    // reinsert them before transformation.
    reInsertIncludeDirectives();

    // Check if parsed properly
    if (translationUnit == nullptr) {
      std::cerr
          << "There was an error parsing the translation unit! Exiting...\n";
      exit(EXIT_FAILURE);
    }
    std::cout << "Translation unit for file: " << filename
              << " successfully parsed.\n";

    // Init cursor and branch count
    rootCursor = clang_getTranslationUnitCursor(translationUnit);
    cxFile = clang_getFile(translationUnit, filename.c_str());
    branchCount = 0;
    // Traverse
  } else {
    std::cerr << "File with name: " << filename
              << ", does not exist! Exiting...\n";
    exit(EXIT_FAILURE);
  }
}

KeyPointsCollector::~KeyPointsCollector() {
  clang_disposeTranslationUnit(translationUnit);
}

void KeyPointsCollector::removeIncludeDirectives() {
  std::ifstream file(filename);
  std::ofstream tempFile("temp.c");
  std::string currentLine;
  const std::string includeStr("#include");
  unsigned lineNum = 1;

  if (file.good() && tempFile.good()) {
    while (getline(file, currentLine)) {
      if (currentLine.find(includeStr) == 0) {
        addIncludeDirective(lineNum++, currentLine);
        continue;
      }
      lineNum++;
      tempFile << currentLine << '\n';
    }
  }
  std::remove(filename.c_str());
  std::rename("temp.c", filename.c_str());
}

void KeyPointsCollector::reInsertIncludeDirectives() {
  std::ifstream file(filename);
  std::ofstream tempFile("temp.c");
  std::string currentLine;
  const std::string includeStr("#include");
  unsigned lineNum = 1;

  if (file.good() && tempFile.good()) {
    while (getline(file, currentLine)) {
      if (MAP_FIND(includeDirectives, lineNum)) {
        tempFile << includeDirectives[lineNum] << '\n';
      }
      lineNum++;
      tempFile << currentLine << '\n';
    }
  }

  std::remove(filename.c_str());
  std::rename("temp.c", filename.c_str());
}

bool KeyPointsCollector::isBranchPointOrCallExpr(const CXCursorKind K) {
  switch (K) {
  case CXCursor_IfStmt:
  case CXCursor_ForStmt:
  case CXCursor_DoStmt:
  case CXCursor_WhileStmt:
  case CXCursor_SwitchStmt:
  case CXCursor_CallExpr:
    return true;
  default:
    return false;
  }
}

bool KeyPointsCollector::isFunctionPtr(const CXCursor C) {
  CXType cursorType = clang_getCursorType(C);
  if (cursorType.kind == CXType_Pointer) {
    CXType ptrType = clang_getPointeeType(cursorType);
    return ptrType.kind == CXType_FunctionProto;
  }
  return false;
}

bool KeyPointsCollector::checkChildAgainstStackTop(CXCursor child) {
  unsigned childLineNum;
  unsigned childColNum;
  BranchPointInfo *currBranch = getCurrentBranch();
  CXSourceLocation childLoc = clang_getCursorLocation(child);
  clang_getSpellingLocation(childLoc, getCXFile(), &childLineNum, &childColNum,
                            nullptr);

  if (inCurrentFunction(childLineNum)) {
    if (childLineNum > currBranch->compoundEndLineNum ||
        (childLineNum == currBranch->compoundEndLineNum &&
         childColNum > currBranch->compoundEndColumnNum)) {
      getCurrentBranch()->addTarget(childLineNum + includeDirectives.size());
      if (debug) {
        printFoundTargetPoint();
      }
      return true;
    } else {
      return false;
    }
  }
  return false;
}

CXChildVisitResult KeyPointsCollector::VisitorFunctionCore(CXCursor current,
                                                           CXCursor parent,
                                                           CXClientData kpc) {
  // Retrieve required data from call
  KeyPointsCollector *instance = static_cast<KeyPointsCollector *>(kpc);
  const CXCursorKind currKind = clang_getCursorKind(current);
  const CXCursorKind parrKind = clang_getCursorKind(parent);

  // If it is a call expression, recurse on children with special visitor
  if (currKind == CXCursor_CallExpr) {
    clang_visitChildren(parent, &KeyPointsCollector::VisitCallExpr, kpc);
    return CXChildVisit_Continue;
  }

  // If parent a branch point, and current is a compount statement,
  // warm up the KPC for analysis of said branch.
  if (instance->isBranchPointOrCallExpr(parrKind) &&
      currKind == CXCursor_CompoundStmt) {
    // Push new point to the stack and retrieve location
    instance->addCursor(parent);
    instance->pushNewBranchPoint();
    CXSourceLocation loc = clang_getCursorLocation(parent);
    clang_getSpellingLocation(loc, instance->getCXFile(),
                              instance->getCurrentBranch()->getBranchPointOut(),
                              nullptr, nullptr);
    // Increment the branch point by the amount of include directives we used.
    instance->getCurrentBranch()->branchPoint +=
        instance->includeDirectives.size();

    // Debug routine
    if (instance->debug) {
      instance->printFoundBranchPoint(parrKind);
    }

    // Visit first child of compound to get target.
    clang_visitChildren(current, &KeyPointsCollector::VisitCompoundStmt, kpc);

    // Save end of parent statement
    BranchPointInfo *currBranch = instance->getCurrentBranch();
    CXSourceLocation parentEnd =
        clang_getRangeEnd(clang_getCursorExtent(parent));
    clang_getSpellingLocation(parentEnd, instance->getCXFile(),
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
    clang_visitChildren(current, &KeyPointsCollector::VisitFuncDecl, kpc);
  }

  // If check to see if it is a VarDecl
  if (currKind == CXCursor_VarDecl || currKind == CXCursor_ParmDecl) {
    clang_visitChildren(parent, &KeyPointsCollector::VisitVarOrParamDecl, kpc);
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
    std::cerr << "Compound statement visitor called when cursor is not "
                 "compound stmt!\n";
    exit(EXIT_FAILURE);
  }
  // Get line number of first child
  unsigned targetLineNumber;
  CXSourceLocation loc = clang_getCursorLocation(current);
  clang_getSpellingLocation(loc, instance->getCXFile(), &targetLineNumber,
                            nullptr, nullptr);

  // Append line number to targets
  instance->getCurrentBranch()->addTarget(targetLineNumber +
                                          instance->includeDirectives.size());
  if (instance->debug) {
    instance->printFoundTargetPoint();
  }
  return CXChildVisit_Continue;
}

CXChildVisitResult KeyPointsCollector::VisitCallExpr(CXCursor current,
                                                     CXCursor parent,
                                                     CXClientData kpc) {
  KeyPointsCollector *instance = static_cast<KeyPointsCollector *>(kpc);

  CXSourceLocation callExprLoc = clang_getCursorLocation(current);
  CXToken *calleeNameTok = clang_getToken(instance->getTU(), callExprLoc);
  CXString calleeNameStr =
      clang_getTokenSpelling(instance->getTU(), *calleeNameTok);
  std::string calleeName(clang_getCString(calleeNameStr));

  if (MAP_FIND(instance->funcDeclsString, calleeName)) {
    unsigned callLocLine;
    clang_getSpellingLocation(callExprLoc, instance->getCXFile(), &callLocLine,
                              nullptr, nullptr);
    instance->addCall(callLocLine + instance->includeDirectives.size(),
                      calleeName);
    // Possibly set recursion flag for function being called.

    if (instance->getFunctionByName(calleeName)->isInBody(callLocLine)) {
      instance->getFunctionByName(calleeName)->setRecursive();
    }
    clang_disposeTokens(instance->getTU(), calleeNameTok, 1);
    clang_disposeString(calleeNameStr);

    return CXChildVisit_Break;
  } else if (MAP_FIND(instance->funcPtrs, calleeName)) {
    unsigned callLocLine;
    clang_getSpellingLocation(callExprLoc, instance->getCXFile(), &callLocLine,
                              nullptr, nullptr);
    instance->addCall(callLocLine + instance->includeDirectives.size(),
                      instance->funcPtrs[calleeName]);
    clang_disposeTokens(instance->getTU(), calleeNameTok, 1);
    clang_disposeString(calleeNameStr);
    return CXChildVisit_Break;
  }
  clang_disposeString(calleeNameStr);
  clang_disposeTokens(instance->getTU(), calleeNameTok, 1);

  return CXChildVisit_Recurse;
}

CXChildVisitResult KeyPointsCollector::VisitFuncPtr(CXCursor current,
                                                    CXCursor parent,
                                                    CXClientData kpc) {
  KeyPointsCollector *instance = static_cast<KeyPointsCollector *>(kpc);

  // Get name of ptr
  CXSourceLocation funcPtrLoc = clang_getCursorLocation(parent);
  CXToken *funcPtrTok = clang_getToken(instance->getTU(), funcPtrLoc);
  CXString funcPtrStr = clang_getTokenSpelling(instance->getTU(), *funcPtrTok);
  std::string funcPtrName(clang_getCString(funcPtrStr));

  // If no key in map, add a nullptr
  if (!(MAP_FIND(instance->funcPtrs, funcPtrName)) &&
      instance->currFuncPtrId.empty()) {
    instance->currFuncPtrId = funcPtrName;
  }

  // Get name of pointee
  CXSourceLocation funcPteeLoc = clang_getCursorLocation(current);
  CXToken *funcPteeTok = clang_getToken(instance->getTU(), funcPteeLoc);
  CXString funcPteeStr =
      clang_getTokenSpelling(instance->getTU(), *funcPteeTok);
  std::string funcPteeName(clang_getCString(funcPteeStr));

  // Check pointee points to function.
  if (instance->getFunctionByName(funcPteeName) != nullptr) {
    instance->funcPtrs[instance->currFuncPtrId] = funcPteeName;
    instance->currFuncPtrId.clear();
    clang_disposeTokens(instance->getTU(), funcPtrTok, 1);
    clang_disposeTokens(instance->getTU(), funcPteeTok, 1);
    clang_disposeString(funcPtrStr);
    clang_disposeString(funcPteeStr);
    return CXChildVisit_Break;
  }

  clang_disposeString(funcPtrStr);
  clang_disposeString(funcPteeStr);
  clang_disposeTokens(instance->getTU(), funcPtrTok, 1);
  clang_disposeTokens(instance->getTU(), funcPteeTok, 1);

  return CXChildVisit_Recurse;
}

CXChildVisitResult KeyPointsCollector::VisitVarOrParamDecl(CXCursor current,
                                                           CXCursor parent,
                                                           CXClientData kpc) {
  KeyPointsCollector *instance = static_cast<KeyPointsCollector *>(kpc);

  // First retrive the line number
  unsigned varDeclLineNum;
  CXSourceLocation varDeclLoc = clang_getCursorLocation(current);
  clang_getSpellingLocation(varDeclLoc, instance->getCXFile(), &varDeclLineNum,
                            nullptr, nullptr);

  // Check if a function ptr
  if (instance->isFunctionPtr(current)) {
    clang_visitChildren(current, &KeyPointsCollector::VisitFuncPtr, kpc);
    return CXChildVisit_Break;
  }

  // Get token and its spelling
  CXToken *varDeclToken = clang_getToken(instance->getTU(), varDeclLoc);
  std::string varName =
      CXSTR(clang_getTokenSpelling(instance->getTU(), *varDeclToken));

  // Get reference to map for checking
  std::map<std::string, unsigned> varMap = instance->getVarDecls();

  // Add to map of FuncDecls
  if (varMap.find(varName) == varMap.end()) {
    if (instance->debug) {
      std::cout << "Found "
                << (current.kind == CXCursor_VarDecl ? "VarDecl" : "ParamDecl")
                << ": " << varName << " at line # " << varDeclLineNum << '\n';
    }
    instance->addVarDeclToMap(varName, varDeclLineNum +
                                           instance->includeDirectives.size());
  }
  clang_disposeTokens(instance->getTU(), varDeclToken, 1);
  return CXChildVisit_Break;
}

CXChildVisitResult KeyPointsCollector::VisitFuncDecl(CXCursor current,
                                                     CXCursor parent,
                                                     CXClientData kpc) {
  KeyPointsCollector *instance = static_cast<KeyPointsCollector *>(kpc);

  // Get return type, beginning and end loc, and name.
  if (clang_getCursorKind(parent) == CXCursor_FunctionDecl) {
    CXType funcReturnType = clang_getResultType(clang_getCursorType(parent));

    CXString funcReturnTypeSpelling = clang_getTypeSpelling(funcReturnType);
    // Extent
    unsigned begLineNum, endLineNum;
    CXSourceRange funcRange = clang_getCursorExtent(parent);
    CXSourceLocation funcBeg = clang_getRangeStart(funcRange);
    CXSourceLocation funcEnd = clang_getRangeEnd(funcRange);
    clang_getSpellingLocation(funcBeg, instance->getCXFile(), &begLineNum,
                              nullptr, nullptr);
    clang_getSpellingLocation(funcEnd, instance->getCXFile(), &endLineNum,
                              nullptr, nullptr);

    // Get name
    CXToken *funcDeclToken =
        clang_getToken(instance->getTU(), clang_getCursorLocation(parent));
    std::string funcName =
        CXSTR(clang_getTokenSpelling(instance->getTU(), *funcDeclToken));

    // Add to map
    instance->addFuncDecl(std::make_shared<FunctionDeclInfo>(
        begLineNum + instance->includeDirectives.size(),
        endLineNum + instance->includeDirectives.size(), funcName,
        clang_getCString(funcReturnTypeSpelling)));
    instance->currentFunction = instance->getFunctionByName(funcName);
    if (instance->debug) {
      std::cout << "Found FunctionDecl: " << funcName << " of return type: "
                << clang_getCString(funcReturnTypeSpelling)
                << " on line #: " << begLineNum << '\n';
    }
    clang_disposeTokens(instance->getTU(), funcDeclToken, 1);
    clang_disposeString(funcReturnTypeSpelling);
  }

  return CXChildVisit_Break;
}

void KeyPointsCollector::collectCursors() {
  clang_visitChildren(rootCursor, this->VisitorFunctionCore, this);
  addBranchesToDictionary();
}

void KeyPointsCollector::printFoundBranchPoint(const CXCursorKind K) {
  std::cout << "Found branch point: " << CXSTR(clang_getCursorKindSpelling(K))
            << " at line#: " << getCurrentBranch()->branchPoint << '\n';
}

void KeyPointsCollector::printFoundTargetPoint() {
  BranchPointInfo *currentBranch = getCurrentBranch();
  std::cout << "Found target for line branch #: " << currentBranch->branchPoint
            << " at line#: " << currentBranch->targetLineNumbers.back() << '\n';
}

void KeyPointsCollector::printCursorKind(const CXCursorKind K) {
  std::cout << "Found cursor: " << CXSTR(clang_getCursorKindSpelling(K))
            << '\n';
}

void KeyPointsCollector::createDictionaryFile() {
  // Open new file for the dicitonary.
  std::ofstream dictFile(std::string(OUT_DIR + filename + ".branch_dict"));
  dictFile << "Branch Dictionary for: " << filename << '\n';
  dictFile << "-----------------------" << std::string(filename.size(), '-')
           << '\n';

  // Get branch dict ref
  const std::map<unsigned, std::map<unsigned, std::string>> &branchDict =
      getBranchDictionary();

  // Iterate over branch poitns and their targets
  for (const std::pair<unsigned, std::map<unsigned, std::string>> &BP :
       branchDict) {
    for (const std::pair<unsigned, std::string> &targets : BP.second) {
      dictFile << targets.second << ": " << filename << ", " << BP.first << ", "
               << targets.first << '\n';
    }
  }

  // Close file
  dictFile.close();
}

void KeyPointsCollector::addCompletedBranch() {
  branchPoints.push_back(branchPointStack.top());
  branchPointStack.pop();
}

void KeyPointsCollector::addBranchesToDictionary() {
  for (std::vector<BranchPointInfo>::reverse_iterator branchPoint =
           branchPoints.rbegin();
       branchPoint != branchPoints.rend(); branchPoint++) {
    std::map<unsigned, std::string> targetsAndIds;
    for (const unsigned &target : branchPoint->targetLineNumbers) {
      targetsAndIds[target] = "br_" + std::to_string(++branchCount);
    }
    branchDictionary[branchPoint->branchPoint] = targetsAndIds;
  }
}

void KeyPointsCollector::transformProgram() {
  // First, open original file for reading, and modified file for writing.
  std::ifstream originalProgram(filename);
  std::ofstream modifiedProgram(MODIFIED_PROGAM_OUT);

  // Check files opened successfully
  if (originalProgram.good() && modifiedProgram.good()) {
    // First write the header to the output file
    modifiedProgram << TRANSFORM_HEADER;

    // Keep track of line numbers
    unsigned lineNum = 1;

    // Containers for current line and new lines
    std::string currentLine;

    // Current function being analyzed
    std::shared_ptr<FunctionDeclInfo> currentTransformFunction = nullptr;

    // Amount of branches within current function.
    int branchCountCurrFunc;

    // Get ref to function decls
    std::map<unsigned, std::shared_ptr<FunctionDeclInfo>> funcDecls =
        getFuncDecls();

    // Get Ref to function calls.
    std::map<unsigned, std::string> funcCalls = getFuncCalls();

    // Get ref to branch dictionary
    std::map<unsigned, std::map<unsigned, std::string>> branchDict =
        getBranchDictionary();

    // Keep track of line branch point line numbers that have been encountered.
    std::vector<unsigned> foundPoints;

    // Core iteration over original program
    while (getline(originalProgram, currentLine)) {
      // If previous line is a function def/decl, insert the branch points for
      // that function and set current function.
      if (MAP_FIND(funcDecls, lineNum - 1)) {
        currentTransformFunction = funcDecls[lineNum - 1];

        // Declare a pointer to the current function within the function scope
        // to handle recursive calls.
        if (currentTransformFunction->name.compare("main") &&
            currentTransformFunction->recursive &&
            currentTransformFunction->type != "void") {
          modifiedProgram << DECLARE_FUNC_PTR(currentTransformFunction);
        }

        foundPoints.clear();
        branchCountCurrFunc = 0;
        insertFunctionBranchPointDecls(
            modifiedProgram, currentTransformFunction, &branchCountCurrFunc);
      }

      // If we have a current function AND the previous line is the end of said
      // function, create a pointer pointer for that function so we can access
      // it for logging purposes. Also, the current function shouldn't be main.
      if (currentTransformFunction != nullptr &&
          (lineNum - 1) == currentTransformFunction->endLoc &&
          currentTransformFunction->name.compare("main")) {
        modifiedProgram << DECLARE_FUNC_PTR(currentTransformFunction);
      }

      // If the previous line was a branch point, set the branch
      if (MAP_FIND(branchDict, lineNum - 1)) {
        modifiedProgram << SET_BRANCH(foundPoints.size());
        foundPoints.push_back(lineNum - 1);
      }

      // Iterate over found branch points and look for targets

      // This vector holds not the location of the branch point, but the INDEX
      // of the branches location in the foundPoints vector above. This is done
      // as the index is how we access BRANCH_X in the transformed program.
      std::vector<unsigned> foundPointsIdxCurrentLine;

      if (!foundPoints.empty()) {
        for (int idx = foundPoints.size() - 1; idx >= 0; --idx) {
          // Get targets for BP
          std::map<unsigned, std::string> targets =
              branchDict[foundPoints[idx]];

          // If target exists for any branch point, add to list for the current
          // line number.
          if (MAP_FIND(targets, lineNum)) {
            foundPointsIdxCurrentLine.push_back(idx);
          }
        }
      }

      // After targets are found, insert proper logging logic into modified
      // program.
      switch (foundPointsIdxCurrentLine.size()) {
        // None? Get outta there.
      case 0:
        break;
      // If only one target for the line number, check to see if all
      // successive branch points have NOT been set, this prevents unecessary
      // logging after the exit of something like an if block. e.g if the
      // target is from BRANCH_0,  ensure that BRANCH_1...BRANCH_N arent set =
      // 1;
      case 1: {
        // If branch actually has successive points, then construct a
        // conditional.
        if (foundPointsIdxCurrentLine[0] + 1 < branchCountCurrFunc) {
          modifiedProgram << "if (";
          for (int successive = foundPointsIdxCurrentLine[0] + 1;
               successive < branchCountCurrFunc; successive++) {
            modifiedProgram << "!BRANCH_" << successive;
            if (branchCountCurrFunc - successive > 1)
              modifiedProgram << " && ";
          }
          modifiedProgram
              << ") LOG(\""
              << branchDict[foundPoints[foundPointsIdxCurrentLine[0]]][lineNum]
              << "\");";
        }
        // If not, just log it.
        else {
          modifiedProgram
              << "LOG(\""
              << branchDict[foundPoints[foundPointsIdxCurrentLine[0]]][lineNum]
              << "\");";
        }
        break;
      }
      // If two targets for the current line number, we can insert a simple if
      // else block
      case 2: {
        modifiedProgram
            << "if (BRANCH_" << foundPointsIdxCurrentLine[0] << ") {LOG(\""
            << branchDict[foundPoints[foundPointsIdxCurrentLine[0]]][lineNum]
            << "\")} else {LOG(\""
            << branchDict[foundPoints[foundPointsIdxCurrentLine[1]]][lineNum]
            << "\")}";
        break;
      }
      // Default is more than 2, in this case, we need to insert a proper if,
      // else if, else chain for all the targets available for the current
      // line number.
      default: {
        // Insert initial if block
        modifiedProgram
            << "if (BRANCH_" << foundPointsIdxCurrentLine[0] << ") {LOG(\""
            << branchDict[foundPoints[foundPointsIdxCurrentLine[0]]][lineNum]
            << "\")}";

        // Insert else if blocks for all branches before the last.
        for (int successive = 1;
             successive < foundPointsIdxCurrentLine.size() - 1; successive++) {
          modifiedProgram
              << " else if (BRANCH_" << foundPointsIdxCurrentLine[successive]
              << ") {LOG(\""
              << branchDict[foundPoints[foundPointsIdxCurrentLine[successive]]]
                           [lineNum]
              << "\")}";
        }

        // Insert final else for the last branch point.
        modifiedProgram
            << "else {LOG(\""
            << branchDict[foundPoints[foundPointsIdxCurrentLine
                                          [foundPointsIdxCurrentLine.size() -
                                           1]]][lineNum]
            << "\")}";

      } break;
      }

      // Check to see if we encountered a call expr last. If branch target and
      // call on the same line, it seems more intuitive for the branch log to
      // come before the function log. e.g br_here THEN call func_0x1010101010.
      if (MAP_FIND(funcCalls, lineNum)) {
        modifiedProgram << "LOG_PTR(" << funcCalls[lineNum] << "_PTR"
                        << ");\n";
      }

      // Write line
      modifiedProgram << WRITE_LINE(currentLine);
      lineNum++;
    }

    // Close files
    originalProgram.close();
    modifiedProgram.close();

  } else {
    std::cerr << "Error opening program files for transformation!\n";
    exit(EXIT_FAILURE);
  }
}

void KeyPointsCollector::insertFunctionBranchPointDecls(
    std::ofstream &program, std::shared_ptr<FunctionDeclInfo> function,
    int *branchCount) {
  // Iterate over range of function and check for branching points.
  for (int lineNum = function->defLoc; lineNum < function->endLoc; lineNum++) {
    if (MAP_FIND(getBranchDictionary(), lineNum)) {
      program << DECLARE_BRANCH((*branchCount)++);
    }
  }
  program << '\n';
}

void KeyPointsCollector::compileModified() {
  // See what compiler we are working with on the machine.
#if defined(__clang__)
  std::string c_compiler("clang");
#elif defined(__GNUC__)
  std::string c_compiler("gcc");
#endif
  if (c_compiler.empty()) {
    c_compiler = std::getenv("CC");
    if (c_compiler.empty()) {
      std::cerr << "No viable C compiler found on system!\n";
      exit(EXIT_FAILURE);
    }
  }
  std::cout << "C compiler is: " << c_compiler << '\n';

  // Ensure that the modified program exists
  if (!static_cast<bool>(std::ifstream(MODIFIED_PROGAM_OUT).good())) {
    std::cerr << "Transformed program has not been created yet!\n";
    exit(EXIT_FAILURE);
  }

  // Construct compilation command.
  std::stringstream compilationCommand;
  compilationCommand << c_compiler << " -w -O0 " << MODIFIED_PROGAM_OUT
                     << " -o " << EXE_OUT;

  // Compile
  bool compiled = static_cast<bool>(system(compilationCommand.str().c_str()));

  // Check if compiled properly
  if (compiled == EXIT_SUCCESS) {
    std::cout << "Compilation Successful" << '\n';
  } else {
    std::cerr << "There was an error with compilation, exiting!\n";
    exit(EXIT_FAILURE);
  }
}

void KeyPointsCollector::invokeValgrind() {
  // First compile the original program
#if defined(__clang__)
  std::string c_compiler("clang");
#elif defined(__GNUC__)
  std::string c_compiler("gcc");
#endif
  if (c_compiler.empty()) {
    c_compiler = std::getenv("CC");
    if (c_compiler.empty()) {
      std::cerr << "No viable C compiler found on system!\n";
      exit(EXIT_FAILURE);
    }
  }

  // Check we acutally have a file to compile
  if (!static_cast<bool>(std::ifstream(filename).good())) {
    std::cerr << "No program to compile!\n";
    exit(EXIT_FAILURE);
  }

  // Construct compilation command.
  std::stringstream shellCommandStream;
  shellCommandStream << c_compiler << " -O0 " << filename << " -o "
                     << ORIGINAL_EXE_OUT;

  // Compile
  bool compiled = static_cast<bool>(system(shellCommandStream.str().c_str()));

  // Check if compiled properly
  if (compiled == EXIT_SUCCESS) {
    std::cout << "Compilation Successful" << '\n';
  } else {
    std::cerr << "There was an error with compilation, exiting!\n";
    exit(EXIT_FAILURE);
  }

  // Clear stream and construt valgrind command
  const std::string valgrindLogFile(OUT_DIR + filename + ".VALGRIND_OUT");
  shellCommandStream.str("");
  shellCommandStream.clear();
  shellCommandStream << "valgrind --tool=callgrind --dump-instr=yes --log-file="
                     << valgrindLogFile << " " << ORIGINAL_EXE_OUT;

  // Run valgrind
  bool valgrind = static_cast<bool>(system(shellCommandStream.str().c_str()));

  // If successful, invoke python script to collect executed number of
  // instructions.
  if (valgrind == EXIT_SUCCESS) {
    std::cout << "Valgrind invoked successfully\n";
    // First remove the call grind files generated.
    system("rm -r callgrind*");
    // Invoke python script
    shellCommandStream.str("");
    shellCommandStream.clear();
    shellCommandStream << "python3 " << VALGRIND_PARSER << " "
                       << valgrindLogFile;
    system(shellCommandStream.str().c_str());
  }
}

void KeyPointsCollector::executeToolchain() {
  collectCursors();
  createDictionaryFile();
  transformProgram();
  compileModified();
  std::cout << "\nToolchain was successful, the branch dicitonary, modified "
               "file, and executable have been written to the "
            << OUT_DIR << " directory \n";

  char decision;
  std::cout << "\nWould you like to invoke Valgrind? (y/n) ";
  std::cin >> decision;
  if (decision == 'y') {
    invokeValgrind();
  }

  std::cout << "\nWould you like to out put the branch pointer trace for the "
               "program? (y/n) ";
  std::cin >> decision;
  if (decision == 'y') {
    system(EXE_OUT.c_str());
  }
}

std::string KeyPointsCollector::getBPTrace() {
  collectCursors();
  transformProgram();
  compileModified();
  std::vector<char> buffer(128);
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(EXE_OUT.c_str(), "r"),
                                                pclose);
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}
