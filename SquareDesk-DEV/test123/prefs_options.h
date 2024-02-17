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

/*  This is where preferences options get set. This file gets run
 *  through the preprocessor in a couple of different places, and
 *  different scenarios, so it's important that:
 *
 *  1. This file has no declarations that can't be processed twice.
 *  2. This file doesn't have "include" guards.
 *
 * When adding a combo box here, make sure that you add the associated
 * "SetPulldownValuesToItemNumberPlusOne" in preferencesdialog.cpp
 *
 * The four places this file gets processed are prefsdialog.{cpp,h},
 * and prefsmanager.{cpp,h}.
 *
 * CONFIG_ATTRIBUTE_*_NO_PREFS(preference_name, default_value) - Sets
 * up a preference of the appropriate type in the PreferencesManager
 * (prefsmanager.{cpp,h}), with accessors of the form
 * Get##preference_name and Set##preference_name.
 *
 * CONFIG_ATTRIBUTE_*(control_name, preference_name, default_value) -
 * Also sets up accessors in the PreferencesDialog structure for the
 * names in the preferences.ui file, and populates the dialog values
 * from the settings by way of populatePreferencesDialog(...), and the
 * settings from the dialog values by way of
 * extractValuesFromPreferencesDialog(...), both members in the
 * PreferencesManager structure in prefsmanager.{cpp,h}.
 *
 */

CONFIG_ATTRIBUTE_STRING(musicPath, musicPath, QDir::homePath() + "/squareDeskMusic")

CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(darkMode, true);  // true if we're using the new dark GUI

CONFIG_ATTRIBUTE_COLOR(patterColorButton,  patterColorString,  DEFAULTPATTERCOLOR)
CONFIG_ATTRIBUTE_COLOR(singingColorButton, singingColorString, DEFAULTSINGINGCOLOR)
CONFIG_ATTRIBUTE_COLOR(calledColorButton,  calledColorString,  DEFAULTCALLEDCOLOR)
CONFIG_ATTRIBUTE_COLOR(extrasColorButton,  extrasColorString,  DEFAULTEXTRASCOLOR)

CONFIG_ATTRIBUTE_BOOLEAN(longTipCheckbox,tipLengthTimerEnabled, false)
CONFIG_ATTRIBUTE_BOOLEAN(thirtySecWarningCheckbox,tipLength30secEnabled, false)
CONFIG_ATTRIBUTE_COMBO(longTipLength,tipLengthTimerLength, 7)
CONFIG_ATTRIBUTE_COMBO(afterLongTipAction,tipLengthAlarmAction, 0)
CONFIG_ATTRIBUTE_BOOLEAN(breakTimerCheckbox, breakLengthTimerEnabled, false)
CONFIG_ATTRIBUTE_COMBO(breakLength,breakLengthTimerLength, 10)
CONFIG_ATTRIBUTE_COMBO(afterBreakAction,breakLengthAlarmAction, 10)
CONFIG_ATTRIBUTE_COMBO(comboBoxMusicFormat, SongFilenameFormat, SongFilenameBestGuess)
CONFIG_ATTRIBUTE_COMBO(comboBoxSessionDefault, SessionDefault, SessionDefaultPractice)
CONFIG_ATTRIBUTE_STRING(lineEditMusicTypeSinging, MusicTypeSinging, "singing;singers")
CONFIG_ATTRIBUTE_STRING(lineEditMusicTypePatter, MusicTypePatter, "patter;hoedown")

CONFIG_ATTRIBUTE_STRING(lineEditMusicTypeExtras, MusicTypeExtras, "extras;xtras")
CONFIG_ATTRIBUTE_STRING(lineEditMusicTypeCalled, MusicTypeCalled, "vocal;vocals;called")

CONFIG_ATTRIBUTE_STRING(lineEditToggleSequence, ToggleSingingPatterSequence, "")

CONFIG_ATTRIBUTE_BOOLEAN(EnableClockColoring,experimentalClockColoringEnabled, false)

CONFIG_ATTRIBUTE_BOOLEAN(initialBPMcheckbox,tryToSetInitialBPM, false)
CONFIG_ATTRIBUTE_BOOLEAN(checkBoxSwitchToLyricsOnPlay,switchToLyricsOnPlay, false)
CONFIG_ATTRIBUTE_INT(initialBPMLineEdit,initialBPM, (ushort)125)

CONFIG_ATTRIBUTE_SLIDER(limitVolumespinBox,limitVolume, (ushort)0) // spin boxes are done with setValue not setText, like sliders

CONFIG_ATTRIBUTE_BOOLEAN(useTimeRemainingCheckbox,useTimeRemaining, false)

//CONFIG_ATTRIBUTE_BOOLEAN(enableFlashCallsCheckbox,enableFlashCalls, false)

CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(normalizeTrackAudio, false);

CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(autostartplayback, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(forcemono, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(startplaybackoncountdowntimer, false)
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(startcountuptimeronplay, false)
CONFIG_ATTRIBUTE_STRING_NO_PREFS(default_dir, QDir::homePath())
CONFIG_ATTRIBUTE_STRING_NO_PREFS(default_playlist_dir, QDir::homePath())

CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(enablevoiceinput, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(enableautoscrolllyrics, false);

CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(enablegroupstation, true);  // defatuls to show group/station
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(enableordersequence, true); // defaults to show order/sequence

CONFIG_ATTRIBUTE_BOOLEAN(enableAutoAirplaneModeCheckbox, enableAutoAirplaneMode, false)
//CONFIG_ATTRIBUTE_BOOLEAN(enableAutoMicsOffCheckbox, enableAutoMicsOff, false)
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(showRecentColumn, true);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(showAgeColumn, true);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(showPitchColumn, true);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(showTempoColumn, true);

CONFIG_ATTRIBUTE_INT_NO_PREFS(songTableFontSize, 13);

// flash call menu
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(flashcallbasic, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(flashcallmainstream, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(flashcallplus, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(flashcalla1, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(flashcalla2, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(flashcallc1, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(flashcallc2, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(flashcallc3a, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(flashcallc3b, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(flashcalluserfile, false);

CONFIG_ATTRIBUTE_STRING_NO_PREFS(lastflashcalluserfile, "");
CONFIG_ATTRIBUTE_STRING_NO_PREFS(lastflashcalluserdirectory, "");

CONFIG_ATTRIBUTE_STRING_NO_PREFS(flashcalltiming, "10");
CONFIG_ATTRIBUTE_STRING_NO_PREFS(default_flashcards_file, "");

CONFIG_ATTRIBUTE_STRING_NO_PREFS(snap, "measure"); // three values: disabled, beat, measure

CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(showSongTags, true);
CONFIG_ATTRIBUTE_STRING_NO_PREFS(recentFenceDateTime, "2023-01-01T00:00:00Z"); // songs played earlier than this date are not "recently played", ISO8601

CONFIG_ATTRIBUTE_COLOR(pushButtonTagsBackgroundColor,  tagsBackgroundColorString,  DEFAULTTAGSBACKGROUNDCOLOR)
CONFIG_ATTRIBUTE_COLOR(pushButtonTagsForegroundColor,  tagsForegroundColorString,  DEFAULTTAGSFOREGROUNDCOLOR)

// SD Animation
CONFIG_ATTRIBUTE_COMBO(comboBoxAnimationSettings, AnimationSpeed, AnimationSpeedMedium)
CONFIG_ATTRIBUTE_BOOLEAN(checkBoxAutomaticEnterOnAnythingTabCompletion, AutomaticEnterOnAnythingTabCompletion, false)


CONFIG_ATTRIBUTE_INT_NO_PREFS(prefsDialogLastActiveTab, 0)
CONFIG_ATTRIBUTE_INT_NO_PREFS(SDCallListCopyOptions, 1)
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(SDCallListCopyHTMLIncludeHeaders, true);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(SDCallListCopyHTMLFormationsAsSVG, true);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(SDCallListCopyDeepUndoBuffer, true);

CONFIG_ATTRIBUTE_STRING_NO_PREFS(SDLevel, "Plus"); // SD's input level is persistent

CONFIG_ATTRIBUTE_INT_NO_PREFS(LastVersionOfKeyMappingDefaultsUsed, 1)

CONFIG_ATTRIBUTE_BOOLEAN(checkBoxInOutEditOnlyWhenLyricsUnlocked, InOutEditingOnlyWhenLyricsUnlocked, false);

// Global FX tab
CONFIG_ATTRIBUTE_BOOLEAN(intelBoostEnabledCheckbox, intelBoostIsEnabled, false)
CONFIG_ATTRIBUTE_SLIDER(intelCenterFreqDial, intelCenterFreq_KHz, 16)  // sliders are integers controlled by value()/setValue()
CONFIG_ATTRIBUTE_SLIDER(intelWidthDial, intelWidth_oct, 20)
CONFIG_ATTRIBUTE_SLIDER(intelGainDial, intelGain_dB, 30) // expressed as a positive number (actually tenths of a dB)

CONFIG_ATTRIBUTE_SLIDER(panEQGainDial, panEQGain_dB, 0)  // expressed as a signed number (actually tenths of a dB)

CONFIG_ATTRIBUTE_BOOLEAN(checkBoxSwapSDTabInputAndAvailableCallsSides, SwapSDTabInputAndAvailableCallsSides, false)
    
CONFIG_ATTRIBUTE_STRING_NO_PREFS(SDTabHorizontalSplitterPosition, "")
CONFIG_ATTRIBUTE_STRING_NO_PREFS(SDTabVerticalSplitterPosition, "")

CONFIG_ATTRIBUTE_STRING_NO_PREFS(lastPlaylistLoaded, "") // if the user had a playlist loaded, then reload it at next app start time (THIS IS PLAYLIST #1 = SLOT 0)
CONFIG_ATTRIBUTE_STRING_NO_PREFS(lastPlaylistLoaded2, "") // if the user had a playlist loaded, then reload it at next app start time (THIS IS PLAYLIST #2 = SLOT 1)
CONFIG_ATTRIBUTE_STRING_NO_PREFS(lastPlaylistLoaded3, "") // if the user had a playlist loaded, then reload it at next app start time (THIS IS PLAYLIST #3 = SLOT 2)

CONFIG_ATTRIBUTE_STRING_NO_PREFS(lastDance,  "")
