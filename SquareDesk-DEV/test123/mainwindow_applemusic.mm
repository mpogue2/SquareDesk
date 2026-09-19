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

#import <Foundation/Foundation.h>
#import <iTunesLibrary/iTunesLibrary.h>

#include "mainwindow_applemusic.h"

std::vector<PlaylistTrack> readAllPlaylists(std::string &errorOut)
{
    std::vector<PlaylistTrack> result;

    @autoreleasepool {
        NSError *error = nil;
        ITLibrary *library = [ITLibrary libraryWithAPIVersion:@"1.1" error:&error];

        if (!library) {
            NSString *msg = error ? error.localizedDescription : @"unknown error";
            errorOut = std::string(msg.UTF8String)
                + "\n\nIf this is a permissions error, grant \"Media & Apple Music\" access"
                  " to SquareDesk in:\n"
                  "  System Settings > Privacy & Security > Media & Apple Music";
            return result;
        }

        // Build persistentID -> playlist map (needed to walk up folder hierarchy)
        NSMutableDictionary<NSNumber *, ITLibPlaylist *> *byID = [NSMutableDictionary dictionary];
        for (ITLibPlaylist *pl in library.allPlaylists)
            byID[pl.persistentID] = pl;

        // Walk from playlist up through parent folders, building "folder/subfolder/name"
        auto fullPath = [&](ITLibPlaylist *pl) -> std::string {
            NSMutableArray<NSString *> *parts = [NSMutableArray array];
            [parts insertObject:pl.name atIndex:0];
            NSNumber *pid = pl.parentID;
            while (pid) {
                ITLibPlaylist *parent = byID[pid];
                if (!parent) break;
                [parts insertObject:parent.name atIndex:0];
                pid = parent.parentID;
            }
            return [parts componentsJoinedByString:@"/"].UTF8String;
        };

        NSISO8601DateFormatter *iso8601 = [[NSISO8601DateFormatter alloc] init];

        for (ITLibPlaylist *playlist in library.allPlaylists) {
            if (playlist.kind != ITLibPlaylistKindSmart &&
                playlist.kind != ITLibPlaylistKindRegular) continue;

            // Skip Apple's own built-in playlists (Library, Music, Purchased, Recently Added,
            //   Top 25 Most Played, 90's Music, Loved Songs, ...).  Filtering on kind rather than
            //   on name catches all of them at once, and works in a non-English Music.app (issue #1740).
            if (playlist.distinguishedKind != ITLibDistinguishedPlaylistKindNone) continue;
            if (playlist.isPrimary) continue;   // the main Library playlist

            std::string type = (playlist.kind == ITLibPlaylistKindSmart) ? "smart" : "static";
            std::string name = fullPath(playlist);
            int itemNum = 1;

            for (ITLibMediaItem *item in playlist.items) {
                NSString *path = item.location.path;
                if (!path) continue;

                auto str = [](NSString *s) -> std::string {
                    return s ? s.UTF8String : "";
                };

                std::string modDate;
                if (item.modifiedDate)
                    modDate = [iso8601 stringFromDate:item.modifiedDate].UTF8String;

                // Issue #1744.  Both are nullable: addedDate can be missing on old library
                //   entries, and lastPlayedDate is nil for anything never played.
                std::string addedDate;
                if (item.addedDate)
                    addedDate = [iso8601 stringFromDate:item.addedDate].UTF8String;

                std::string lastPlayedDate;
                if (item.lastPlayedDate)
                    lastPlayedDate = [iso8601 stringFromDate:item.lastPlayedDate].UTF8String;

                result.push_back({
                    type,
                    name,
                    itemNum++,
                    path.UTF8String,
                    str(item.title),
                    str(item.artist.name),
                    str(item.composer),
                    str(item.genre),
                    (int)item.beatsPerMinute,
                    (int)item.rating,
                    (int)item.year,
                    str(item.grouping),
                    str([item valueForProperty:ITLibMediaItemPropertyWork]),
                    modDate,
                    str(item.album.title),
                    str(item.album.albumArtist),
                    str(item.comments),
                    (int)item.totalTime,
                    (bool)item.isRatingComputed,
                    addedDate,
                    lastPlayedDate
                });
            }
        }
    }

    return result;
}

