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

#include "songtempo.h"
#include "songsettings.h"

#include <cmath>

SongTempoRange tempoRangeForSong(int detectedBPM, double id3BPM, const QString &categoryName)
{
    // If the file has an embedded TBPM frame in the ID3 tag, then it overrides the MiniBPM auto-detect of BPM
    bool songBPM_fromID3tag = (id3BPM != 0.0);
    int songBPM = (songBPM_fromID3tag ? static_cast<int>(id3BPM) : detectedBPM);

    // Intentionally compare against a narrower range here than BPM detection, because BPM detection
    //   returns a number at the limits, when it's actually out of range.
    // An explicit ID3 TBPM tag is trusted by the user, so it gets a much wider sanity range
    //   than MiniBPM's auto-detected BPM (which is unreliable near its limits).
    // Also, turn off BPM for xtras (they are all over the place, including round dance cues, which have no BPM at all).
    //
    // TODO: make the types for turning off BPM detection a preference
    bool songBPMinRange = songBPM_fromID3tag ? (songBPM >= 60 && songBPM <= 300)
                                              : (songBPM >= 125-15 && songBPM <= 125+15);

    SongTempoRange range;
    range.baseBPM = songBPM;  // remember the base-level BPM of this song, for when the Tempo slider changes later

    if (songBPMinRange && categoryName != "extras") {
        range.tempoIsBPM   = true;
        range.minimum      = songBPM - 15;
        range.maximum      = songBPM + 15;
        range.defaultValue = songBPM;
    } else {
        range.tempoIsBPM   = false;
        // if we can't figure out a BPM, then use percent as a fallback (centered: 100%, range: +/-20%)
        range.minimum      = 100 - 20;     // allow +/-20%
        range.maximum      = 100 + 20;
        range.defaultValue = 100;
    }

    return(range);
}

int savedTempoForSlider(const SongSetting &settings, bool tempoIsBPM,
                        int minimum, int maximum, int currentValue)
{
    // A saved tempo is a bare number whose meaning depends on the units it was saved in:
    //   130 means 130 BPM or 130%, and tempoIsPercent is what says which.  tempoRangeForSong()
    //   has already chosen this load's units (tempoIsBPM) and the slider's range, so a
    //   saved value in the OTHER units is not a tempo for this slider at all -- applying it
    //   would just let setValue() clamp it to the nearest end of the range, silently, which
    //   is how a song ends up playing at 120% or pinned to the bottom of its BPM range.
    //
    //   There is no conversion to make here: a percent was taken against a base BPM we no
    //   longer know, so the honest thing is to keep the default tempoRangeForSong() just
    //   computed, which is correct for this song (issue #1757).
    int tempo = currentValue;
    if (settings.isSetTempo()) {
        const int savedTempo = settings.getTempo();
        if (settings.isSetTempoIsPercent()) {
            // tempoIsPercent is stored as !tempoIsBPM, so the units agree exactly when it
            //   differs from this load's tempoIsBPM.
            if (settings.getTempoIsPercent() != tempoIsBPM) {
                tempo = savedTempo;
            }
        } else if (savedTempo >= minimum && savedTempo <= maximum) {
            // A row from before the units were recorded.  Accept it only when it already fits
            //   this song's slider -- which is the test clamping would have applied anyway,
            //   just silently and with a wrong answer instead of no answer.
            tempo = savedTempo;
        }
    }
    return(tempo);
}

int tempoPercentForSliderValue(bool tempoIsBPM, double baseBPM, int value)
{
    if (tempoIsBPM) {
        double desiredBPM = static_cast<double>(value);            // desired BPM
        return(static_cast<int>(round(100.0*desiredBPM/baseBPM)));
    }

    double basePercent = 100.0;                      // original detected percent
    double desiredPercent = static_cast<double>(value);            // desired percent
    return(static_cast<int>(round(100.0*desiredPercent/basePercent)));
}
