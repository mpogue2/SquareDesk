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

#include <cstdio>   // snprintf, for formatting persistentID (issue #1747)
#include <cstdint>  // SIZE_MAX
#include <unordered_map>

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

        // A '/' inside a playlist or folder name (e.g. "Mike's playlist/fun") would be taken for a
        //   folder separator by everything downstream -- the treeWidget, tree selection, the Type
        //   column, palette slots -- all of which split the hierarchical name on '/'.  So swap it
        //   for U+29F8 BIG SOLIDUS, which looks like a slash on screen but splits on nothing.
        //   (U+2215 DIVISION SLASH and U+2044 FRACTION SLASH were tried first, but the system font
        //   kerns the next letter in under them, so "playlist/fun" came out jammed together.)
        //   Music never sees the result, because ITLibrary is read-only (issue #1778).
        //   Keep in sync with APPLE_MUSIC_SLASH_UNICODE in common_enums.h, which titles use.
        auto component = [](NSString *name) -> NSString * {
            return name ? [name stringByReplacingOccurrencesOfString:@"/" withString:@"\u29F8"] : @"";
        };

        // Walk from playlist up through parent folders, building "folder/subfolder/name"
        auto fullPath = [&](ITLibPlaylist *pl) -> std::string {
            NSMutableArray<NSString *> *parts = [NSMutableArray array];
            [parts insertObject:component(pl.name) atIndex:0];
            NSNumber *pid = pl.parentID;
            while (pid) {
                ITLibPlaylist *parent = byID[pid];
                if (!parent) break;
                [parts insertObject:component(parent.name) atIndex:0];
                pid = parent.parentID;
            }
            return [parts componentsJoinedByString:@"/"].UTF8String;
        };

        NSISO8601DateFormatter *iso8601 = [[NSISO8601DateFormatter alloc] init];

        // A song on several playlists comes back once per playlist, and its fields are the same
        //   every time.  Reading item.location is nearly all of this function's cost (~0.1-0.3ms
        //   per item, against a few microseconds for every other field put together), so convert
        //   each song once, keyed on Music's persistentID, and copy it for its other playlists.
        //   persistentID itself is free to read (issue #1784).
        static const size_t kNoPath = SIZE_MAX;               // a song with no file: skip it every time
        std::unordered_map<unsigned long long, size_t> firstIndexByID; // persistentID -> its first entry in result

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
                const unsigned long long itemID = item.persistentID ? [item.persistentID unsignedLongLongValue] : 0;
                if (itemID != 0) {
                    auto seen = firstIndexByID.find(itemID);
                    if (seen != firstIndexByID.end()) {
                        if (seen->second == kNoPath) continue;
                        PlaylistTrack copy = result[seen->second];   // a copy: push_back may reallocate
                        copy.playlistType = type;
                        copy.playlistName = name;
                        copy.itemNumber   = itemNum++;
                        result.push_back(std::move(copy));
                        continue;
                    }
                }

                NSString *path = item.location.path;
                if (!path) {
                    if (itemID != 0) firstIndexByID[itemID] = kNoPath;
                    continue;
                }
                if (itemID != 0) firstIndexByID[itemID] = result.size();   // the entry pushed below

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

                // 16 lowercase hex digits.  This is what the songs table keys an Apple Music
                //   track on, so the spelling has to be stable -- hence the explicit width and
                //   case rather than whatever %llx happens to produce (issue #1747).
                std::string persistentID;
                if (item.persistentID) {
                    char pidBuf[17];
                    snprintf(pidBuf, sizeof(pidBuf), "%016llx",
                             (unsigned long long)[item.persistentID unsignedLongLongValue]);
                    persistentID = pidBuf;
                }

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
                    lastPlayedDate,
                    persistentID
                });
            }
        }
    }

    return result;
}

