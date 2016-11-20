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
}

void MacUtils::reenableScreensaver() {

    IOPMAssertionRelease(assertionID);

}
