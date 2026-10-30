/****************************************************************************
**
** Copyright (C) 2016-2026 Mike Pogue, Dan Lyke
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

#ifndef SONGTEMPO_H
#define SONGTEMPO_H

#include <QString>

class SongSetting;

// How loading a song decides what the Tempo slider says, and what that means to the audio
//   engine.  This lives on its own so that a real load (handleDurationBPM() and
//   loadSettingsForSong()) and Audition (issue #1817) make exactly the same decision: if they
//   each had their own copy, an audition would sooner or later play a song at a tempo the
//   real load never would.

// The units, range and default of the Tempo slider for one song.
struct SongTempoRange {
    bool tempoIsBPM;    // true: the slider is in BPM, false: it is in percent
    int  baseBPM;       // the song's own BPM (only meaningful when tempoIsBPM)
    int  minimum;
    int  maximum;
    int  defaultValue;  // where the slider starts (and goes when double-clicked)
};

// detectedBPM is MiniBPM's answer (already rounded), id3BPM is the ID3 TBPM tag (0.0 if none).
SongTempoRange tempoRangeForSong(int detectedBPM, double id3BPM, const QString &categoryName);

// The slider value a song's saved settings ask for, or currentValue if they don't give one that
//   is valid in these units (issue #1757).  NOT clamped: QSlider::setValue() does that.
int savedTempoForSlider(const SongSetting &settings, bool tempoIsBPM,
                        int minimum, int maximum, int currentValue);

// Slider value --> the audio engine's tempo, in percent (100 = as recorded).
int tempoPercentForSliderValue(bool tempoIsBPM, double baseBPM, int value);

#endif // SONGTEMPO_H
