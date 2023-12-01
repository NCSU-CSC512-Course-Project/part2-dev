/** 
 * main.cpp
 * @author Carter Fultz (cmfultz)
*/

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

    // Debugger on or off
    bool debug;
    std::string debugStr;
    std::cout << "Would you like the debugger on? (y/n): ";
    std::cin >> debugStr;

    if ( debugStr == "y" ) {
        debug = true;
    } else if ( debugStr == "n" ) {
        debug = false;
    }

    // Call SeminalInputFeatureDetector constructor
    SeminalInputFeatureDetector detector( filename, debug );
    detector.cursorFinder();

    return EXIT_SUCCESS;

}
