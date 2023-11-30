// main.cpp

#include "SeminalInputFeatureDetector.h"
#include "KeyPointsCollector.h"
#include <iostream>
#include <fstream>

int main( int argc, char *argv[] )
{
    // Get filename
    std::string filename;
    std::cout << "Enter a file name for analysis: ";
    std::cin >> filename;

    // Call SeminalInputFeatureDetector constructor
    SeminalInputFeatureDetector detector( filename, true );

    detector.cursorFinder();

    return EXIT_SUCCESS;

}