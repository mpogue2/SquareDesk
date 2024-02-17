/****************************************************************************
**
** Copyright (C) 2016-2024 Mike Pogue, Dan Lyke
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

#ifndef KEYBINDINGS_H_INCLUDED
#define KEYBINDINGS_H_INCLUDED
#include <QVector>
#include <QHash>
#include <QObject>

namespace Ui
{
class MainWindow;
}


class MainWindow;

#define CURRENT_VERSION_OF_KEY_DEFAULTS 2
#define MAX_KEYPRESSES_PER_ACTION 5

class KeyAction : public QObject
{
    Q_OBJECT;
public:
    KeyAction();
    virtual const char *name() = 0;
    virtual void doAction(MainWindow *) = 0;
    static QVector<KeyAction*> availableActions();
    static QHash<QString, KeyAction *> defaultKeyToActionMappings(int version = 0);
    static QHash<QString, KeyAction*> actionNameToActionMappings();
    static KeyAction *actionByName(const QString &name);
    static void setKeybindingsFromMenuObjects(const QHash<QString, KeyAction *> &keyMappings);
    
    void setMainWindow(MainWindow *mainWindow)
    {
        mw = mainWindow;
    }
    virtual ~KeyAction();

protected:
    MainWindow *mw;
public slots:
    void do_activated();
};

#define KEYACTION(NAME, STRNAME, ACTION) extern const char *keyActionName_##NAME;
#include "keyactions.h"
#undef KEYACTION


#endif /* ifndef KEYBINDINGS_H_INCLUDED */
