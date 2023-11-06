// main.cpp

#include "SeminalInputFeatureDetector.h"
#include "KeyPointsCollector.h"
#include <iostream>
#include <fstream>

int main( int argc, char *argv[] )
{
    // Call SeminalInputFeatureDetector constructor
    SeminalInputFeatureDetector detector( "test-files/test.c", true );

    detector.CursorFinder();



    // // Testing
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
    
    // // Recursively visit children
    // clang_visitChildren( cursor,
    //     []( CXCursor current, CXCursor parent, CXClientData clientData ) {

    //         // Example code
    //         // Allocate a CXString representing the name of the current cursor
    //         CXString currentDisplayName = clang_getCursorDisplayName(current);

    //         CXType cursor_type = clang_getCursorType(current);
    //         CXString type_kind_spelling = clang_getTypeKindSpelling(cursor_type.kind);

    //         CXCursorKind cursor_kind = clang_getCursorKind(current);
    //         CXString kind_spelling = clang_getCursorKindSpelling(cursor_kind);

    //         // // Print the char* value of currentDisplayName
    //         // std::cout << "Visiting element: " << clang_getCString(currentDisplayName) << "\n"
    //         //           << "  Type Kind: " << clang_getCString(type_kind_spelling) << "\n"
    //         //           << "  Kind: " << clang_getCString(kind_spelling);
            
    //         if ( cursor_kind == CXCursor_ForStmt || cursor_kind == CXCursor_IfStmt ) {
    //             // Print the char* value of currentDisplayName
    //             std::cout << "Visiting element: " << clang_getCString(currentDisplayName) << "\n"
    //                       << "  Type Kind: " << clang_getCString(type_kind_spelling) << "\n"
    //                       << "  Kind: " << clang_getCString(kind_spelling) << "\n\n";

                
    //         }
    
    //         // Free currentDisplayName
    //         clang_disposeString(currentDisplayName);

    //         if (cursor_type.kind == CXType_Pointer ||         // If cursor_type is a pointer
    //             cursor_type.kind == CXType_LValueReference || // or an LValue Reference (&)
    //             cursor_type.kind == CXType_RValueReference)
    //         {                                                               // or an RValue Reference (&&),
    //             CXType pointed_to_type = clang_getPointeeType(cursor_type); // retrieve the pointed-to type

    //             CXString pointed_to_type_spelling = clang_getTypeSpelling(pointed_to_type);      // Spell out the entire
    //             std::cout << "pointing to type: " << clang_getCString(pointed_to_type_spelling); // pointed-to type
    //             clang_disposeString(pointed_to_type_spelling);
    //         }
    //         else if (cursor_type.kind == CXType_Record)
    //         {
    //             CXString type_spelling = clang_getTypeSpelling(cursor_type);
    //             std::cout << ", namely " << clang_getCString(type_spelling);
    //             clang_disposeString(type_spelling);
    //         }
    //         std::cout << "\n\n";

    //         return CXChildVisit_Recurse;

    //     }, 
    //     nullptr
    //     );

    return EXIT_SUCCESS;

}