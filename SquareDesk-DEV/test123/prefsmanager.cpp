/* See the large comment at the top of prefs_options.h */

#include "prefsmanager.h"
#include "preferencesdialog.h"
#include <QDir>

#include "common_enums.h"

QString defaultPatterColor = "#7963ff";
QString defaultSingingColor = "#00af5c";
QString defaultCalledColor = "#ab6900";
QString defaultExtrasColor = "#9c1f00";

PreferencesManager::PreferencesManager() : MySettings()
{
}

#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name, default)  \
QString PreferencesManager::Get##name()                  \
{                                                        \
    qDebug() << "STRING_NO_PREFS PreferencesManager::Get" << #name << ", default:" << #default;   \
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
    qDebug() << "STRING_NO_PREFS PreferencesManager::Set" << #name << ", value:" << value;   \
    MySettings.setValue(#name, value);                   \
}

#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name, default) \
bool PreferencesManager::Get##name()                     \
{                                                        \
    qDebug() << "BOOL_NO_PREFS PreferencesManager::Get" << #name << ", default:" << #default;   \
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
    qDebug() << "BOOL_NO_PREFS PreferencesManager::Set" << #name << ", value:" << value;   \
    MySettings.setValue(#name, value ? true : false);    \
}

#define CONFIG_ATTRIBUTE_STRING(control, name, default) \
    QString PreferencesManager::Get##name()             \
{                                                       \
    QString value = MySettings.value(#name).toString(); \
    if (value.isNull())                                 \
    {                                                   \
        value = default;                                \
        MySettings.setValue(#name, value);              \
    }                                                   \
    qDebug() << "STRING PreferencesManager::Get" << #name << ", default:" << #default << ", returning:" << value;   \
    return value;                                       \
}                                                       \
                                                        \
void PreferencesManager::Set##name(QString value)       \
{                                                       \
    qDebug() << "STRING PreferencesManager::Set" << #name << ", value:" << value;   \
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
    qDebug() << "COLOR PreferencesManager::Get" << #name << ", default:" << #default << ", returning:" << value;   \
    return value;                                       \
}                                                       \
                                                        \
void PreferencesManager::Set##name(QString value)       \
{                                                       \
    qDebug() << "COLOR PreferencesManager::Set" << #name << ", value:" << value;   \
    MySettings.setValue(#name, value);                  \
}

#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) \
bool PreferencesManager::Get##name()                     \
{                                                        \
    QString value = MySettings.value(#name).toString();  \
    if (value.isNull())                                  \
    {                                                    \
        value = (default) ? "true" : "false";            \
        Set##name(default);                              \
    }                                                    \
    qDebug() << "BOOL PreferencesManager::Get" << #name << ", default:" << #default << ", returning:" << (value == "true" || value == "checked");   \
    return value == "true" || value == "checked";        \
}                                                        \
                                                         \
void PreferencesManager::Set##name(bool value)           \
{                                                        \
    qDebug() << "BOOL PreferencesManager::Set" << #name << ", value:" << value;   \
    MySettings.setValue(#name, value ? true : false);    \
}

// OK get is wrong below, because it returns a string, not an int

#define CONFIG_ATTRIBUTE_INT(control, name, default) \
    int PreferencesManager::Get##name()              \
{                                                    \
    value = MySettings.value(#name).toString();      \
    if (value.isNull())                              \
    {                                                \
        value = default;                             \
        Set##name(default);                          \
    }                                                \
    qDebug() << "INT PreferencesManager::Get" << #name << ", default:" << #default << ", returning as string (wrong):" << value;   \
    return value;                                    \
}                                                    \
                                                     \
void PreferencesManager::Set##name(int value)        \
{                                                    \
    qDebug() << "INT PreferencesManager::Set" << #name << ", value:" << value;   \
    MySettings.setValue(#name, value);               \
}

#define CONFIG_ATTRIBUTE_COMBO(control, name, default) \
int  PreferencesManager::Get##name()                   \
{                                                      \
    QVariant value = MySettings.value(#name);          \
    {                                                  \
        if (value.isNull())                            \
            value = QVariant(default);                 \
        Set##name(default);                            \
    }                                                  \
    qDebug() << "COMBO PreferencesManager::Get" << #name << ", default:" << #default << ", returning as int:" << value.toInt();   \
    return value.toInt();                              \
}                                                      \
                                                       \
void PreferencesManager::Set##name(int value)          \
{                                                      \
    qDebug() << "COMBO PreferencesManager::Set" << #name << ", value:" << value;   \
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
    qDebug() << "populatePreferencesDialog ------------------------";
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
}


void PreferencesManager::extractValuesFromPreferencesDialog(PreferencesDialog *prefDialog)
{
    qDebug() << "extractValuesFromPreferencesDialog ------------------------";
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
}




