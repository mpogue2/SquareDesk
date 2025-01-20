#ifndef GLOBALDEFINES_H
#define GLOBALDEFINES_H

// define this if you want the DarkMode tab, and all the new stuff that goes with it
#define DARKMODE
#define DARKMUSICTABNAME "Music"

// from: https://raymii.org/s/blog/Qt_Property_Macro_Q_PROPERTY_with_95_percent_less_code.html
/* Macro to define Q_PROPERTY backed by a regular value
 * QP_V = Q_Property, value (not reference) */
#define QP_V(Type, Name) \
private: \
    Q_PROPERTY(Type Name READ Name WRITE set##Name NOTIFY Name##Changed FINAL) \
    Type _##Name; \
    public: \
    Type Name() const { return _##Name; } \
    public Q_SLOTS: \
    void set##Name(Type value) { if (_##Name != value) { _##Name = value; Q_EMIT Name##Changed(value); } } \
    Q_SIGNALS: \
    void Name##Changed(Type)
// end macro


// define this is you want to experiment with the qss file Light and Dark modes
#define DEBUG_LIGHT_MODE

// define this to play with JUCE
// #define USE_JUCE

#ifdef USE_JUCE
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_DISABLE_AUDIOPROCESSOR_BEGIN_END_GESTURE_CHECKING 1

// ONE of the next two must be defined, AND it must match the way the library was compiled earlier
#define DEBUG 1
// #define NDEBUG 1

#define JUCE_PLUGINHOST_AU 1
#define JUCE_MAC 1
#endif

#endif // GLOBALDEFINES_H
