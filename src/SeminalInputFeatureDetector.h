// SeminalInputFeatureDetector.h

#include <string>
#include <vector>
#include <clang-c/Index.h>

class SeminalInputFeatureDetector {

    // Name of file we are analyzing
    const std::string filename;

    // Vector of CXCursor objs pointing to node of interest
    std::vector<CXCursor> cursorObjs;

public:

    SeminalInputFeatureDetector(const std::string &fileName);

    void CursorFinder( CXCursor cursor );

};