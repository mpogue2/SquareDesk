/****************************************************************************
**
** Copyright (C) 2016-2020 Mike Pogue, Dan Lyke
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

#ifndef COMMON_ENUMS_H
#define COMMON_ENUMS_H

enum SongFilenameMatchingType {
    SongFilenameLabelDashName = 1,
    SongFilenameNameDashLabel,
    SongFilenameBestGuess
};

enum SessionDefaultType {
    SessionDefaultPractice = 1,
    SessionDefaultDOW
};

enum ColumnExportData
{
    ExportDataFileName = 0,
    ExportDataPitch,
    ExportDataTempo,
    ExportDataIntro,
    ExportDataOutro,
    ExportDataVolume,
    ExportDataCuesheetPath,
    ExportDataNone
};

enum CheckerColorScheme
{
    CheckerColorSchemeNormal = 0,
    CheckerColorSchemeColorOnly,
    CheckerColorSchemeMentalImage,
    CheckerColorSchemeSight
};

enum AnimationSpeed
{
    AnimationSpeedOff = 0,
    AnimationSpeedSlow,
    AnimationSpeedMedium,
    AnimationSpeedFast,
};

enum Gender
{
    Boys = 0,
    Girls,
    UnknownGender
};

enum Order
{
    InOrder = 0,
    OutOfOrder,
    UnknownOrder
};

#endif // ifndef COMMON_ENUMS_H
