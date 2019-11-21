#include <qglobal.h>

#ifdef Q_OS_MAC

#include <QCoreApplication>
#include <QDebug>

#include <QFile>
#include <QUrl>
#include <QXmlStreamReader>

// This class reads the output of Catalina's Apple Music (used to be iTunes) "Export Library..."
//   function.  Output is usually called "Library.xml", and the user can place it anywhere.
// Pass that pathname to the constructor, and it will read in ALL the playlist information,
//   and ONLY the track information (location/pathname, title, genre, and BPM) for those tracks
//   that are in a playlist.  (Right now, that's all we need, since normal users will store
//   all tracks in the MUSICDIR.)
//
// Currently takes about 1 second on my laptop to get all the info (my Library.xml file is ~19K lines of text).
//
// This is a TWO-pass parser, for efficiency (i.e.. so we don't have to read in the whole file at once).
// Apple's format is stateful, that is, you can't just use DOM or xQuery to pull out what you want.  We must
//   parse the file one line or element at a time, and maintain state.
//
// NOTE: if a user has LOTS of tracks in Apple Music, most likely the playlists refer to only a very small fraction of those.
//   e.g. Eric H has ~20-30 playlists that he uses regularly, and ~21K music files.  Most likely, the playlists
//        have on the order of 20 files max per playlist.
//
// Pass 1: get playlists, and identify the tracks therein (just by number)
// Pass 2: get track infos, but ONLY the ones that are in playlists
//
// Output is in a bunch of QSet/QMaps, which can then be perused as desired:
// reader.allPlaylists      = QMap<strPlaylistName, qVector<trackID>
// reader.allTrackNumbers   = QSet<trackID>
// reader.knownTrackLocations = QMap<trackID, pathname>
// reader.knownTrackTitles  = QMap<trackID, strTitle>
// reader.knownTrackGenres  = QMap<trackID, genre>, e.g. "singing" or "patter" or "vocals", etc.
// reader.knownTrackBPMs    = QMap<trackID, intBPM>, e.g. presumed accurate BPM for this song
//
// The idea here is to add some new Mac-specific menu items, perhaps "Load iTunes Playlist... > Mike's Playlist"
//   which would provide a way to load in iTunes playlists, and play songs that are NOT in the MUSICDIR.
//   If "genre" is set to a known string, e.g. "patter", we'd use that as its type.  Otherwise, it would be typeless.
//   If "BPM" is set, that would override the auto-BPM detector.
//   The "Title" would override the one derived from the filename.
//   The "Location" would be the pathname, as expected.
// This seems rather modal, because we'd need a way to get back, maybe?  Or, should it just merge into
//   the songList, and disappear if a different iTunes Playlist is loaded (similar to how that happens currently).
//   The playlist order would be immutable, and not stored for these songs in the sqlite DB.

class AppleMusicLibraryXMLReader
{
public:
    AppleMusicLibraryXMLReader(QString filename) {
        libraryFile.setFileName(filename);

        // PASS 1: get playlist info
        getPlaylists();

        // PASS 2: get Track Locations, Names, Genres, BPMs
        getTrackInfos();  // get the titles and locations of just playlist tracks
    }

    ~AppleMusicLibraryXMLReader() {
    }

    QString key;
    QString lastTrackID;
    QString lastChars;
    QString currentTrackName, currentPlaylistName, currentFileLocation;

    int pass = 0;  // 1 = playlist pass, 2 = track info pass

    enum mode {nothing, tracks, playlists} currentMode = nothing;
    bool seenVisible = false;
    bool currentVisible = true;

    QMap<int, QString> knownTrackLocations; // e.g. 123 => "/Users/mpogue/SquareDeskMusic/foo.mp3"
    QMap<int, QString> knownTrackTitles; // e.g. 123 => "Foo (Karaoke version)"
    QMap<int, QString> knownTrackGenres; // e.g. 123 => "patter"  // NOTE: always lowercase
    QMap<int, int> knownTrackBPMs; // e.g. 123 => 127

    bool readThing() {
        QXmlStreamReader::TokenType t = xml.readNext();
        switch (t) {
            case QXmlStreamReader::StartElement:
                if (xml.name() == "true" || xml.name() == "false") {
//                    qDebug() << "T/F!";
                    if (key == "Visible") {
//                        qDebug() << "Visible: " << xml.name();
                        currentVisible = (xml.name() == "true");
                    }
                }
                break;
            case QXmlStreamReader::EndElement:
                if (xml.name() == "key") {
                    key = lastChars;
                    if (key == "Tracks") {
//                        qDebug() << "****** TRACKS ******";
                        currentMode = tracks;
                    } else if (key == "Playlists") {
//                        qDebug() << "****** PLAYLISTS ******";
                        currentMode = playlists;
                    } else if (key == "Visible") {
                        seenVisible = true;
//                        qDebug() << "Visible!";
                    } else {
                        seenVisible = false;
                    }
                } else if (xml.name() == "integer") {
                    if (key == "Track ID") {
                        if (currentMode == tracks) {
                            lastTrackID = lastChars;
                        } else if (currentMode == playlists) {
                            lastTrackID = lastChars;
                            if (currentVisible) {
//                                qDebug().noquote() << "    PlaylistTrackID " << lastTrackID << " = " << knownTracks[lastTrackID];
//                                qDebug() << currentPlaylistName << ": " << lastTrackID;

                                // ***** ADD TRACKID TO PLAYLIST *****
                                // ***** ADD TRACKID TO TRACKNUMBERS *****
                                if (pass == 1) {
                                    int iTrackID = lastTrackID.toInt();
                                    QVector<int> tr = allPlaylists.value(currentPlaylistName); // get current list
                                    tr.append(iTrackID);
                                    allPlaylists[currentPlaylistName] = tr;  // append, and stick it back in

                                    allTrackNumbers.insert(iTrackID);
    //                                qDebug() << "allPlaylists: " << allPlaylists;
                                }
                            }
                        }
                    } else if (key == "BPM") {
                        int iTrackID = lastTrackID.toInt();
                        if (pass == 2 && allTrackNumbers.contains(iTrackID)) {
                            int currentBPM = lastChars.toInt();
                            knownTrackBPMs[iTrackID] = currentBPM;  // save in the QMap<int, int>
                        }
                    }
                } else if (xml.name() == "string") {
                    int iTrackID = lastTrackID.toInt();
                    if (key == "Location") {
                        if (pass == 2 && allTrackNumbers.contains(iTrackID)) {
                            currentFileLocation = QUrl::fromPercentEncoding( lastChars.replace("file://", "").toUtf8() );
                            knownTrackLocations[iTrackID] = currentFileLocation;  // save in the QMap<int, QString>
                        }
                    } else if (key == "Genre") {
                        if (pass == 2 && allTrackNumbers.contains(iTrackID)) {
                            QString currentGenre = lastChars.toLower();
                            knownTrackGenres[iTrackID] = currentGenre;  // save in the QMap<int, QString>
                        }
//                    } else if (key == "BPM") {
//                        if (pass == 2 && allTrackNumbers.contains(iTrackID)) {
//                            QString currentBPM = lastChars;  // NOTE: still a string!
//                            knownTrackBPMs[iTrackID] = currentBPM;  // save in the QMap<int, QString>
//                        }
                    } else if (key == "Name") {
                        if (currentMode == tracks) {
                            currentTrackName = lastChars;  // Name is a track or playlist name playlist name
                            int iTrackID = lastTrackID.toInt();
                            if (pass == 2 && allTrackNumbers.contains(iTrackID)) {
                                knownTrackTitles[iTrackID] = currentTrackName;
                            }
//                            qDebug() << "Track Name: " << currentTrackName;
                        } else if (currentMode == playlists) {
                            currentPlaylistName = lastChars;  // Name is a track or playlist name playlist name
                            currentVisible = !((currentPlaylistName == "Library") ||
                                               (currentPlaylistName == "Downloaded") ||
                                               (currentPlaylistName == "Music"));
                            if (currentVisible) {
//                                qDebug() << "Playlist Name: " << currentPlaylistName;
                                if (pass == 1) {
                                    allPlaylists[currentPlaylistName] = QVector<int>();
    //                                qDebug() << "allPlaylists: " << allPlaylists;
                                }
                            }
                        }
                    }
                }
                break;
            case QXmlStreamReader::Characters:
                lastChars = *(xml.text().string());
                break;
            default:
                break;
        }
        return(t == QXmlStreamReader::EndDocument);
    }

    bool read(QIODevice *device, int whichPass) {

        // OPEN FILE -----------
//        qDebug() << "\n*** Trying to open file: " << libraryFile.fileName();
        if (!libraryFile.open(QFile::ReadOnly | QFile::Text)) {
            qDebug() << "ERROR: " << libraryFile.errorString();
        }

        pass = whichPass; // the same code is used twice to parse  (FIX: separate into two parsers later)
        xml.setDevice(device);  // XMLstreamreader now ready to read open libraryFile

        if (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("plist") &&
                xml.attributes().value(versionAttribute()) == QLatin1String("1.0")) {
                while (!readThing());
            } else {
                xml.raiseError(QObject::tr("The file is not a Music Library.xml version 1.0 file."));
            }
        }

        if (libraryFile.isOpen()) {
//            qDebug() << "*** closing file: " << libraryFile.fileName();
            libraryFile.close();
        }

        return !xml.error();
    }

    QString errorString() const {
        return QObject::tr("%1\nLine %2, column %3")
                    .arg(xml.errorString())
                    .arg(xml.lineNumber())
                    .arg(xml.columnNumber());
    }

    static inline QString versionAttribute() { return QStringLiteral("version"); }
    static inline QString hrefAttribute() { return QStringLiteral("href"); }
    static inline QString foldedAttribute() { return QStringLiteral("folded"); }

    QMap<QString, QVector<int>> allPlaylists;  // a playlist name, and an ordered list of track numbers (ints)
    QSet<int> allTrackNumbers;          // all track numbers contained in any playlist (for fast lookup when reading track infos)

    // get the PLAYLISTS
    void getPlaylists() {
        read(&libraryFile, 1);  // opens, then closes the file
    }

    // get the titles and locations of just playlist tracks
    //   NOTE: assumes that getPlaylists() has already been called
    void getTrackInfos() {
        read(&libraryFile, 2);  // opens, then closes the file  // FIX: just rewind
    }

private:
    QFile libraryFile;
    QXmlStreamReader xml;
};

#endif
