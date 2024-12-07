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

#ifndef UPDATEID3TAGSMANAGER_H
#define UPDATEID3TAGSMANAGER_H

#include <stdint.h>

class updateID3TagsDialog;

class updateID3TagsManager
{
public:
    updateID3TagsManager();

    void populateUpdateID3TagsDialog(updateID3TagsDialog *updateDialog);
    void extractValuesFromUpdateID3TagsDialog(updateID3TagsDialog *updateDialog);

public:
    int currentTBPM;
    int newTBPM;

    uint64_t currentLoopStart;
    uint64_t newLoopStart;

    uint64_t currentLoopLength;
    uint64_t newLoopLength;
};

#endif // UPDATEID3TAGSMANAGER_H
