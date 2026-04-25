#pragma once
#include <string>
#include <vector>

struct PlaylistTrack {
    std::string playlistType;   // "smart" or "static"
    std::string playlistName;
    int         itemNumber;
    std::string absolutePath;
    std::string title;
    std::string artist;
    std::string composer;
    std::string genre;
    int         beatsPerMinute; // 0 = not set
    int         rating;         // 0 = not set, otherwise 20/40/60/80/100
    int         year;           // 0 = not set
    std::string grouping;
    std::string work;
    std::string modifiedDate;   // ISO 8601, empty if not set
};

// Returns all tracks from every playlist via the ITLibrary framework.
// On failure, returns an empty vector and sets errorOut.
std::vector<PlaylistTrack> readAllPlaylists(std::string &errorOut);
