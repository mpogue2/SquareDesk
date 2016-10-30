README for SquareDeskPlayer

This is my list of bugs and feature requests that I'd like to get to at some point.  I've
merged in the suggestions I was keeping in the .cpp file.

===========================
FIXED RECENTLY:
BUG: Tempo slider now goes to initial value when double-clicked.
FEATURE: Tip timers are now included (experimental feature, use Preferences to enable)
FEATURE: Playlists now supported.  Click in the # column, then wait 2 sec, and click again to edit.
FEATURE: Playlists can be saved in M3U or CSV format.  CSV format also saves your last pitch/tempo changes.
FEATURE: You can make a playlist that contains your favorites (name it anything you like).  You can
   also use Playlists to keep track of what you want to play at your next Club.  Just create a playlist
   for each club.
FEATURE: Resize of columns in songTable now resizes the search fields to match.
FEATURE: Next song, Previous song buttons take you to the next item in the song list.
FEATURE: Continuous play -- when a song reaches the end, the next song on the list will automatically
  start, if "Continuous Play" is turned on.
FEATURE: Double-clicking on the Tempo slider now brings you back to the original tempo (possibly
  one that you specified).

---
P1: When song table was stable-sorted (e.g. by clicking on Title), any attempt to use the search filters
      would put the search back to the default (by type, by label, by title).
P1: Combined the Label and Label # fields [Cal Campbell suggestion], and reordered them
P1: Bad fonts sizes on list of calls tabs, but only the 2nd label on a tab (Windows only)

P1: Allow "=" as well as "+" so don't have to hold shift down for shortcuts
P1: Add "&" for Windows shortcuts to Menu and subMenu items
P1: Removed a Mac-specific #pragma that was no longer needed.

P1: Allow Pitch, Volume, Mix to be changed, even without a song loaded yet [Don Beck suggestion]
P1: Allow LEFT/RIGHT arrows when a song is loaded, not just when playing
P1: Don't allow RIGHT ARROW to go off the end on the right hand side
P1: remove mainToolbar that's not needed (Windows only) - Note: could not delete it in QtDesigner

P1: added tick marks to the EQ sliders, and readjusted the width of the panel to match [Don Beck Suggestion]
P1: All EQ to be changed, even without a song loaded yet [Don Beck suggestion]
P1: some songs are not tempo-detected correctly (e.g. Possum Sop Long Play).
P1: Open File can give a really long filename (should remove the Label/Label# and " - ", like in the song list)
P1: "=" should be allowed for "+"

---
P1: Combine label and label #?  Might make it simpler. (DONE, 10/2/2016)
P1: Make it easy to get from the patter to the singing call (next tip function might do this) (PLAYLISTS, 10/9/2016)
P1: checkbox for "next tip", allow sort by this field (PLAYLISTS, 10/9/2016)
P1: checkbox for "favorites", allow sort by this field (PLAYLISTS, 10/9/2016)

P1: DONE: zero line on the EQ sliders
P1: ALREADY THERE: double click to set EQ to zero
P1: DONE: ability to set volume, tempo, pitch BEFORE a song is selected.  e.g. preset the volume level, or
        set tempo to a specific value (if I want to call everything at 122, I could preset it, but if a song
        has a saved tempo, that would override the setting)
P1: ALREADY THERE: Make it easy to scan patter and singing calls separately.  (I think this is 
		already there, using the type field which is searchable)
P2: I'd like to see both BPM and % for tempo.  (Preference, maybe?) (FIXED 10/9/16)
P1: Tab between search fields (FIXED 10/1/16)
P3: When click into a field, select text that is already there.  (Already true for the # field,
      and double-click does this for the search fields.)
P1: FUTURE: playlists?  (DONE, 10/2/2016)
P1: BUG: the song timer changes length (made more room and left-justified it)
P1: BUG: the Next and Previous song buttons should be disabled, until a song has been selected in songTable.
P1: BUG: Loadin an MP3 file manually should clear the songtable selection, and disable the Next/Previous buttons.

===========================
BUGS (high priority):

// BUG: Windows only -- the songTable should be less tall, so window can start out smaller and will
//        fit on most screens.  This works fine on Mac OS X, so the problem is Windows-specific.
// BUG: Windows only -- Alt-F and Alt-M shortcuts don't work, second keys are being swallowed by 
//        HandleKeypress(), before the menus get them.
// BUG: Windows only -- font sizes are not consistent with Mac font sizes (known problem with Qt, but 
//        fix is to set them manually)

===========================
Mike's current TODO list (lower priority):

// TODO: change loop points from default 0.9,0.1 to read out of the file, or find the start/end points and set them with menu item
//   must allow for fine tuning...  Also a Don Beck request.
// TODO: where does the "follow along" text go, for Synchronized Lyrics?
//   If it's like SqView, the whole bottom half (table) becomes the song (when playing only!)
// P1: Tip Timer
// P1: Break Timer (1 bong, 2 bongs, starts next music in playlist)
// FUTURE: patter: auto turn on "random call @ <level>" during PLAYBACK ONLY
// FUTURE: singing: auto turn on lyrics during PLAYBACK ONLY (like SqView)
// FUTURE: preview mode (plays first few sec of any song you click on, without loading it)
// FUTURE: audio level indicator (helps debug output problems)
// FUTURE: singing call should tell me OPENER/BREAK/CLOSER/1/2/3/4 (auto switches to this mode for singing calls)
//   Can make this approximate, just by song length (divided by 7 or so, like SqView does...)
//   Also Don Beck request.
// FIX: rounds with split music and cues don't work the way I expected in MONO mode.  Entire mono mix 
//      moves L and R. This is because mix-to-mono is after the Mix mixer, should be before.
// TODO: add a Favorites column, that can be used to flag songs as favorites.  Then, make
//   favorites be the default at app open time.  Make the favorites a number, so we can sort by that number.
//   This is a very simple one-playlist implementation.  (NOTE: This is what Don Beck is asking for, too)
//   Now that playlists is in, allow a magic named playlist, called "favorites.m3u" which always
//   is pre-loaded, if set.
// TODO: Preference: font size (for people to read without their glasses) (also Don Beck)

====================

Extracted from Cal Campbells' email suggestions:

11) P1: Cal would like to see TIME REMAINING, rather than TIME PLAYED.  Preference, maybe?
4) P1: when adding folders under the Master folder, have a file watcher that will auto-repopulate the table
6) P1: should pay attention to existing metadata (especially iTunes metadata), rather than filenames.
    Metadata should override the filenames (Cal uses title, artist, album, and genre)
16) P1: First time through, ask where the Music Directory should be.  (Pop up the preferences dialog?)
17) P1: Linux version! (Dan)

1) P2: Canonical name for the folder, e.g. "SquareDanceMusic", look in exactly 3 places for it:
    Desktop, Documents, or Music
2) P2: Use iTunes library and/or playlists (maybe)
3) P2: preference: allow search to be words only (vs parts of words, which is the way I like it...)
8) P2: Allow turning off certain columns?  Like Label and #...
12) P2: VU Meter! (stereo, so we can do the L/R trick with cued rounds)

Probably won't do:
There are some features that Cal requested that I do not plan to implement (because I disagree that it's correct
to do so):
9) P3: Cal doesn't like SQUARE used for Stop/rewind.  He likes TWO_TRIANGLES_FAST_REWIND.
    I prefer the Square, because it follows the 2-button standard on the web, that I have seen several times (and
    I think it's more consistent with tape recorders and CD players with only 2 buttons).

===================
Extracted from Don Beck's email suggestions:

P1: frequency count of play times, allow sort by this field (I think Dan has this in the Python player)
P1: Save EQ for each song, maybe also in the playlist?  Bass=+5,Mid=-2,Treble=+1 in the comment?
P1: Ability to set loop points for each song
P2: Fade out button would be nice (like SqView).
P1: Singing call indicator (7 segments or equivalent)
P1: Fonts should resize to be bigger when window is resized to be bigger
P2: Make buttons bigger (also resize when window resizes?), use keyboard instead for now?
P2: Modify lists to include X/O for each call!  (stick these in TEXT files)

Extracted from Don Beck's SECOND ROUND of email suggestions:
P1: Stereo/Mono button is hard to tell whether it shows the mode, or what you go to when pressing it
P1: All column heads were highlighted?  (Could be due to him being on a VERY old Mac OS release)
P3: Black text on blue band (almost certainly the old Mac OS X release.  I don't see this on El Capitan.)
P1: Put the fact that the search fields work together into the Manual.

===========================
from: Don Beck
Hi Mike,

I just took a quick look at the version you released a few days ago and here are some comments. I haven’t had a chance to download today’s release, so I hope I am not being redundant. I’ll try to get to the latest version soon.

September 30, 2016

Stereo/Mono button: It’s hard to tell, when it says Stereo, whether it means it is set to Stereo or whether you should press it to make it Stereo. Same with Loop button. This is a common interface problem with many apps.

I had a condition where all of the column heads of the music field where high lighted. After clicking on one of them, to sort the column, only that header was highlighted. I haven’t been able to duplicate that, but while it was like that, I assumed they were all highlighted as a style preference. Now I see that only one is supposed to be highlighted to show which column the songs are sorted by. If I figure out how to do it again, I will let you know. I tried rebooting the app, but that didn’t do it. It could just be that your initial build has it that way, and once it is sorted on a column, you keep that state in memory. Before I noted that only one of the cells in the first row was selected, I wrote the next paragraph. Now, it only applies to the selected column.

The text on the blue band at the top of the list of songs is black, and is impossible to read against the blue back ground. Make it white, White, like the text on the buttons at the top of the window.

It is nice that you can resize the columns in the list of music table. It would be nice if the search fields above the columns would automatically resize at the same time.

Is it possible to read the BPM of the music, and have the Tempo slider include the corrected BPM as well as the percent of original tempo?

When you are in one of the search fields, and press Tab, the expected behavior is to change focus to the next field. Your fields appear to be tables with only one cell showing, and tabbing takes you to the next cell, making the display look weird. The only way that I can get out of this is to press the Right Arrow key a lot to get to the end of the tabs, and then press Delete several times to start over.

It would be nice, if when you clicked [or tabbed] into a new search field, the text that was already in there would select, so you didn’t have to delete before typing a new search criteria.

It is nice, that when you type into one of the search fields, you can refine your search by typing into one of the other ones. Eventually, you should include this feature in the manual.

===========================
Hi Mike,

Here is a list of features that I think would be very helpful to the SquareDesk user. If they could be added without too much problem, I think it would greatly enhance the use of the app. As I think I have said  before, I've only used SqView before, and haven't used it much. Some of my suggestions are taken from SqView, and at least one is, to my knowledge, not.

First, a bit about how I used SqView. I didn't create playlists like many people seem to do, and I didn't choose all my music before the dance. What I did do was to have all of my patter call music in one field, and my singing calls in another field. I didn't use third field. Before each tip, I would visually scan the patter list and choose what music I was going to use that tip. Same with the singers. I would try to have the singer as easy to get to after the patter as possible. (This is similar to what I did with vinyl. One section of the record case had patters, one had singers. The more frequently used ones were nearer the front of each section. Before each tip, actually at the end of the previous tip, I would thumb through each pile for what I wanted to use next, and put those two records by the turntable.)

If I had to remember the name of some music and search for it, like you require, I would have troubles. Being able to search is good, but being able to visually scan is even more important to me.

What I am guessing would be the easiest way to implement this is to be able to sort by each of the columns that you have listed (by clicking on the header of the column.)

Add a check box to each row that you can put a P for patter or an S for singer or an R for rounds, etc. This would allow me to sort by patter/singers, etc. and would alleviate the need for a second field.

I would then want two more columns that contained only check boxes. In one, I would check the patter and the singer that I wanted to do during the next tip, and when I sorted by it, I would have my music for the next tip right at the top of the list. The second new column would be for Favorites or Suggestions or something similar. I could scan through all of my music well before the dance and check off music that would be a possibility to use for the upcoming MS dance or Adv dance or party night, etc. Sorting by this column would make it easier to visually scan for the next tip.

Here's an idea that I don't think any of the square dance apps have, but I think iTunes does. Keep a frequency count of how many times each song has been used, display it in a column, and be able to sort by this column. This could serve two purposes. It could either be used to choose music for the next tip, since these are songs you are currently using; or it could be a reminder of which songs you use too frequently.

Moving on. Setting the tempo is an important feature, but you should be able to set the tempo and save it for each song. You shouldn't have to adjust it each time you use it. I would not want it to remember the last tempo you used with each song, but only a tempo that you set and save.

Currently, you can only move pitch, tempo, volume, etc. when a song is selected. It would be nice to be able to set these before selecting a song, so that you could preset the volume to something close or set the tempo to a given tempo before the music starts, i.e. if I want to call everything at 122 bpm, I could pre-set it. (If a given song had a saved tempo, it would default to that.)

I like how large the range of the equalizer is, but would like to see a zero line or notch, so you could easily set it to the original setting. It might also be nice to have a Save button for each song, so that you can pre-set equalization for individual songs.

Looping is obviously necessary for patter, but it would be great if you could set the looping points and save them for each song. I like to set mine so that it starts and ends with the phrase of that particular piece of music. It would also be nice to have a button and keyboard command to end looping (which you probably already have, but I can't check right now), so that you can go out with the end of the record if you so choose.

I usually set my loop to start about half way through the song, so that I am reminded that once and a half through the record is about the recommended length for that half of the tip (because I tend to go much longer).

SqView has a fade out button which would also be nice, but not necessary for me, since I have rewired my remote volume to go all the way to Off.

On SqView, the timeline is broken into seven segments, i.e. background shading. This is a great help to me when doing singing calls to tell, at a glance, which figure I am on, incase I have the occasional brain freeze. This would probably be easy to implement.

On the Callerlab lists, knowing that a few of us are old enough to have had our eyesight deteriorate a little already, it would be nice to have the font be as large as possible. In particular, if it is not too hard to implement, please have the font resize when you resize the window, so it always fills the available space.

While on the subject of easier to see, please make all buttons as large as possible. I'm also referring to the buttons themselves, not just the text. When you are calling and want to quickly click on something while standing with a mike in one had and a mouse in the other, it would be nice to have a bigger target than what is necessary when you are sitting at your computer, fully focused on it.

And lastly, along the lines of making things easy to do while doing ten other things while calling, your keyboard commands are great, but please make them non-case sensitive. When I reach down and press a key, I want it to work whether or not the caps-lock is on or not. When I want to change the pitch with "-" or "+", where "+" = "Shift-+", I don't want to have to press the Shift key, i.e. make it okay to also press just the "=" key. It is fine to make it work with the Shift key also, for the literal minded. (e.g. Quicken allows you to do that, and it makes it very convenient.) I notice that in many cases, you make the Cmd key optional; this is great.

Well, so much for me just having just a *few* suggestions, for me keeping my email short, and for me agreeing with you that SquareDesk should stick to the basics! I hope that I was clear with my suggestions, and that you might even consider implementing some of the easier ones. Let me know if you have any questions.

I'm also curious to see if you have had any other similar requests. I look following the adventures of SquareDesk. I'm pretty sure that I signed up to be on your mailing list; I sure hope I got around to it.

Another thought is to have suggestions like mine (although condensed) be made available to everyone on the mailing list, so that we can refine each others thoughts or implementations. Brainstorming is always good, i.e. my ideas might trigger better, more useful, or easier to implement ideas from others and vise versa.

Here's hoping these ideas help rather than hinder your project. Again, as a Mac user, thank you.

Don

p.s. Next I'll probably want you to modify the Callerlab lists by adding Xs or Os after each call!

Don Beck
Chilmark, MA, USA
donbeck@donbeck.org
Follow me on Twitter @donbeck
www.donbeck.org
- at your beck and call -
