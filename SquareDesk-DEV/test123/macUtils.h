#import <IOKit/pwr_mgt/IOPMLib.h>

#ifndef MACUTILS_H
#define MACUTILS_H

class MacUtils
{

public:
    MacUtils();
    ~MacUtils();

    void disableScreensaver();
    void reenableScreensaver();

private:
    IOPMAssertionID assertionID;

};
#endif
