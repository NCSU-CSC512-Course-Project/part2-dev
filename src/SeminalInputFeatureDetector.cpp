// SeminalInputFeatureDetector.cpp

#include "SeminalInputFeatureDetector.h"

#include <clang-c/Index.h>
#include <iostream>
#include <fstream>

SeminalInputFeatureDetector::SeminalInputFeatureDetector(const std::string &filename) {

    // Obtained from part 1, KeyPointsCollector.cpp
    cursorObjs = getCursorObjs();
    CXCursorKind cursorKind;
    CXSourceLocation location;
    // CXTranslationUnit translationUnit = clang_parseTranslationUnit()
    
    for ( int i = 0; i < cursorObjs.size(); i++ ) {

        cursorKind = clang_getCursorKind( cursorObjs[i] );
        CursorFinder( cursorObjs[i] );
        // clang_visitChildren()
        

        // location = clang_getExpansionLocation( location, )
        
        // if ( cursorKind == CXCursor_IfStmt ) {



        // } else if ( cursorKind == CXCursor_ForStmt ) {



        // } else if ( cursorKind == CXCursor_WhileStmt ) {



        // }

    }

}

void SeminalInputFeatureDetector::CursorFinder( CXCursor cursor ) {

    clang_visitChildren( cursor,
    []( CXCursor currentCursor, CXCursor parent, CXClientData clientData ) {

        // Example code
        // Allocate a CXString representing the name of the current cursor
        CXString currentDisplayName = clang_getCursorDisplayName(currentCursor);

        // Print the char* value of currentDisplayName
        std::cout << "Visiting element " << clang_getCString(currentDisplayName) << "\n";

        // Free currentDisplayName
        clang_disposeString(currentDisplayName);

        return CXChildVisit_Recurse;

    }, 
    nullptr
    );

}

int main() {

    // Testing
    // CXIndex index = clang_createIndex(0, 0); // Create index
    // CXTranslationUnit unit = clang_parseTranslationUnit(
    //     index,
    //     "test-files/test.c", nullptr, 0,
    //     nullptr, 0,
    //     CXTranslationUnit_None); // Parse "test.c"

    // if (unit == nullptr)
    // {
    //     std::cerr << "Unable to parse translation unit. Quitting.\n";
    //     return 0;
    // }
    // CXCursor cursor = clang_getTranslationUnitCursor(unit); // Obtain a cursor at the root of the translation unit

    return EXIT_SUCCESS;
}