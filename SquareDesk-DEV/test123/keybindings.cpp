#include "keybindings.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"



static const char * keyActionName_UnassignedNoAction = "<unassigned>";
static const char * keyActionName_StopSong = "Stop Song";
static const char * keyActionName_RestartSong = "Restart Song";
static const char * keyActionName_Forward15Seconds = "Forward 15 Seconds";
static const char * keyActionName_Backward15Seconds = "Backward 15 Seconds";
static const char * keyActionName_VolumeMinus = "Volume -";
static const char * keyActionName_VolumePlus = "Volume +";
static const char * keyActionName_TempoPlus = "Tempo +";
static const char * keyActionName_TempoMinus = "Tempo -";
static const char * keyActionName_PlayNext = "Play Next";
static const char * keyActionName_Mute = "Mute";
static const char * keyActionName_PitchPlus = "Pitch +";
static const char * keyActionName_PitchMinus = "Pitch -";
static const char * keyActionName_FadeOut  = "Fade Out";
static const char * keyActionName_LoopToggle = "Loop Toggle";
static const char * keyActionName_NextTab = "Next Tab";
static const char * keyActionName_PlaySong = "Play Song";

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



// --------------------------------------------------------------------
static KeyActionUnassignedNoAction keyaction_KeyActionUnassignedNoAction;
static KeyActionStopSong keyaction_KeyActionStopSong;
static KeyActionPlaySong keyaction_KeyActionPlaySong;
static KeyActionRestartSong keyaction_KeyActionRestartSong;
static KeyActionForward15Seconds keyaction_KeyActionForward15Seconds;
static KeyActionBackward15Seconds keyaction_KeyActionBackward15Seconds;
static KeyActionVolumeMinus keyaction_KeyActionVolumeMinus;
static KeyActionVolumePlus keyaction_KeyActionVolumePlus;
static KeyActionTempoPlus keyaction_KeyActionTempoPlus;
static KeyActionTempoMinus keyaction_KeyActionTempoMinus;
static KeyActionPlayNext keyaction_KeyActionPlayNext;
static KeyActionMute keyaction_KeyActionMute;
static KeyActionPitchPlus keyaction_KeyActionPitchPlus;
static KeyActionPitchMinus keyaction_KeyActionPitchMinus;
static KeyActionFadeOut  keyaction_KeyActionFadeOut ;
static KeyActionLoopToggle keyaction_KeyActionLoopToggle;
static KeyActionNextTab keyaction_KeyActionNextTab;


KeyAction::KeyAction()
{
}

QVector<KeyAction*> KeyAction::availableActions()
{
    QVector<KeyAction*> actions;
    
    actions.append(&keyaction_KeyActionUnassignedNoAction);
    actions.append(&keyaction_KeyActionStopSong);
    actions.append(&keyaction_KeyActionPlaySong);
    actions.append(&keyaction_KeyActionRestartSong);
    actions.append(&keyaction_KeyActionForward15Seconds);
    actions.append(&keyaction_KeyActionBackward15Seconds);
    actions.append(&keyaction_KeyActionVolumeMinus);
    actions.append(&keyaction_KeyActionVolumePlus);
    actions.append(&keyaction_KeyActionTempoPlus);
    actions.append(&keyaction_KeyActionTempoMinus);
    actions.append(&keyaction_KeyActionPlayNext);
    actions.append(&keyaction_KeyActionMute);
    actions.append(&keyaction_KeyActionPitchPlus);
    actions.append(&keyaction_KeyActionPitchMinus);
    actions.append(&keyaction_KeyActionFadeOut );
    actions.append(&keyaction_KeyActionLoopToggle);
    actions.append(&keyaction_KeyActionNextTab);
    
    return actions;
}

QVector<Qt::Key> KeyAction::mappableKeys()
{
    QVector<Qt::Key> keys;
    keys.append( Qt::Key_End );
    keys.append( Qt::Key_Space );
    keys.append( Qt::Key_Home );
    keys.append( Qt::Key_Period );
    keys.append( Qt::Key_Right );
    keys.append( Qt::Key_Left );
    keys.append( Qt::Key_Down );
    keys.append( Qt::Key_Up );
    keys.append( Qt::Key_Plus );
    keys.append( Qt::Key_Minus );
    keys.append( Qt::Key_A );
    keys.append( Qt::Key_B );
    keys.append( Qt::Key_C );
    keys.append( Qt::Key_D );
    keys.append( Qt::Key_E );
    keys.append( Qt::Key_F );
    keys.append( Qt::Key_G );
    keys.append( Qt::Key_H );
    keys.append( Qt::Key_I );
    keys.append( Qt::Key_J );
    keys.append( Qt::Key_K );
    keys.append( Qt::Key_L );
    keys.append( Qt::Key_M );
    keys.append( Qt::Key_N );
    keys.append( Qt::Key_O );
    keys.append( Qt::Key_P );
    keys.append( Qt::Key_Q );
    keys.append( Qt::Key_R );
    keys.append( Qt::Key_S );
    keys.append( Qt::Key_T );
    keys.append( Qt::Key_U );
    keys.append( Qt::Key_V );
    keys.append( Qt::Key_Y );
    keys.append( Qt::Key_W );
    keys.append( Qt::Key_X );
    keys.append( Qt::Key_Y );
    keys.append( Qt::Key_Z );

    return keys;
}

QHash<QString, KeyAction*> KeyAction::actionNameToActionMappings()
{
    QHash<QString, KeyAction*> actionMappings;
    QVector<KeyAction*> actions(availableActions());

    for (auto action = actions.begin(); action != actions.end(); ++action)
    {
        actionMappings[(*action)->name()] = *action;
    }
    return actionMappings;
}


QHash<Qt::Key, KeyAction *> KeyAction::defaultKeyToActionMappings()
{
    QHash<Qt::Key, KeyAction *> keyMappings;
    
    keyMappings[Qt::Key_End] = &keyaction_KeyActionStopSong;
    keyMappings[Qt::Key_Space] = &keyaction_KeyActionPlaySong;
    keyMappings[Qt::Key_Home] = &keyaction_KeyActionRestartSong;
    keyMappings[Qt::Key_Period] = &keyaction_KeyActionRestartSong;
    keyMappings[Qt::Key_Right] = &keyaction_KeyActionForward15Seconds;
    keyMappings[Qt::Key_Left] = &keyaction_KeyActionBackward15Seconds;
    keyMappings[Qt::Key_Down] = &keyaction_KeyActionVolumeMinus;
    keyMappings[Qt::Key_Up] = &keyaction_KeyActionVolumePlus;
    keyMappings[Qt::Key_Plus] = &keyaction_KeyActionTempoPlus;
    keyMappings[Qt::Key_Minus] = &keyaction_KeyActionTempoMinus;
    keyMappings[Qt::Key_D] = &keyaction_KeyActionPitchMinus;
    keyMappings[Qt::Key_K] = &keyaction_KeyActionPlayNext;
    keyMappings[Qt::Key_L] = &keyaction_KeyActionLoopToggle;
    keyMappings[Qt::Key_M] = &keyaction_KeyActionMute;
    keyMappings[Qt::Key_P] = &keyaction_KeyActionPlaySong;
    keyMappings[Qt::Key_S] = &keyaction_KeyActionStopSong;
    keyMappings[Qt::Key_T] = &keyaction_KeyActionNextTab;
    keyMappings[Qt::Key_U] = &keyaction_KeyActionPitchPlus;
    return keyMappings;
}


