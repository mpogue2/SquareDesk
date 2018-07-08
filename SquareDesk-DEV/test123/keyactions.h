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

KEYACTION(StopSong, "Stop Song", mw->on_stopButton_clicked() )
KEYACTION(RestartSong, "Restart Song", mw->on_stopButton_clicked(); mw->on_playButton_clicked();mw->on_warningLabel_clicked() )
KEYACTION(Forward15Seconds, "Forward 15 Seconds", mw->on_actionSkip_Ahead_15_sec_triggered())
KEYACTION(Backward15Seconds, "Backward 15 Seconds", mw->on_actionSkip_Back_15_sec_triggered())
KEYACTION(VolumeMinus, "Volume -", mw->on_actionVolume_Down_triggered())
KEYACTION(VolumePlus, "Volume +", mw->on_actionVolume_Up_triggered())
KEYACTION(TempoPlus, "Tempo +", mw->actionTempoPlus())
KEYACTION(TempoMinus, "Tempo -", mw->actionTempoMinus())
KEYACTION(PlayPrevious, "Load Previous Song", mw->on_actionPrevious_Playlist_Item_triggered())
KEYACTION(PlayNext, "Load Next Song", mw->on_actionNext_Playlist_Item_triggered())
KEYACTION(Mute, "Mute", mw->on_actionMute_triggered())
KEYACTION(PitchPlus, "Pitch +", mw->on_actionPitch_Up_triggered())
KEYACTION(PitchMinus, "Pitch -", mw->on_actionPitch_Down_triggered())
KEYACTION(FadeOut , "Fade Out", mw->actionFadeOutAndPause())
KEYACTION(LoopToggle, "Loop Toggle", mw->on_loopButton_toggled(!mw->ui->actionLoop->isChecked()))
KEYACTION(TestLoop, "Test Loop", mw->on_actionTest_Loop_triggered())
KEYACTION(NextTab, "Toggle Music/Lyrics Tab ", mw->actionNextTab())
KEYACTION(PlaySong, "Play/Pause Song", mw->on_playButton_clicked())
KEYACTION(SwitchToMusicTab, "Switch to Music Tab", mw->on_playButton_clicked())
KEYACTION(SwitchToTimersTab, "Switch to Timers Tab", mw->actionSwitchToTab("Timers"))
KEYACTION(SwitchToLyricsTab, "Switch to Lyrics/Patter Tab", mw->actionSwitchToTab("Lyrics"); mw->actionSwitchToTab("Patter") )
KEYACTION(SwitchToSDTab, "Switch to SD Tab", mw->actionSwitchToTab("SD"))
KEYACTION(SwitchToDanceProgramsTab, "Switch to Dance Programs Tab", mw->actionSwitchToTab("Dance Programs"))
KEYACTION(SwitchToReferenceTab, "Switch to Reference Tab", mw->actionSwitchToTab("Reference"))
KEYACTION(FilterPatter, "Filter Songs to Patter", mw->actionFilterSongsToPatter())
KEYACTION(FilterSingers, "Filter Songs to Singers", mw->actionFilterSongsToSingers())
