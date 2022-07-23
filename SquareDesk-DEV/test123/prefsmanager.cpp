/****************************************************************************
**
** Copyright (C) 2016-2022 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usage
** For commercial licensing terms and conditions, contact the authors via the
** email address above.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appear in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.
**
** $SQUAREDESK_END_LICENSE$
**
****************************************************************************/

/* See the large comment at the top of prefs_options.h */

#include "prefsmanager.h"
#include "preferencesdialog.h"
#include <QDir>

//#include "common_enums.h"
#include "default_colors.h"

PreferencesManager::PreferencesManager() : MySettings()
{
}

static const char * hotkeyPrefix = "hotkey_";

QHash<QString, KeyAction *> PreferencesManager::GetHotkeyMappings()
{
    QHash<QString, KeyAction*> actions(KeyAction::actionNameToActionMappings());
    QHash<QString, KeyAction*> mappings(KeyAction::defaultKeyToActionMappings(GetLastVersionOfKeyMappingDefaultsUsed()));
    bool foundAnySettings = false;

    QStringList all_keys(MySettings.allKeys());
    
    for (const auto &key : all_keys)
    {
        if (key.startsWith(hotkeyPrefix))
        {
            QString value = MySettings.value(key).toString();
            
            if (!value.isNull())
            {
                auto action = actions.find(value);
                if (action != actions.end())
                {
                    QString justKey(key);
                    justKey.replace(hotkeyPrefix,"");
                    if (justKey.length() > 0)
                    {
                        mappings[justKey] = action.value();
                        foundAnySettings = true;
                    }
                }
            }
        } // end of all of the mappable keys
    }
    if (!foundAnySettings)
    {
        mappings = KeyAction::defaultKeyToActionMappings();
    }

    return mappings;
}

void PreferencesManager::SetHotkeyMappings(QHash<QString, KeyAction *> mapping)
{
    QStringList all_keys(MySettings.allKeys());
    
    for (const auto &key : all_keys)
    {
        if (key.startsWith(hotkeyPrefix))
        {
            QString justKey(key);
            justKey.replace(hotkeyPrefix,"");
                
            auto keymap = mapping.find(justKey);
            if (keymap == mapping.end())
            {
                MySettings.remove(key);
            }
        }
    }

    for (auto map = mapping.cbegin(); map != mapping.cend(); ++map)
    {
        QString hotkeyName(hotkeyPrefix + QKeySequence(map.key()).toString());
        MySettings.setValue(hotkeyName, map.value()->name());
    }
    SetLastVersionOfKeyMappingDefaultsUsed(CURRENT_VERSION_OF_KEY_DEFAULTS + 1);
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
#define CONFIG_ATTRIBUTE_INT_NO_PREFS(name, default)     \
int PreferencesManager::Get##name()                      \
{                                                        \
    QString value = MySettings.value(#name).toString();  \
    if (value.isNull())                                  \
    {                                                    \
        value = QString("%1").arg(default);              \
        Set##name(default);                              \
    }                                                    \
    return value.toInt();                                \
}                                                        \
                                                         \
void PreferencesManager::Set##name(int value)        \
{                                                        \
    MySettings.setValue(#name, value);                   \
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

// sliders are initialized like text widgets
#define CONFIG_ATTRIBUTE_SLIDER(control, name, default) \
    int PreferencesManager::Get##name()              \
{                                                    \
    QString value = MySettings.value(#name).toString();      \
    if (value.isNull())                              \
    {                                                \
        value = QString::number(default);                             \
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
#undef CONFIG_ATTRIBUTE_SLIDER

#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
#undef CONFIG_ATTRIBUTE_INT_NO_PREFS

void PreferencesManager::setTagColors( const QHash<QString,QPair<QString,QString>> &tagColors)
{
    this->tagColors = tagColors;
}

const QHash<QString,QPair<QString,QString>> &PreferencesManager::getTagColors()
{
    return this->tagColors;
}

void PreferencesManager::populatePreferencesDialog(PreferencesDialog *prefDialog)
{
    prefDialog->setHotkeys(GetHotkeyMappings());
    prefDialog->setTagColors(getTagColors());
    prefDialog->setActiveTab(GetprefsDialogLastActiveTab());
    
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name, default)
#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name, default)
#define CONFIG_ATTRIBUTE_INT_NO_PREFS(name, default)

#define CONFIG_ATTRIBUTE_STRING(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_INT(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_COMBO(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_COLOR(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_SLIDER(control, name, default) prefDialog->Set##name(Get##name());

    #include "prefs_options.h"

#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
#undef CONFIG_ATTRIBUTE_INT_NO_PREFS

#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_INT
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR
#undef CONFIG_ATTRIBUTE_SLIDER

    MySettings.sync();
    prefDialog->finishPopulation();
}


void PreferencesManager::extractValuesFromPreferencesDialog(PreferencesDialog *prefDialog)
{
    SetHotkeyMappings(prefDialog->getHotkeys());
    setTagColors(prefDialog->getTagColors());
    SetprefsDialogLastActiveTab(prefDialog->getActiveTab());
    
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name, default)
#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name, default)
#define CONFIG_ATTRIBUTE_INT_NO_PREFS(name, default)

#define CONFIG_ATTRIBUTE_STRING(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_INT(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_COMBO(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_COLOR(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_SLIDER(control, name, default) Set##name(prefDialog->Get##name());

    #include "prefs_options.h"

#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
#undef CONFIG_ATTRIBUTE_INT_NO_PREFS

#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_INT
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR
#undef CONFIG_ATTRIBUTE_SLIDER
    MySettings.sync();
}
