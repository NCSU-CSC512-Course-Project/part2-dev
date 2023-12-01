/** 
 * SeminalInputFeatureDetector.h
 * @author Carter Fultz (cmfultz)
*/

#include "KeyPointsCollector.h"
#include <string>
#include <vector>
#include <map>
#include <clang-c/Index.h>

class SeminalInputFeatureDetector {

    // Name of file we are analyzing
    std::string filename;

    // CXFile object of analysis file.
    CXFile cxFile;

  KeyPointsCollector *kpc; 

    // Vector of CXCursor objs pointing to node of interest
    std::vector<CXCursor> cursorObjs;

    // Instance of KeyPointsCollector
    // KeyPointsCollector kpc;

    // Index - set of translation units that would be linked together as an exe
    // Ref ^ https://clang.llvm.org/docs/LibClang.html
    // Marked as static as there could be multiple KPC objects for files that need
    // to be grouped together.
    inline static CXIndex index = clang_createIndex(0, 0);

    // Top level translation unit of the source file.
    CXTranslationUnit translationUnit;

    // Recursive searches through branches
    static CXChildVisitResult ifStmtBranch(CXCursor current, CXCursor parent, CXClientData clientData);
    static CXChildVisitResult forStmtBranch(CXCursor current, CXCursor parent, CXClientData clientData);
    static CXChildVisitResult whileStmtBranch(CXCursor current, CXCursor parent, CXClientData clientData);

    // Information struct for a Seminal Input Feature
    struct SeminalInputFeature {
        std::string name;
        unsigned line;
        std::string type;
    };

    // *All obtained from KeyPointsCollector*
    // Vector of completed branch points
    std::vector<SeminalInputFeature> SeminalInputFeatures;
    // Number of SeminalInputFeatures in the vector
    unsigned count;
    // Map of variable names (VarDecls) mapped to their declaration location
    std::map<std::string, unsigned> varDecls;

    // Temp value for storing Seminal Input Features
    SeminalInputFeature temp;

    // Updates the Seminal Input Features with their declare locations
    void getDeclLocation( std::string name, int index, std::string type );
    
    // Function to print the Seminal Input Features
    void printSeminalInputFeatures();

    // debug boolean value used to print out statements
    bool debug;

public:

    // SeminalInputFeatureDetector(const std::string &fileName);
    SeminalInputFeatureDetector( const std::string &fileName, bool debug = false );

    // Looks through the vector of CXCursors
    void cursorFinder();

};
