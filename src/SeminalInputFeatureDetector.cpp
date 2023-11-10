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
    // kpc.collectCursors();
    // this.kpc( "test-files/test.c", false );
    
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

    // Cursor Type
    CXType cursor_type = clang_getCursorType(current);
    CXString type_kind_spelling = clang_getTypeKindSpelling( cursor_type.kind );

    // Cursor Kind
    CXCursorKind parent_kind = clang_getCursorKind( parent );
    CXCursorKind current_kind = clang_getCursorKind( current );
    CXString parent_kind_spelling = clang_getCursorKindSpelling( parent_kind );
    CXString current_kind_spelling = clang_getCursorKindSpelling( current_kind );

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
                      << "      Type Kind: " << clang_getCString(type_kind_spelling) << "\n"
                      << "      Token: " << clang_getCString(token_spelling) << "\n"
                      << "      Line " << line << "\n\n";
        }

        instance->temp.name = clang_getCString(token_spelling);
        for ( int i = 0; i < instance->SeminalInputFeatures.size(); i++ ) {
            if ( instance->SeminalInputFeatures[ i ].name == instance->temp.name ) {
                return CXChildVisit_Break;
            }
        }
        instance->SeminalInputFeatures.push_back(instance->temp);
        instance->getDeclLocation( instance->count++ );

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

    // Cursor Type
    CXType cursor_type = clang_getCursorType(current);
    CXString type_kind_spelling = clang_getTypeKindSpelling( cursor_type.kind );

    // Cursor Kind
    CXCursorKind parent_kind = clang_getCursorKind( parent );
    CXCursorKind current_kind = clang_getCursorKind( current );
    CXString parent_kind_spelling = clang_getCursorKindSpelling( parent_kind );
    CXString current_kind_spelling = clang_getCursorKindSpelling( current_kind );

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
                      << "      Type Kind: " << clang_getCString(type_kind_spelling) << "\n"
                      << "      Token: " << clang_getCString(token_spelling) << "\n"
                      << "      Line " << line << "\n\n";
        }

        instance->temp.name = clang_getCString(token_spelling);
    }

    if ( parent_kind == CXCursor_BinaryOperator && current_kind == CXCursor_UnexposedExpr ) {
        if ( instance->debug ) {
            std::cout << "  Kind: " << clang_getCString(parent_kind_spelling) << "\n"
                      << "    Kind: " << clang_getCString(current_kind_spelling) << "\n"
                      << "      Type Kind: " << clang_getCString(type_kind_spelling) << "\n"
                      << "      Token: " << clang_getCString(token_spelling) << "\n"
                      << "      Line " << line << "\n\n";
        }

        if ( instance->temp.name != clang_getCString(token_spelling) ) {
            instance->temp.name = clang_getCString(token_spelling);
            instance->SeminalInputFeatures.push_back( instance->temp );
            instance->getDeclLocation( instance->count++ );
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

    // Cursor Type
    CXType cursor_type = clang_getCursorType(current);
    CXString type_kind_spelling = clang_getTypeKindSpelling( cursor_type.kind );

    // Cursor Kind
    CXCursorKind parent_kind = clang_getCursorKind( parent );
    CXCursorKind current_kind = clang_getCursorKind( current );
    CXString parent_kind_spelling = clang_getCursorKindSpelling( parent_kind );
    CXString current_kind_spelling = clang_getCursorKindSpelling( current_kind );

    // Cursor Location
    CXSourceLocation location = clang_getCursorLocation( current );
    unsigned line;
    clang_getExpansionLocation( location, &instance->cxFile, &line, nullptr, nullptr );
    
    // Cursor Token
    CXToken *cursor_token = clang_getToken( instance->translationUnit, location );
    CXString token_spelling = clang_getTokenSpelling( instance->translationUnit, *cursor_token );


    if ( parent_kind == CXCursor_BinaryOperator && current_kind == CXCursor_UnexposedExpr ) {
        if ( instance->debug ) {
            std::cout << "  Kind: " << clang_getCString(parent_kind_spelling) << "\n"
                      << "    Kind: " << clang_getCString(current_kind_spelling) << "\n"
                      << "      Type Kind: " << clang_getCString(type_kind_spelling) << "\n"
                      << "      Token: " << clang_getCString(token_spelling) << "\n"
                      << "      Line " << line << "\n\n";
        }

        instance->temp.name = clang_getCString(token_spelling);
        instance->SeminalInputFeatures.push_back(instance->temp);
        instance->getDeclLocation( instance->count++ );
        return CXChildVisit_Break;
    }

    // if ( instance->debug ) {
    //     std::cout << "  Kind: " << clang_getCString(parent_kind_spelling) << "\n"
    //               << "    Kind: " << clang_getCString(current_kind_spelling) << "\n"
    //               << "      Type Kind: " << clang_getCString(type_kind_spelling) << "\n"
    //               << "      Token: " << clang_getCString(token_spelling) << "\n"
    //               << "      Line " << line << "\n\n";
    // }
    // clang_getPointeeTy

    clang_disposeString( currentDisplayName );
    clang_disposeString( type_kind_spelling );
    clang_disposeString( parent_kind_spelling );
    clang_disposeString( current_kind_spelling );
    clang_disposeString( token_spelling );

    return CXChildVisit_Recurse;
}



void SeminalInputFeatureDetector::getDeclLocation( int index ) {
    // std::cout << SeminalInputFeatures[ index ].name << "--" << varDecls.at( SeminalInputFeatures[ index ].name ) << "\n\n";
    // SeminalInputFeatures[ index ].line = varDecls.at( SeminalInputFeatures[ index ].name );

    // iterator it = varDecls.find( SeminalInputFeatures[ index ].name );
    std::map<std::string, unsigned>::iterator it;
    it = varDecls.find( SeminalInputFeatures[ index ].name );

    if ( it != varDecls.end() ) {
        SeminalInputFeatures[ index ].line = it->second;
    } else {
        std::cout << "Var not found\n\n";
    }
}

void SeminalInputFeatureDetector::printSeminalInputFeatures() {
    for ( int i = 0; i < SeminalInputFeatures.size(); i++ ) {
        CXString token_spelling = clang_getTokenSpelling( translationUnit, SeminalInputFeatures[ i ].token );

        std::cout << "Line " << SeminalInputFeatures[ i ].line << ": "
                  << SeminalInputFeatures[ i ].name << "\n";
    }
}

void SeminalInputFeatureDetector::cursorFinder() {

    for ( int i = 0; i < cursorObjs.size(); i++ ) {

        CXCursorKind cursorKind = clang_getCursorKind( cursorObjs[i] );
        CXString kind_spelling = clang_getCursorKindSpelling(cursorKind);
        if ( debug ) {
            std::cout << "Kind: " << clang_getCString(kind_spelling) << "\n";
        }

        // // CXSourceLocation location = clang_getCursorLocation( cursorObjs[i] );
        // CXSourceRange cursor_range = clang_getCursorExtent( cursorObjs[i] );
        // CXString cursor_spelling = clang_getCursorSpelling( cursorObjs[i] );
        // CXFile file;
        // unsigned start_line, start_column, start_offset;
        // unsigned end_line, end_column, end_offset;

        // clang_getExpansionLocation(clang_getRangeStart(cursor_range), &file, &start_line, &start_column, &start_offset);
        // clang_getExpansionLocation(clang_getRangeEnd  (cursor_range), &file, &end_line  , &end_column  , &end_offset);
        // std::cout << " spanning lines " << start_line << "\n";
        // clang_disposeString(cursor_spelling);

        // int countBefore = count;
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

        // if ( countBefore + 1 == count ) {
        //     getDeclLocation( i );
        // }

        if ( debug ) {
            std::cout << "\n";
        }
    }

    printSeminalInputFeatures();


    // Testing
    // CXCursorKind cursorKind = clang_getCursorKind( cursor );
    //     // location = clang_getExpansionLocation( location, )

    //     switch ( cursorKind ) {
    //         case CXCursor_IfStmt:
    //             clang_visitChildren( cursor, this->ifStmtBranch, this );
    //             break;
    //         case CXCursor_ForStmt:
    //             clang_visitChildren( cursor, this->forStmtBranch, this );
    //             break;
    //         case CXCursor_WhileStmt:
    //             clang_visitChildren( cursor, this->whileStmtBranch, this );
    //             break;
    //         default:
    //             break;
    //     }
}
