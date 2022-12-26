/****************************************************************************
`**
** Copyright (C) 2016-2023 Mike Pogue, Dan Lyke
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

#include "keybindings.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"



#define KEYACTION(NAME, STRNAME, ACTION) const char *keyActionName_##NAME = STRNAME;
#include "keyactions.h"
#undef KEYACTION

// ----------------------------------------------------------------------

#define KEYACTION(NAME, STRNAME, ACTION)                         \
     class KeyAction##NAME : public KeyAction {                  \
     public:                                                     \
         const char *name() override;                            \
         void doAction(MainWindow *) override;                   \
     };                                                          \
                                                                 \
     const char *KeyAction##NAME::name() {                       \
         return keyActionName_##NAME;                            \
     };                                                          \
     void KeyAction##NAME::doAction(MainWindow *) {              \
         ACTION;                                                 \
     };                                                          \
                                                                 \
     static KeyAction##NAME keyaction_KeyAction##NAME;
#include "keyactions.h"
#undef KEYACTION

KeyAction::KeyAction() : mw(NULL)
{
}

KeyAction::~KeyAction()
{
}


void KeyAction::do_activated()
{
    if (mw)
        doAction(mw);
}

QVector<KeyAction*> KeyAction::availableActions()
{
    QVector<KeyAction*> actions;

#define KEYACTION(name, strname, action) actions.append(&keyaction_KeyAction##name);
#include "keyactions.h"
#undef KEYACTION
    return actions;
}

static QHash<QString, KeyAction*> actionNameToActionMap;


QHash<QString, KeyAction*> KeyAction::actionNameToActionMappings()
{
    if (actionNameToActionMap.empty())
    {
        QVector<KeyAction*> actions(availableActions());

        for (auto action = actions.begin(); action != actions.end(); ++action)
        {
            actionNameToActionMap[(*action)->name()] = *action;
        }
    }
    return actionNameToActionMap;
}

KeyAction * KeyAction::actionByName(const QString &name)
{
    if (actionNameToActionMap.empty())
    {
        actionNameToActionMappings();
    }
    return actionNameToActionMap[name];
}


QHash<QString, KeyAction *>menuKeyMappings;

void KeyAction::setKeybindingsFromMenuObjects(const QHash<QString, KeyAction *> &keyMappings)
{
    menuKeyMappings = keyMappings;
}

QHash<QString, KeyAction *> KeyAction::defaultKeyToActionMappings(int revisionNumber)
{
    QHash<QString, KeyAction *> keyMappings(menuKeyMappings);

    // When you add a new set of defaults, put them above this section
    // prefixed with a break like this + 1. Then increment the constant
    // CURRENT_VERSION_OF_KEY_DEFAULTS in keybindings.h

    if (revisionNumber > 2)
        return keyMappings;
    
//    keyMappings[QKeySequence(Qt::Key_A|Qt::MetaModifier|Qt::ControlModifier).toString()] = &keyaction_KeyActionSDSquareYourSets;
//    keyMappings[QKeySequence(Qt::Key_S|Qt::MetaModifier|Qt::ControlModifier).toString()] = &keyaction_KeyActionSDHeadsStart;
//    keyMappings[QKeySequence(Qt::Key_B|Qt::MetaModifier|Qt::ControlModifier).toString()] = &keyaction_KeyActionSDHeadsSquareThru;
//    keyMappings[QKeySequence(Qt::Key_L|Qt::MetaModifier|Qt::ControlModifier).toString()] = &keyaction_KeyActionSDHeads1p2p;

    // The old way (above) is deprecated as of Qt6.  Don't ask me how doing it the below way gets rid of this warning:
    //   warning: 'operator int' is deprecated: Use QKeyCombination instead of int [-Wdeprecated-declarations]
    keyMappings[QKeySequence(Qt::CTRL | Qt::META | Qt::Key_A).toString()] = &keyaction_KeyActionSDSquareYourSets;
    keyMappings[QKeySequence(Qt::CTRL | Qt::META | Qt::Key_S).toString()] = &keyaction_KeyActionSDHeadsStart;
    keyMappings[QKeySequence(Qt::CTRL | Qt::META | Qt::Key_B).toString()] = &keyaction_KeyActionSDHeadsSquareThru;
    keyMappings[QKeySequence(Qt::CTRL | Qt::META | Qt::Key_L).toString()] = &keyaction_KeyActionSDHeads1p2p;

    if (revisionNumber > 1)
        return keyMappings;
    
    keyMappings[QKeySequence(Qt::Key_T | Qt::ControlModifier).toString()] = &keyaction_KeyActionFilterToggle;

    if (revisionNumber > 0)
        return keyMappings;
    
    keyMappings[QKeySequence(Qt::Key_End).toString()] = &keyaction_KeyActionStopSong;
    keyMappings[QKeySequence(Qt::Key_Space).toString()] = &keyaction_KeyActionPlaySong;
    keyMappings[QKeySequence(Qt::Key_Home).toString()] = &keyaction_KeyActionRestartSong;
    keyMappings[QKeySequence(Qt::Key_Period).toString()] = &keyaction_KeyActionRestartSong;
    keyMappings[QKeySequence(Qt::Key_Right).toString()] = &keyaction_KeyActionForward15Seconds;
    keyMappings[QKeySequence(Qt::Key_Left).toString()] = &keyaction_KeyActionBackward15Seconds;
//    keyMappings[QKeySequence(Qt::Key_Down).toString()] = &keyaction_KeyActionVolumeMinus;
//    keyMappings[QKeySequence(Qt::Key_Up).toString()] = &keyaction_KeyActionVolumePlus;
    keyMappings[QKeySequence(Qt::Key_Plus).toString()] = &keyaction_KeyActionTempoPlus;
    keyMappings[QKeySequence(Qt::Key_Minus).toString()] = &keyaction_KeyActionTempoMinus;
    keyMappings[QKeySequence(Qt::Key_D).toString()] = &keyaction_KeyActionPitchMinus;
    keyMappings[QKeySequence(Qt::Key_F17).toString()] = &keyaction_KeyActionPlayPrevious;
    keyMappings[QKeySequence(Qt::Key_J).toString()] = &keyaction_KeyActionPlayPrevious;
    keyMappings[QKeySequence(Qt::Key_F18).toString()] = &keyaction_KeyActionPlayNext;
    keyMappings[QKeySequence(Qt::Key_K).toString()] = &keyaction_KeyActionPlayNext;
    keyMappings[QKeySequence(Qt::Key_L).toString()] = &keyaction_KeyActionLoopToggle;
    keyMappings[QKeySequence(Qt::Key_M).toString()] = &keyaction_KeyActionMute;
    keyMappings[QKeySequence(Qt::Key_F16).toString()] = &keyaction_KeyActionPlaySong;
    keyMappings[QKeySequence(Qt::Key_P).toString()] = &keyaction_KeyActionPlaySong;
    keyMappings[QKeySequence(Qt::Key_Q).toString()] = &keyaction_KeyActionTestLoop;
    keyMappings[QKeySequence(Qt::Key_S).toString()] = &keyaction_KeyActionStopSong;
    keyMappings[QKeySequence(Qt::Key_T).toString()] = &keyaction_KeyActionNextTab;
    keyMappings[QKeySequence(Qt::Key_U).toString()] = &keyaction_KeyActionPitchPlus;
    keyMappings[QKeySequence(Qt::Key_Y).toString()] = &keyaction_KeyActionFadeOut;
    keyMappings[QKeySequence(Qt::Key_Y).toString()] = &keyaction_KeyActionFadeOut;

    return keyMappings;
}


