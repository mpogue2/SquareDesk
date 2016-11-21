#include "macUtils.h"

// https://forum.qt.io/topic/38504/solved-qdialog-in-fullscreen-disable-os-screensaver/27
MacUtils::MacUtils() {
}

MacUtils::~MacUtils() {
}

void MacUtils::disableScreensaver() {
    CFStringRef* reasonForActivity = (CFStringRef *)CFSTR("SquareDeskPlayer active");
    IOReturn success = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
                                        kIOPMAssertionLevelOn, *reasonForActivity, &assertionID);
    if (success == kIOReturnSuccess)
    {
        //return success? TODO...
    }

    // TODO: do I also need to disable IDLE sleep?
    // TODO: does this need to be a Preference (e.g. "[ ] disable screensaver while SquareDesk is open")
}

void MacUtils::reenableScreensaver() {

    IOPMAssertionRelease(assertionID);

}
