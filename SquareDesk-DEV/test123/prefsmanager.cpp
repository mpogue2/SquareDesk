/* See the large comment at the top of prefs_options.h */

#include "prefsmanager.h"
#include "preferencesdialog.h"
#include <QDir>

#include "common_enums.h"
#include "default_colors.h"

PreferencesManager::PreferencesManager() : MySettings()
{
}

// ------------------------------------------------------------------------
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name, default)  \
QString PreferencesManager::Get##name()                  \
{                                                        \
    QString value = MySettings.value(#name).toString();  \
    if (value.isNull())                                  \
    {                                                    \
        value = default;                                 \
        Set##name(default);                              \
    }                                                    \
    return value;                                        \
}                                                        \
                                                         \
void PreferencesManager::Set##name(QString value)        \
{                                                        \
    MySettings.setValue(#name, value);                   \
}

// ------------------------------------------------------------------------
#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name, default) \
bool PreferencesManager::Get##name()                     \
{                                                        \
    QString value = MySettings.value(#name).toString();  \
    if (value.isNull())                                  \
    {                                                    \
        value = (default) ? "true" : "false";            \
        Set##name(default);                              \
    }                                                    \
    return value == "true" || value == "checked";        \
}                                                        \
                                                         \
void PreferencesManager::Set##name(bool value)           \
{                                                        \
    MySettings.setValue(#name, value ? true : false);    \
}

// ------------------------------------------------------------------------
#define CONFIG_ATTRIBUTE_STRING(control, name, default) \
    QString PreferencesManager::Get##name()             \
{                                                       \
    QString value = MySettings.value(#name).toString(); \
    if (value.isNull())                                 \
    {                                                   \
        value = default;                                \
        MySettings.setValue(#name, value);              \
    }                                                   \
    return value;                                       \
}                                                       \
                                                        \
void PreferencesManager::Set##name(QString value)       \
{                                                       \
    MySettings.setValue(#name, value);                  \
}

// ------------------------------------------------------------------------
// Color is a string, but it's handled specially in the prefs dialog (it's actually a specially painted button there)
#define CONFIG_ATTRIBUTE_COLOR(control, name, default) \
    QString PreferencesManager::Get##name()             \
{                                                       \
    QString value = MySettings.value(#name).toString(); \
    if (value.isNull())                                 \
    {                                                   \
        value = default;                                \
        MySettings.setValue(#name, value);              \
    }                                                   \
    return value;                                       \
}                                                       \
                                                        \
void PreferencesManager::Set##name(QString value)       \
{                                                       \
    MySettings.setValue(#name, value);                  \
}

// ------------------------------------------------------------------------
#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) \
bool PreferencesManager::Get##name()                     \
{                                                        \
    QString value = MySettings.value(#name).toString();  \
    if (value.isNull())                                  \
    {                                                    \
        value = (default) ? "true" : "false";            \
        Set##name(default);                              \
    }                                                    \
    return value == "true" || value == "checked";        \
}                                                        \
                                                         \
void PreferencesManager::Set##name(bool value)           \
{                                                        \
    MySettings.setValue(#name, value ? true : false);    \
}


// ------------------------------------------------------------------------
#define CONFIG_ATTRIBUTE_INT(control, name, default) \
    int PreferencesManager::Get##name()              \
{                                                    \
    QString value = MySettings.value(#name).toString();      \
    if (value.isNull())                              \
    {                                                \
        value = default;                             \
        Set##name(default);                          \
    }                                                \
    return value.toInt();                                    \
}                                                    \
                                                     \
void PreferencesManager::Set##name(int value)        \
{                                                    \
    MySettings.setValue(#name, value);               \
}

// ------------------------------------------------------------------------
#define CONFIG_ATTRIBUTE_COMBO(control, name, default) \
int  PreferencesManager::Get##name()                   \
{                                                      \
    QVariant value = MySettings.value(#name);          \
    if (value.isNull())                                \
    {                                                  \
        value = QVariant(default);                     \
        Set##name(default);                            \
    }                                                  \
    return value.toInt();                              \
}                                                      \
                                                       \
void PreferencesManager::Set##name(int value)          \
{                                                      \
    MySettings.setValue(#name, QVariant(value));       \
}

#include "prefs_options.h"

#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_INT
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR
#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS


void PreferencesManager::populatePreferencesDialog(PreferencesDialog *prefDialog)
{
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name, default)
#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name, default)
#define CONFIG_ATTRIBUTE_STRING(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_INT(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_COMBO(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_COLOR(control, name, default) prefDialog->Set##name(Get##name());
    #include "prefs_options.h"
#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_INT
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR
#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
    MySettings.sync();
}


void PreferencesManager::extractValuesFromPreferencesDialog(PreferencesDialog *prefDialog)
{
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name, default)
#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name, default)
#define CONFIG_ATTRIBUTE_STRING(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_INT(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_COMBO(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_COLOR(control, name, default) Set##name(prefDialog->Get##name());
    #include "prefs_options.h"
#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_INT
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR
#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
    MySettings.sync();
}
