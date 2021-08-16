/****************************************************************************
**
** Copyright (C) 2016-2021 Mike Pogue, Dan Lyke
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

#ifndef PREFSMANAGER_H_INCLUDED
#define PREFSMANAGER_H_INCLUDED

#include "QSettings"
#include "keybindings.h"

class PreferencesDialog;
class PreferencesManager {
public:
    QSettings MySettings;
    PreferencesManager();

    void populatePreferencesDialog(PreferencesDialog *prefDialog);
    void extractValuesFromPreferencesDialog(PreferencesDialog *prefDialog);
    void setTagColors( const QHash<QString,QPair<QString,QString>> &);
    const QHash<QString,QPair<QString,QString>> &getTagColors();

    QHash<QString, KeyAction *> GetHotkeyMappings();
    void SetHotkeyMappings(QHash<QString, KeyAction *>);

private:
    QHash<QString,QPair<QString,QString>> tagColors;
public:

#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name, default)  bool Get##name(); void Set##name(bool value);
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name, default)  QString Get##name(); void Set##name(QString value);
#define CONFIG_ATTRIBUTE_INT_NO_PREFS(name, default) int Get##name(); void Set##name(int value);

#define CONFIG_ATTRIBUTE_STRING(control, name, default) QString Get##name(); void Set##name(QString value);
#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) bool Get##name(); void Set##name(bool value);
#define CONFIG_ATTRIBUTE_INT(control, name, default) int Get##name(); void Set##name(int value);
#define CONFIG_ATTRIBUTE_COMBO(control, name, default) int Get##name(); void Set##name(int value);
#define CONFIG_ATTRIBUTE_COLOR(control, name, default) QString Get##name(); void Set##name(QString value);
#define CONFIG_ATTRIBUTE_SLIDER(control, name, default) int Get##name(); void Set##name(int value);

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
};

#endif // ifndef PREFSMANAGER_H_INCLUDED
