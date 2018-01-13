/****************************************************************************
**
** Copyright (C) 2016, 2017, 2018 Mike Pogue, Dan Lyke
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

#include "common_enums.h"
#include "default_colors.h"

PreferencesManager::PreferencesManager() : MySettings()
{
}

static const char * hotkeyPrefix = "hotkey_";

QHash<Qt::Key, KeyAction *> PreferencesManager::GetHotkeyMappings()
{
    QHash<Qt::Key, KeyAction *> mappings;
    QHash<QString, KeyAction*> actions(KeyAction::actionNameToActionMappings());
    QHash<Qt::Key, KeyAction*> defaultKeyToActionMappings(KeyAction::defaultKeyToActionMappings());
    QVector<Qt::Key> mappableKeys(KeyAction::mappableKeys());
    
    for (auto key : mappableKeys)
    {
        QString value = MySettings.value(hotkeyPrefix + QKeySequence(key).toString()).toString();
        if (!value.isNull())
        {
            auto action = actions.find(value);
            if (action != actions.end())
            {
                mappings[key] = action.value();
            }
            else
            {
                mappings[key] = defaultKeyToActionMappings[key];
            }
        } // end of if we found the hotkey
    } // end of all of the mappable keys
    return mappings;
}

void PreferencesManager::SetHotkeyMappings(QHash<Qt::Key, KeyAction *> mapping)
{
    QVector<Qt::Key> mappableKeys(KeyAction::mappableKeys());
    for (auto key : mappableKeys)
    {
        auto keymap = mapping.find(key);
        if (keymap == mapping.end())
        {
            MySettings.remove(hotkeyPrefix + QKeySequence(key).toString());
        }
        else
        {
            MySettings.setValue(hotkeyPrefix + QKeySequence(key).toString(), keymap.value()->name());
        }
    }
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
        value = default;                                 \
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
#undef CONFIG_ATTRIBUTE_INT_NO_PREFS


void PreferencesManager::populatePreferencesDialog(PreferencesDialog *prefDialog)
{
    prefDialog->setHotkeys(GetHotkeyMappings());
    
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name, default)
#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name, default)
#define CONFIG_ATTRIBUTE_INT_NO_PREFS(name, default)

#define CONFIG_ATTRIBUTE_STRING(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_INT(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_COMBO(control, name, default) prefDialog->Set##name(Get##name());
#define CONFIG_ATTRIBUTE_COLOR(control, name, default) prefDialog->Set##name(Get##name());

    #include "prefs_options.h"

#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
#undef CONFIG_ATTRIBUTE_INT_NO_PREFS

#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_INT
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR

    MySettings.sync();
}


void PreferencesManager::extractValuesFromPreferencesDialog(PreferencesDialog *prefDialog)
{
    SetHotkeyMappings(prefDialog->getHotkeys());
    
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name, default)
#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name, default)
#define CONFIG_ATTRIBUTE_INT_NO_PREFS(name, default)

#define CONFIG_ATTRIBUTE_STRING(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_INT(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_COMBO(control, name, default) Set##name(prefDialog->Get##name());
#define CONFIG_ATTRIBUTE_COLOR(control, name, default) Set##name(prefDialog->Get##name());

    #include "prefs_options.h"

#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
#undef CONFIG_ATTRIBUTE_INT_NO_PREFS

#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_INT
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR
    MySettings.sync();
}
