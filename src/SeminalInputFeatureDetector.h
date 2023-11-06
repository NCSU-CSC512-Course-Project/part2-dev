// SeminalInputFeatureDetector.h

#include <string>
#include <vector>
#include <clang-c/Index.h>

class SeminalInputFeatureDetector {

    // Name of file we are analyzing
    std::string filename;

    // CXFile object of analysis file.
    CXFile cxFile;

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

    static CXChildVisitResult ifStmtBranch(CXCursor current, CXCursor parent, CXClientData clientData);
    static CXChildVisitResult forStmtBranch(CXCursor current, CXCursor parent, CXClientData clientData);
    static CXChildVisitResult whileStmtBranch(CXCursor current, CXCursor parent, CXClientData clientData);

    // Information struct for a Seminal Input Feature
    struct SeminalInputFeature {
        std::string name;
        unsigned line;
    };

    // Vector of completed branch points
    std::vector<SeminalInputFeature> SeminalInputFeatures;
    // Number of SeminalInputFeatures in the vector
    unsigned count;

    SeminalInputFeature temp;

    // Function to print the Seminal Input Features
    void printSeminalInputFeatures();

    bool debug;

public:

    // SeminalInputFeatureDetector(const std::string &fileName);
    SeminalInputFeatureDetector( const std::string &fileName, bool debug = false );

    // Looks through the vector of CXCursors
    void CursorFinder();

};