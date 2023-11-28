// SeminalInputFeatureDetector.cpp

#include "SeminalInputFeatureDetector.h"
#include "KeyPointsCollector.h"

#include <clang-c/Index.h>
#include <iostream>
#include <fstream>
#include <iterator>

SeminalInputFeatureDetector::SeminalInputFeatureDetector( const std::string &filename, bool debug )
    : filename(std::move(filename)), debug(debug) {

    // Call KeyPointsCollector constructor
    KeyPointsCollector kpc( filename, false );
    
    // Obtained from part 1, KeyPointsCollector.cpp
    cursorObjs = kpc.getCursorObjs();
    varDecls = kpc.getVarDecls();
    count = 0;

    translationUnit =
        clang_parseTranslationUnit( index, filename.c_str(), nullptr, 0,
                                   nullptr, 0, CXTranslationUnit_None );
    cxFile = clang_getFile( translationUnit, filename.c_str() );
}



CXChildVisitResult SeminalInputFeatureDetector::ifStmtBranch(CXCursor current,
                                                      CXCursor parent,
                                                      CXClientData clientData) {

    // instance of SeminalInputFeatureDetector
    SeminalInputFeatureDetector *instance = static_cast<SeminalInputFeatureDetector *>(clientData);

    // Allocate a CXString representing the name of the current cursor
    CXString currentDisplayName = clang_getCursorDisplayName(current);

    // Cursor Kind
    CXCursorKind parent_kind = clang_getCursorKind( parent );
    CXCursorKind current_kind = clang_getCursorKind( current );
    CXString parent_kind_spelling = clang_getCursorKindSpelling( parent_kind );
    CXString current_kind_spelling = clang_getCursorKindSpelling( current_kind );

    // Cursor Type
    CXType cursor_type = clang_getCursorType(current);
    CXString type_kind_spelling = clang_getTypeSpelling( cursor_type );

    // Cursor Location
    CXSourceLocation location = clang_getCursorLocation( current );
    unsigned line;
    clang_getExpansionLocation( location, &instance->cxFile, &line, nullptr, nullptr );

    // Cursor Token
    CXToken *cursor_token = clang_getToken( instance->translationUnit, location );
    CXString token_spelling = clang_getTokenSpelling( instance->translationUnit, *cursor_token );


    if ( parent_kind == CXCursor_IfStmt && ( current_kind == CXCursor_UnexposedExpr 
                                          || current_kind == CXCursor_BinaryOperator ) ) {
        if ( instance->debug ) {
            std::cout << "  Kind: " << clang_getCString(parent_kind_spelling) << "\n"
                      << "    Kind: " << clang_getCString(current_kind_spelling) << "\n"
                      << "      Type: " << clang_getCString(type_kind_spelling) << "\n"
                      << "      Token: " << clang_getCString(token_spelling) << "\n"
                      << "      Line " << line << "\n\n";
        }

        instance->getDeclLocation( clang_getCString(token_spelling), instance->count++, clang_getCString(type_kind_spelling) );
        return CXChildVisit_Break;
    }

    clang_disposeString( currentDisplayName );
    clang_disposeString( type_kind_spelling );
    clang_disposeString( parent_kind_spelling );
    clang_disposeString( current_kind_spelling );
    clang_disposeString( token_spelling );

    return CXChildVisit_Continue;
}

CXChildVisitResult SeminalInputFeatureDetector::forStmtBranch(CXCursor current,
                                                      CXCursor parent,
                                                      CXClientData clientData) {

    // instance of SeminalInputFeatureDetector
    SeminalInputFeatureDetector *instance = static_cast<SeminalInputFeatureDetector *>(clientData);

    // Allocate a CXString representing the name of the current cursor
    CXString currentDisplayName = clang_getCursorDisplayName(current);

    // Cursor Kind
    CXCursorKind parent_kind = clang_getCursorKind( parent );
    CXCursorKind current_kind = clang_getCursorKind( current );
    CXString parent_kind_spelling = clang_getCursorKindSpelling( parent_kind );
    CXString current_kind_spelling = clang_getCursorKindSpelling( current_kind );

    // Cursor Type
    CXType cursor_type = clang_getCursorType(current);
    CXString type_kind_spelling = clang_getTypeSpelling( cursor_type );

    // Cursor Location
    CXSourceLocation location = clang_getCursorLocation( current );
    unsigned line;
    clang_getExpansionLocation( location, &instance->cxFile, &line, nullptr, nullptr );

    // Cursor Token
    CXToken *cursor_token = clang_getToken( instance->translationUnit, location );
    CXString token_spelling = clang_getTokenSpelling( instance->translationUnit, *cursor_token );


    if ( parent_kind == CXCursor_DeclStmt && current_kind == CXCursor_VarDecl ) {
        if ( instance->debug ) {
            std::cout << "  Kind: " << clang_getCString(parent_kind_spelling) << "\n"
                      << "    Kind: " << clang_getCString(current_kind_spelling) << "\n"
                      << "      Type: " << clang_getCString(type_kind_spelling) << "\n"
                      << "      Token: " << clang_getCString(token_spelling) << "\n"
                      << "      Line " << line << "\n\n";
        }

        instance->temp.name = clang_getCString(token_spelling);
    }

    if ( ( parent_kind == CXCursor_BinaryOperator || parent_kind == CXCursor_CallExpr ) && current_kind == CXCursor_UnexposedExpr ) {
        if ( instance->debug ) {
            std::cout << "  Kind: " << clang_getCString(parent_kind_spelling) << "\n"
                      << "    Kind: " << clang_getCString(current_kind_spelling) << "\n"
                      << "      Type: " << clang_getCString(type_kind_spelling) << "\n"
                      << "      Token: " << clang_getCString(token_spelling) << "\n"
                      << "      Line " << line << "\n\n";
        }

        if ( instance->temp.name != clang_getCString(token_spelling) ) {
            instance->getDeclLocation( clang_getCString(token_spelling), instance->count++, clang_getCString(type_kind_spelling) );
            return CXChildVisit_Break;
        }
    }

    clang_disposeString( currentDisplayName );
    clang_disposeString( type_kind_spelling );
    clang_disposeString( parent_kind_spelling );
    clang_disposeString( current_kind_spelling );
    clang_disposeString( token_spelling );

    return CXChildVisit_Recurse;
}

CXChildVisitResult SeminalInputFeatureDetector::whileStmtBranch(CXCursor current,
                                                      CXCursor parent,
                                                      CXClientData clientData) {

    // instance of SeminalInputFeatureDetector
    SeminalInputFeatureDetector *instance = static_cast<SeminalInputFeatureDetector *>(clientData);

    // Allocate a CXString representing the name of the current cursor
    CXString currentDisplayName = clang_getCursorDisplayName(current);

    // Cursor Kind
    CXCursorKind parent_kind = clang_getCursorKind( parent );
    CXCursorKind current_kind = clang_getCursorKind( current );
    CXString parent_kind_spelling = clang_getCursorKindSpelling( parent_kind );
    CXString current_kind_spelling = clang_getCursorKindSpelling( current_kind );

    // Cursor Type
    CXType cursor_type = clang_getCursorType(current);
    CXString type_kind_spelling = clang_getTypeSpelling( cursor_type );

    // Cursor Location
    CXSourceLocation location = clang_getCursorLocation( current );
    unsigned line;
    clang_getExpansionLocation( location, &instance->cxFile, &line, nullptr, nullptr );

    // Check for break statements
    if ( parent_kind == CXCursor_IfStmt && current_kind == CXCursor_BreakStmt ) {
        clang_visitChildren( parent, instance->ifStmtBranch, instance );
        return CXChildVisit_Recurse;
    }

    if ( ( parent_kind == CXCursor_BinaryOperator || parent_kind == CXCursor_CallExpr ) && current_kind == CXCursor_UnexposedExpr ) {
        // Cursor Token
        CXToken *cursor_token = clang_getToken( instance->translationUnit, location );
        if ( cursor_token ) {
            CXString token_spelling = clang_getTokenSpelling( instance->translationUnit, *cursor_token );
            if ( instance->debug ) {
                std::cout << "  Kind: " << clang_getCString(parent_kind_spelling) << "\n"
                        << "    Kind: " << clang_getCString(current_kind_spelling) << "\n"
                        << "      Type: " << clang_getCString(type_kind_spelling) << "\n"
                        << "      Token: " << clang_getCString(token_spelling) << "\n"
                        << "      Line " << line << "\n\n";
            }
            instance->getDeclLocation( clang_getCString(token_spelling), instance->count++, clang_getCString(type_kind_spelling) );
            clang_disposeString( token_spelling );
        }
        return CXChildVisit_Recurse;
    }

    clang_disposeString( currentDisplayName );
    clang_disposeString( type_kind_spelling );
    clang_disposeString( parent_kind_spelling );
    clang_disposeString( current_kind_spelling );

    return CXChildVisit_Recurse;
}



void SeminalInputFeatureDetector::getDeclLocation( std::string name, int index, std::string type ) {
    
    // Check if variable already exists in the vector of SeminalInputFeatures
    bool exists = false;
    for ( int i = 0; i < SeminalInputFeatures.size(); i++ ) {
        if ( SeminalInputFeatures[ i ].name == name ) {
            exists = true;
        }
    }

    // Check if variable exists in the map of variable declarations
    std::map<std::string, unsigned>::iterator it;
    it = varDecls.find( name );
    if ( !exists ) {
        if ( it != varDecls.end() ) {
            temp.name = it->first;
            temp.line = it->second;
            temp.type = type;
            SeminalInputFeatures.push_back( temp );
        } else if ( debug ) {
            std::cout << "Variable was not found.\n\n";
        }
    } else if ( debug ) {
        std::cout << "Variable is already accounted for.\n\n";
    }
}

void SeminalInputFeatureDetector::printSeminalInputFeatures() {
    for ( int i = 0; i < SeminalInputFeatures.size(); i++ ) {
        if ( SeminalInputFeatures[ i ].type == "FILE *" ) {
            std::cout << "Line " << SeminalInputFeatures[ i ].line << ": size of file "
                  << SeminalInputFeatures[ i ].name << "\n";
        } else {
            std::cout << "Line " << SeminalInputFeatures[ i ].line << ": "
                  << SeminalInputFeatures[ i ].name << "\n";
        }
    }
}

void SeminalInputFeatureDetector::cursorFinder() {

    // DEBUGGING: printing out list of variable declarations
    if ( debug ) {
        std::cout << "Variable Declarations: \n";
        for( const std::pair<std::string, unsigned> var : varDecls ) {
            std::cout << var.second << ": " << var.first << "\n";
        }
        std::cout << "\n";
    }

    // Looks at each of the cursor objects to recursively search through
    for ( int i = 0; i < cursorObjs.size(); i++ ) {

        CXCursorKind cursorKind = clang_getCursorKind( cursorObjs[i] );
        CXString kind_spelling = clang_getCursorKindSpelling(cursorKind);
        if ( debug ) {
            std::cout << "Kind: " << clang_getCString(kind_spelling) << "\n";
        }

        switch ( cursorKind ) {
            case CXCursor_IfStmt:
                clang_visitChildren( cursorObjs[i], this->ifStmtBranch, this );
                break;
            case CXCursor_ForStmt:
                clang_visitChildren( cursorObjs[i], this->forStmtBranch, this );
                break;
            case CXCursor_WhileStmt:
                clang_visitChildren( cursorObjs[i], this->whileStmtBranch, this );
                break;
            default:
                continue;
        }

        if ( debug ) {
            std::cout << "\n";
        }
    }

    printSeminalInputFeatures();
}
