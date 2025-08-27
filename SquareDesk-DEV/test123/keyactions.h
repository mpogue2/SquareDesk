/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
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

/*  This is where the key action mappings get set. This file gets run
 *  through the preprocessor in a couple of different places, and
 *  different scenarios, so it's important that:
 *
 *  1. This file has no declarations that can't be processed twice.
 *  2. This file doesn't have "include" guards.
 *  
 *  Each "KEYACTION" mapping here takes 3 arguments:
 *  1. a name used symbolically internally (also for the
 *     defaultKeyToActionMappings table key bindings.cpp)
 *  2. a string describing the action for the hotkeys setting
 *  3. the code to run when that operation is triggered, usually a
 *     function on the main window via "mw->". Note that this can be a
 *     private function, all of these actions are declared as friends of the
 *     main window.
 *  
 */

KEYACTION(StopSong, "Stop Song", mw->on_darkStopButton_clicked() )
KEYACTION(RestartSong, "Restart Song", mw->on_darkStopButton_clicked(); mw->on_darkPlayButton_clicked(); mw->on_darkWarningLabel_clicked() )
KEYACTION(Forward15Seconds, "Skip Forward 10 Seconds", mw->on_actionSkip_Forward_triggered())
KEYACTION(Backward15Seconds, "Skip Backward 10 Seconds", mw->on_actionSkip_Backward_triggered())
KEYACTION(VolumeMinus, "Volume -", mw->on_actionVolume_Down_triggered())
KEYACTION(VolumePlus, "Volume +", mw->on_actionVolume_Up_triggered())
KEYACTION(TempoPlus, "Tempo +", mw->actionTempoPlus())
KEYACTION(TempoMinus, "Tempo -", mw->actionTempoMinus())
// KEYACTION(PlayPrevious, "Load Previous Song", mw->on_actionPrevious_Playlist_Item_triggered())
// KEYACTION(PlayNext, "Load Next Song", mw->on_actionNext_Playlist_Item_triggered())
KEYACTION(Mute, "Mute", mw->on_actionMute_triggered())
KEYACTION(PitchPlus, "Pitch +", mw->on_actionPitch_Up_triggered())
KEYACTION(PitchMinus, "Pitch -", mw->on_actionPitch_Down_triggered())
KEYACTION(FadeOut , "Fade Out", mw->actionFadeOutAndPause())
KEYACTION(LoopToggle, "Loop Toggle", mw->on_loopButton_toggled(!mw->ui->actionLoop->isChecked()))

KEYACTION(StartLoop, "Start Loop", mw->on_darkStartLoopButton_clicked())
KEYACTION(EndLoop,   "End Loop",   mw->on_darkEndLoopButton_clicked())

KEYACTION(TestLoop, "Test Loop", mw->on_actionTest_Loop_triggered())
KEYACTION(NextTab, "Toggle Music/Cuesheet Tab ", mw->actionNextTab())
KEYACTION(PlaySong, "Play/Pause Song", mw->on_darkPlayButton_clicked())
KEYACTION(SwitchToMusicTab, "Switch to Music Tab", mw->actionSwitchToTab("Music"))
KEYACTION(SwitchToTimersTab, "Switch to Timers Tab", mw->actionSwitchToTab("Timers"))
KEYACTION(SwitchToLyricsTab, "Switch to Cuesheet Tab", mw->actionSwitchToTab("Cuesheet"))
KEYACTION(SwitchToSDTab, "Switch to SD Tab", mw->actionSwitchToTab("SD"))
KEYACTION(SwitchToDanceProgramsTab, "Switch to Dance Programs Tab", mw->actionSwitchToTab("Dance Programs"))
KEYACTION(SwitchToReferenceTab, "Switch to Reference Tab", mw->actionSwitchToTab("Reference"))
KEYACTION(FilterPatter, "Filter Songs to Patter", mw->actionFilterSongsToPatter())
KEYACTION(FilterSingers, "Filter Songs to Singing", mw->actionFilterSongsToSingers())
KEYACTION(FilterToggle, "Toggle Songs Filter", mw->actionFilterSongsPatterSingersToggle())

KEYACTION(AutoscrollToggle, "Toggle Cuesheet Auto-scrolling", mw->actionToggleCuesheetAutoscroll())

KEYACTION(SDSquareYourSets, "SD Square Your Sets", mw->on_actionSDSquareYourSets_triggered())
KEYACTION(SDHeadsStart, "SD Heads Start", mw->on_actionSDHeadsStart_triggered())
KEYACTION(SDHeadsSquareThru, "SD Heads Square Thru", mw->on_actionSDHeadsSquareThru_triggered())
KEYACTION(SDHeads1p2p, "SD Heads 1P2P", mw->on_actionSDHeads1p2p_triggered())

KEYACTION(ResetPatterTimer, "Reset Patter Timer", mw->on_actionReset_Patter_Timer_triggered())
