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
                    modDate
                });
            }
        }
    }

    return result;
}
