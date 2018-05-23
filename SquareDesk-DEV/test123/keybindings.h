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

#define MAX_KEYPRESSES_PER_ACTION 5

class KeyAction : public QObject
{
    Q_OBJECT;
public:
    KeyAction();
    virtual const char *name() = 0;
    virtual void doAction(MainWindow *) = 0;
    static QVector<KeyAction*> availableActions();
    static QHash<QString, KeyAction *> defaultKeyToActionMappings();
    static QHash<QString, KeyAction*> actionNameToActionMappings();
    static KeyAction *actionByName(const QString &name);
    static void setKeybindingsFromMenuObjects(const QHash<QString, KeyAction *> &keyMappings);
    
    void setMainWindow(MainWindow *mainWindow)
    {
        mw = mainWindow;
    }
    virtual ~KeyAction();
private:
    MainWindow *mw;
public slots:
    void do_activated();
};

extern const char * keyActionName_UnassignedNoAction;
extern const char * keyActionName_StopSong;
extern const char * keyActionName_RestartSong;
extern const char * keyActionName_Forward15Seconds;
extern const char * keyActionName_Backward15Seconds;
extern const char * keyActionName_VolumeMinus;
extern const char * keyActionName_VolumePlus;
extern const char * keyActionName_TempoPlus;
extern const char * keyActionName_TempoMinus;
extern const char * keyActionName_PlayPrevious;
extern const char * keyActionName_PlayNext;
extern const char * keyActionName_Mute;
extern const char * keyActionName_PitchPlus;
extern const char * keyActionName_PitchMinus;
extern const char * keyActionName_FadeOut ;
extern const char * keyActionName_LoopToggle;
extern const char * keyActionName_TestLoop;
extern const char * keyActionName_NextTab;
extern const char * keyActionName_PlaySong;


#endif /* ifndef KEYBINDINGS_H_INCLUDED */
