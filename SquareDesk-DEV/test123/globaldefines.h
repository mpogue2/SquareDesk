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

#endif // GLOBALDEFINES_H
