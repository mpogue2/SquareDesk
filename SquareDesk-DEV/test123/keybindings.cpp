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

#include "keybindings.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"



const char * keyActionName_UnassignedNoAction = "<unassigned>";
const char * keyActionName_StopSong = "Stop Song";
const char * keyActionName_RestartSong = "Restart Song";
const char * keyActionName_Forward15Seconds = "Forward 15 Seconds";
const char * keyActionName_Backward15Seconds = "Backward 15 Seconds";
const char * keyActionName_VolumeMinus = "Volume -";
const char * keyActionName_VolumePlus = "Volume +";
const char * keyActionName_TempoPlus = "Tempo +";
const char * keyActionName_TempoMinus = "Tempo -";
const char * keyActionName_PlayNext = "Load Next Song";
const char * keyActionName_Mute = "Mute";
const char * keyActionName_PitchPlus = "Pitch +";
const char * keyActionName_PitchMinus = "Pitch -";
const char * keyActionName_FadeOut  = "Fade Out";
const char * keyActionName_LoopToggle = "Loop Toggle";
const char * keyActionName_TestLoop = "Test Loop";
const char * keyActionName_NextTab = "Toggle Music/Lyrics Tab ";
const char * keyActionName_PlaySong = "Play/Pause Song";

const char * keyActionName_SwitchToMusicTab = "Switch to Music Tab";
const char * keyActionName_SwitchToTimersTab = "Switch to Timers Tab";
const char * keyActionName_SwitchToLyricsTab = "Switch to Lyrics/Patter Tab";
const char * keyActionName_SwitchToSDTab = "Switch to SD Tab";
const char * keyActionName_SwitchToDanceProgramsTab = "Switch to Dance Programs Tab";
const char * keyActionName_SwitchToReferenceTab = "Switch to Reference Tab";

// ----------------------------------------------------------------------

class KeyActionUnassignedNoAction : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionUnassignedNoAction::name() {
    return keyActionName_UnassignedNoAction;
};
void KeyActionUnassignedNoAction::doAction(MainWindow *) {
};



class KeyActionStopSong : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionStopSong::name() {
    return keyActionName_StopSong;
};
void KeyActionStopSong::doAction(MainWindow *mw) {
    mw->on_stopButton_clicked();
};



class KeyActionPlaySong : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionPlaySong::name() {
    return keyActionName_PlaySong;
};
void KeyActionPlaySong::doAction(MainWindow *mw) {
    mw->on_playButton_clicked();
};





class KeyActionRestartSong : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionRestartSong::name() {
    return keyActionName_RestartSong;
};

void KeyActionRestartSong::doAction(MainWindow *mw) {
    mw->on_stopButton_clicked();
    mw->on_playButton_clicked();
    mw->on_warningLabel_clicked();  // also reset the Patter Timer to zero
};


class KeyActionForward15Seconds : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionForward15Seconds::name() {
    return keyActionName_Forward15Seconds;
};
void KeyActionForward15Seconds::doAction(MainWindow *mw) {
    mw->on_actionSkip_Ahead_15_sec_triggered();
};



class KeyActionBackward15Seconds : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionBackward15Seconds::name() {
    return keyActionName_Backward15Seconds;
};
void KeyActionBackward15Seconds::doAction(MainWindow *mw) {
    mw->on_actionSkip_Back_15_sec_triggered();
};




class KeyActionVolumeMinus : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionVolumeMinus::name() {
    return keyActionName_VolumeMinus;
};
void KeyActionVolumeMinus::doAction(MainWindow *mw) {
    mw->on_actionVolume_Down_triggered();
};



class KeyActionVolumePlus : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionVolumePlus::name() {
    return keyActionName_VolumePlus;
};
void KeyActionVolumePlus::doAction(MainWindow *mw) {
    mw->on_actionVolume_Up_triggered();
};



class KeyActionTempoPlus : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionTempoPlus::name() {
    return keyActionName_TempoPlus;
};
void KeyActionTempoPlus::doAction(MainWindow *mw) {
    mw->actionTempoPlus();
};



class KeyActionTempoMinus : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionTempoMinus::name() {
    return keyActionName_TempoMinus;
};
void KeyActionTempoMinus::doAction(MainWindow *mw) {
    mw->actionTempoMinus();
};



class KeyActionPlayNext : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionPlayNext::name() {
    return keyActionName_PlayNext;
};
void KeyActionPlayNext::doAction(MainWindow *mw) {
    mw->on_actionNext_Playlist_Item_triggered();  // compatible with SqView!
};



class KeyActionMute : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionMute::name() {
    return keyActionName_Mute;
};
void KeyActionMute::doAction(MainWindow *mw) {
    mw->on_actionMute_triggered();
};



class KeyActionPitchPlus : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionPitchPlus::name() {
    return keyActionName_PitchPlus;
};
void KeyActionPitchPlus::doAction(MainWindow *mw) {
    mw->on_actionPitch_Up_triggered();
};



class KeyActionPitchMinus : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionPitchMinus::name() {
    return keyActionName_PitchMinus;
};
void KeyActionPitchMinus::doAction(MainWindow *mw) {
    mw->on_actionPitch_Down_triggered();
};



class KeyActionFadeOut  : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionFadeOut ::name() {
    return keyActionName_FadeOut ;
};
void KeyActionFadeOut ::doAction(MainWindow *mw) {
    mw->actionFadeOutAndPause();
};



class KeyActionLoopToggle : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionLoopToggle::name() {
    return keyActionName_LoopToggle;
};
void KeyActionLoopToggle::doAction(MainWindow *mw) {
    mw->on_loopButton_toggled(!mw->ui->actionLoop->isChecked());
};


class KeyActionTestLoop : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionTestLoop::name() {
    return keyActionName_TestLoop;
};
void KeyActionTestLoop::doAction(MainWindow *mw) {
    mw->on_actionTest_Loop_triggered();
};


class KeyActionNextTab : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionNextTab::name() {
    return keyActionName_NextTab;
};
void KeyActionNextTab::doAction(MainWindow *mw) {
    mw->actionNextTab();
};


class KeyActionSwitchToMusicTab : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionSwitchToMusicTab::name() {
    return keyActionName_SwitchToMusicTab;
};
void KeyActionSwitchToMusicTab::doAction(MainWindow *mw) {
    mw->actionSwitchToTab("Music");
};


class KeyActionSwitchToTimersTab : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionSwitchToTimersTab::name() {
    return keyActionName_SwitchToTimersTab;
};
void KeyActionSwitchToTimersTab::doAction(MainWindow *mw) {
    mw->actionSwitchToTab("Timers");
};


class KeyActionSwitchToLyricsTab : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionSwitchToLyricsTab::name() {
    return keyActionName_SwitchToLyricsTab;
};
void KeyActionSwitchToLyricsTab::doAction(MainWindow *mw) {
    mw->actionSwitchToTab("Lyrics");
    mw->actionSwitchToTab("Patter");
};


class KeyActionSwitchToSDTab : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionSwitchToSDTab::name() {
    return keyActionName_SwitchToSDTab;
};
void KeyActionSwitchToSDTab::doAction(MainWindow *mw) {
    mw->actionSwitchToTab("SD");
};


class KeyActionSwitchToDanceProgramsTab : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionSwitchToDanceProgramsTab::name() {
    return keyActionName_SwitchToDanceProgramsTab;
};
void KeyActionSwitchToDanceProgramsTab::doAction(MainWindow *mw) {
    mw->actionSwitchToTab("Dance Programs");
};


class KeyActionSwitchToReferenceTab : public KeyAction {
public:
    const char *name() override;
    void doAction(MainWindow *) override;
};

const char *KeyActionSwitchToReferenceTab::name() {
    return keyActionName_SwitchToReferenceTab;
};
void KeyActionSwitchToReferenceTab::doAction(MainWindow *mw) {
    mw->actionSwitchToTab("Reference");
};




// --------------------------------------------------------------------
static KeyActionUnassignedNoAction keyaction_KeyActionUnassignedNoAction;
static KeyActionStopSong keyaction_KeyActionStopSong;
static KeyActionPlaySong keyaction_KeyActionPlaySong;
static KeyActionRestartSong keyaction_KeyActionRestartSong;
static KeyActionForward15Seconds keyaction_KeyActionForward15Seconds;
static KeyActionBackward15Seconds keyaction_KeyActionBackward15Seconds;
static KeyActionVolumePlus keyaction_KeyActionVolumePlus;
static KeyActionVolumeMinus keyaction_KeyActionVolumeMinus;
static KeyActionTempoPlus keyaction_KeyActionTempoPlus;
static KeyActionTempoMinus keyaction_KeyActionTempoMinus;
static KeyActionPlayNext keyaction_KeyActionPlayNext;
static KeyActionMute keyaction_KeyActionMute;
static KeyActionPitchPlus keyaction_KeyActionPitchPlus;
static KeyActionPitchMinus keyaction_KeyActionPitchMinus;
static KeyActionFadeOut  keyaction_KeyActionFadeOut ;
static KeyActionLoopToggle keyaction_KeyActionLoopToggle;
static KeyActionTestLoop keyaction_KeyActionTestLoop;
static KeyActionNextTab keyaction_KeyActionNextTab;
static KeyActionSwitchToMusicTab keyaction_KeyActionSwitchToMusicTab;
static KeyActionSwitchToTimersTab keyaction_KeyActionSwitchToTimersTab;
static KeyActionSwitchToLyricsTab keyaction_KeyActionSwitchToLyricsTab;
static KeyActionSwitchToSDTab keyaction_KeyActionSwitchToSDTab;
static KeyActionSwitchToDanceProgramsTab keyaction_KeyActionSwitchToDanceProgramsTab;
static KeyActionSwitchToReferenceTab keyaction_KeyActionSwitchToReferenceTab;


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

    actions.append(&keyaction_KeyActionUnassignedNoAction);
    actions.append(&keyaction_KeyActionPlaySong);
    actions.append(&keyaction_KeyActionStopSong);
    actions.append(&keyaction_KeyActionRestartSong);
    actions.append(&keyaction_KeyActionForward15Seconds);
    actions.append(&keyaction_KeyActionBackward15Seconds);
    actions.append(&keyaction_KeyActionPlayNext);
    actions.append(&keyaction_KeyActionLoopToggle);
    actions.append(&keyaction_KeyActionTestLoop);

    actions.append(&keyaction_KeyActionTempoPlus);
    actions.append(&keyaction_KeyActionTempoMinus);
    actions.append(&keyaction_KeyActionPitchPlus);
    actions.append(&keyaction_KeyActionPitchMinus);

    actions.append(&keyaction_KeyActionVolumePlus);
    actions.append(&keyaction_KeyActionVolumeMinus);
    actions.append(&keyaction_KeyActionMute);
    actions.append(&keyaction_KeyActionFadeOut );

    actions.append(&keyaction_KeyActionNextTab);

    actions.append(&keyaction_KeyActionSwitchToMusicTab);
    actions.append(&keyaction_KeyActionSwitchToTimersTab);
    actions.append(&keyaction_KeyActionSwitchToLyricsTab);
    actions.append(&keyaction_KeyActionSwitchToSDTab);
    actions.append(&keyaction_KeyActionSwitchToDanceProgramsTab);
    actions.append(&keyaction_KeyActionSwitchToReferenceTab);
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

QHash<QString, KeyAction *> KeyAction::defaultKeyToActionMappings()
{
    QHash<QString, KeyAction *> keyMappings(menuKeyMappings);

    keyMappings[QKeySequence(Qt::Key_End).toString()] = &keyaction_KeyActionStopSong;
    keyMappings[QKeySequence(Qt::Key_Space).toString()] = &keyaction_KeyActionPlaySong;
    keyMappings[QKeySequence(Qt::Key_Home).toString()] = &keyaction_KeyActionRestartSong;
    keyMappings[QKeySequence(Qt::Key_Period).toString()] = &keyaction_KeyActionRestartSong;
    keyMappings[QKeySequence(Qt::Key_Right).toString()] = &keyaction_KeyActionForward15Seconds;
    keyMappings[QKeySequence(Qt::Key_Left).toString()] = &keyaction_KeyActionBackward15Seconds;
    keyMappings[QKeySequence(Qt::Key_Down).toString()] = &keyaction_KeyActionVolumeMinus;
    keyMappings[QKeySequence(Qt::Key_Up).toString()] = &keyaction_KeyActionVolumePlus;
    keyMappings[QKeySequence(Qt::Key_Plus).toString()] = &keyaction_KeyActionTempoPlus;
    keyMappings[QKeySequence(Qt::Key_Minus).toString()] = &keyaction_KeyActionTempoMinus;
    keyMappings[QKeySequence(Qt::Key_D).toString()] = &keyaction_KeyActionPitchMinus;
    keyMappings[QKeySequence(Qt::Key_K).toString()] = &keyaction_KeyActionPlayNext;
    keyMappings[QKeySequence(Qt::Key_L).toString()] = &keyaction_KeyActionLoopToggle;
    keyMappings[QKeySequence(Qt::Key_M).toString()] = &keyaction_KeyActionMute;
    keyMappings[QKeySequence(Qt::Key_P).toString()] = &keyaction_KeyActionPlaySong;
    keyMappings[QKeySequence(Qt::Key_Q).toString()] = &keyaction_KeyActionTestLoop;
    keyMappings[QKeySequence(Qt::Key_S).toString()] = &keyaction_KeyActionStopSong;
    keyMappings[QKeySequence(Qt::Key_T).toString()] = &keyaction_KeyActionNextTab;
    keyMappings[QKeySequence(Qt::Key_U).toString()] = &keyaction_KeyActionPitchPlus;
    keyMappings[QKeySequence(Qt::Key_Y).toString()] = &keyaction_KeyActionFadeOut;
    return keyMappings;
}


