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

// This file is included in several places with different meanings. Do
// not put include guards on it.
SONGSETTING_ELEMENT(QString, Filename)
SONGSETTING_ELEMENT(QString, FilenameWithPath)
SONGSETTING_ELEMENT(QString, Songname)
SONGSETTING_ELEMENT(int, Volume)
SONGSETTING_ELEMENT(int, Pitch)
SONGSETTING_ELEMENT(int, Tempo)
SONGSETTING_ELEMENT(bool, TempoIsPercent)
SONGSETTING_ELEMENT(double, IntroPos)
SONGSETTING_ELEMENT(double, OutroPos)
SONGSETTING_ELEMENT(QString, CuesheetName)

SONGSETTING_ELEMENT(bool, IntroOutroIsTimeBased)
SONGSETTING_ELEMENT(double, SongLength)

SONGSETTING_ELEMENT(int, Treble)
SONGSETTING_ELEMENT(int, Bass)
SONGSETTING_ELEMENT(int, Midrange)

SONGSETTING_ELEMENT(int, Mix)

SONGSETTING_ELEMENT(int, Loop)

SONGSETTING_ELEMENT(QString, Tags)

//SONGSETTING_ELEMENT(double, ReplayGain)
