/* See the large comment at the top of prefs_options.h */

#ifndef PREFSMANAGER_H_INCLUDED
#define PREFSMANAGER_H_INCLUDED

#include "QSettings"

class PreferencesDialog;
class PreferencesManager {
public:
    QSettings MySettings;
    PreferencesManager();

    void populatePreferencesDialog(PreferencesDialog *prefDialog);
    void extractValuesFromPreferencesDialog(PreferencesDialog *prefDialog);
    
#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name, default)  bool Get##name(); void Set##name(bool value);
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name, default)  QString Get##name(); void Set##name(QString value);
#define CONFIG_ATTRIBUTE_STRING(control, name, default) QString Get##name(); void Set##name(QString value);
#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) bool Get##name(); void Set##name(bool value);
#define CONFIG_ATTRIBUTE_INT(control, name, default) int Get##name(); void Set##name(int value);
#define CONFIG_ATTRIBUTE_COMBO(control, name, default) int Get##name(); void Set##name(int value);
    #include "prefs_options.h"
#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_INT
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
};

#endif // ifndef PREFSMANAGER_H_INCLUDED
