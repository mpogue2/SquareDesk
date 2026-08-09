# Building and running SquareDesk under AddressSanitizer

Added while chasing the quit-time crashes in issue #1686.

ASan is off unless you ask for it. Normal Debug and Release builds are completely
unaffected — everything lives inside an `asan { ... }` block at the bottom of
`test123/test123.pro` that is only entered when qmake is given `CONFIG+=asan`.

## Why

The quit-time crash reports tell us *where* the program died but not *why*. The most
recent one died in `~darkSongTitleLabel` freeing `0xffffe1e05a34a648` — which is the
bitwise inverse of `0x1e1fa5cb59b7`, a normal heap pointer. macOS libmalloc stores
free-list links inverted inside freed blocks, so that address is free-list metadata:
the object had already been freed, and its memory reused, before the destructor ran.
That is a use-after-free / double-delete.

A crash report cannot tell us who freed it the first time. ASan can: it reports at the
moment of the second free and prints **three** stacks — where the bad free happened,
where the memory was freed the first time, and where it was originally allocated. That
third-and-second pair is the whole answer.

## Building

### QtCreator (what Mike uses)

1. **Projects** → **Build & Run** → your Qt kit → **Build**
2. Clone the **Debug** configuration; name the clone `ASan`
3. Give it its own build directory, e.g.
   `/Users/mpogue/clean3/SquareDesk/build-SquareDesk-Qt_6_10_1_for_macOS-ASan`
   (do **not** reuse the Debug directory — the object files are incompatible)
4. On the **qmake** build step, add to *Additional arguments*:
   ```
   CONFIG+=asan
   ```
5. Build. You should see `*** ADDRESS SANITIZER BUILD ENABLED (CONFIG+=asan) ***` in the
   build log — if you don't, the argument didn't take.

Then just **Run** normally. QtCreator shows ASan's output in the Application Output pane.

### Use Qt 6.10.1, not 6.10.3

Build ASan (and everything else) against **Qt 6.10.1**. Qt 6.10.3 has a CoreAudio bug that
crashes SquareDesk every time an audio device goes away — unplugging the headphone jack, or a
sleep/wake cycle. See issues #1693 and #1683.

Briefly: 6.10.3 registers its audio device-disconnect listener as an ObjC *block*
(`AudioObjectAddPropertyListenerBlock`), but does not retain a reference to it. macOS frees
that block when it destroys the `HALDevice` for the departing device, and Qt then passes the
freed block back to `AudioObjectRemovePropertyListenerBlock()`, which retains it — crash in
`objc_retain`. 6.10.1 registers a plain C callback function pointer instead, so there is no
block for CoreAudio to free and the bug cannot happen.

If an ASan session is dying on device changes rather than on whatever you are chasing, check
which Qt version the kit points at first.

### Command line

```sh
mkdir -p ~/clean3/SquareDesk/build-asan && cd ~/clean3/SquareDesk/build-asan
~/Qt/6.10.1/macos/bin/qmake ../SquareDesk-DEV/SquareDesk.pro CONFIG+=asan CONFIG+=debug
make -j8
```

`CONFIG+=asan` propagates from the top-level subdirs project to `test123`. `taglib` and
`sdlib` ignore it and build uninstrumented; that is fine and intentional (see below).

**Don't expect the ASan banner from the qmake step.** `SquareDesk.pro` is a subdirs
project, so the top-level qmake prints nothing but `creating stash file`, and only writes
the top-level Makefile. The `*** ADDRESS SANITIZER BUILD ENABLED ***` message comes from
`test123.pro`, which qmake does not read until `make` recurses into it. To confirm the flag
took effect without doing a full build:

```sh
make qmake_all                                  # generates the sub-Makefiles, no compiling
grep -m1 '^CXXFLAGS' test123/Makefile | tr ' ' '\n' | grep fsanitize
```

Verified 2026-08-07, the whole way through — qmake, `make -j8`, and the resulting binary:

- `CXXFLAGS` gets `-fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls
  -O1 -g`, `LFLAGS` gets `-fsanitize=address`, `DEFINES` gets `-DSQUAREDESK_ASAN`
- `sdlib/Makefile` correctly gets no ASan flags at all
- the build completes with **exit 0 and zero errors** (there are ~5000 pre-existing
  `-Wall -Wextra` warnings from the codebase and its vendored dependencies; the ASan flags
  add none of them)
- the linked binary really is instrumented, not merely compiled with the flags:
  ```sh
  otool -L test123/SquareDesk.app/Contents/MacOS/SquareDesk | grep asan
  #  @rpath/libclang_rt.asan_osx_dynamic.dylib
  ```

## Running

`ASAN_OPTIONS` is read by the ASan runtime when the program starts, so it is a **run**-time
setting, not a build-time one. In QtCreator it does *not* go anywhere near the qmake step
where you added `CONFIG+=asan` — that catches people out.

### Setting ASAN_OPTIONS in QtCreator

1. Left sidebar → **Projects**
2. **Build & Run** → your Qt kit → click **Run** (not Build)
3. Scroll to the **Environment** section — you may have to expand **Details**
4. **Add**:
   - Name: `ASAN_OPTIONS`
   - Value: `abort_on_error=1:malloc_context_size=50:detect_leaks=0`

Add `QTWEBENGINE_CHROMIUM_FLAGS` here too if you hit the web-engine noise described below.

Run settings in QtCreator are **per-kit, not per-build-configuration**, so this one entry
applies to the Debug, Release and ASan builds alike. That is harmless and you can leave it
set: a binary built without `-fsanitize=address` contains no ASan runtime and ignores
`ASAN_OPTIONS` entirely.

Either **Run** or **Debug** works, and the choice matters more than it looks:

- **Run** — nothing interrupts you. JUCE's `jassertfalse` checks
  `juce_isRunningUnderDebugger()` before breaking, so with no debugger attached its
  assertions only log and execution continues. ASan still writes its full report to
  stderr, which still lands in the Application Output pane. **This is the better choice
  for quit testing**, because the LoudMax assertions (see the caveats below) otherwise
  halt you several times per session.
- **Debug** — `abort_on_error=1` raises `SIGABRT`, so lldb stops right at the ASan report
  with the live stack inspectable. Better when you actually have a finding to dig into.

### Turn OFF QML debugging (do this, or every quit test ends in a fake hang)

An ASan configuration cloned from Debug inherits QtCreator's QML debugging, which makes
**the app hang at quit, every time**, with no relation to anything we are testing.
SquareDesk has no QML in it, so there is nothing to lose by disabling it.

Uncheck it in **both** places — one alone is not enough:

1. **Projects → Run** → Debugger settings → uncheck **"Enable QML debugging and profiling"**
   (this is what adds the `-qmljsdebugger=port:...` command-line argument)
2. **Projects → Build** → the qmake step → uncheck the same option
   (this is what adds `CONFIG+=qml_debug`), then rebuild

If you skip this, the hang looks exactly like a teardown deadlock, and the main-thread
stack is:

```
main.cpp → QApplication::~QApplication → qt_call_post_routines()
  → QQmlDebugServerImpl::cleanup()          <- qqmldebugserverfactory.cpp
  → QCocoaEventDispatcher::processEvents
```

Qt's QML debug server spinning an event loop waiting for a QtCreator client that is never
going to disconnect. If you see `QQmlDebugServerImpl::cleanup()` in the stack, stop
debugging it — it is this, and nothing else.

### Setting it from the command line

```sh
export ASAN_OPTIONS=abort_on_error=1:malloc_context_size=50:detect_leaks=0
./test123/SquareDesk.app/Contents/MacOS/SquareDesk
```

### What the options are for

- `malloc_context_size=50` — the widget teardown stacks are deep and recursive; the
  default of 30 frames can truncate exactly the part we need.
- `detect_leaks=0` — LeakSanitizer isn't supported on macOS anyway; this just silences it.
- `abort_on_error=1` — stop at the first real error rather than continuing on corrupt state.

**To reproduce the bug: start the app, let the song table populate, use it normally for a
bit, then quit.** The crash is at quit, so the report appears as the app shuts down.

ASan writes to **stderr**, which lands in QtCreator's **Application Output** pane. Because
the report only appears as the app is quitting, don't close that pane or start another run
before copying it out. The report is long — scroll up for the `freed by thread T0 here:`
block, which is the part that actually matters.

## Reading the output

For this bug, look for `AddressSanitizer: heap-use-after-free` or
`attempting double-free`, then find these three blocks:

```
    #0 ... operator delete / free          <- the crash we already know about
    ...
freed by thread T0 here:                   <- THIS is the one we don't know
    #0 ...
    #1 ...
previously allocated by thread T0 here:    <- confirms which widget it is
    #0 ...
```

The **"freed by thread T0 here"** stack is the answer.

Save the whole report; it is worth pasting into #1686 verbatim.

### When there is no report: `memory history`

If the crash is a bare `EXC_BAD_ACCESS` with no ASan report — which is what you get when the
faulting access is inside an uninstrumented system library (see the caveat below) — ASan has
still recorded the allocation history. Ask it directly, from the lldb console (in QtCreator,
the input field at the bottom left of the **Debugger Log** window):

```
frame select 0
register read x0            # or whichever register holds the suspect pointer
memory history 0x<addr>
```

`memory history` prints the malloc **and** free stacks for that address, with thread names —
the same information as the report's "previously allocated by" / "freed by" blocks. This is
the single most useful command in this document: in #1693 it is what proved macOS itself was
freeing the object, on a HAL dispatch worker thread, from
`HALPropertyListener::~HALPropertyListener()`.

`expr (void)__asan_describe_address((void*)0x<addr>)` gives similar information, but writes
to stderr, so it only helps if the Application Output pane is actually receiving output.

### What it found the first time it was run (2026-08-06)

Worth recording, because the answer was nothing like what we had guessed from the crash
reports. We had suspected a double-free of song-table cell widgets, from Qt's index-widget
machinery releasing them with `deleteLater()`. That was wrong. The actual bug:

- `~MainWindow` frees the whole `Ui::MainWindow` struct at `mainwindow.cpp` (`delete ui`)
- ...then keeps going, and `delete sessionActionGroup` runs `~QActionGroup`, which unparents
  each of its actions
- ...each unparenting sends a `ChildRemoved` event to MainWindow
- ...MainWindow has installed itself as its own event filter (`mainwindow_JUCE.cpp:423`)
- ...so `MainWindow::eventFilter()` runs and evaluates `watched == ui->darkEndLoopButton`
- ...reading `ui`, which was freed several statements ago. **heap-use-after-free.**

Fixed by removing the event filter and setting `ui = nullptr` immediately after `delete ui`,
plus a `ui == nullptr` guard at the top of `MainWindow::eventFilter()`.

The lesson worth keeping: the crash reports pointed at `~darkSongTitleLabel` and
`~auditionButton` because those were where the *corrupted* memory got touched, not where the
bug was. Only the "freed by" stack identified the real culprit.

## Caveats and gotchas

- **It is slow.** Roughly 2x slower and several times more memory. Startup scanning of a
  large music directory will feel sluggish. This is normal, not a new bug.
- **Uninstrumented libraries still work — but only for the allocator.** ASan replaces
  `malloc`/`free` for the whole process, so a double-free is caught even in Qt or JUCE code
  that wasn't rebuilt, and every allocation gets its history recorded. Only the *symbol names
  and line numbers* need instrumentation.

  **What is *not* caught is a read or write of freed memory inside an uninstrumented
  library.** The instrumentation that checks each memory access only exists in code compiled
  with `-fsanitize=address`, so a use-after-free whose faulting access happens inside
  libobjc, CoreAudio, AppKit or a stock system dylib produces **no ASan report at all** — just
  a plain `EXC_BAD_ACCESS`.

  This matters more than it sounds: **the absence of an ASan report is not evidence that
  nothing was freed.** In #1693 the fatal read was `objc_retain` loading the isa of a freed
  ObjC block. No report, and every `this` pointer in the backtrace looked perfectly healthy,
  which sent the investigation down two wrong theories before `memory history` settled it.
- **QtWebEngine may complain.** The Chromium helper process is not instrumented and its
  sandbox does not always get along with ASan. If the Taminations or Reference tabs
  misbehave *in the ASan build only*, try:
  ```sh
  export QTWEBENGINE_CHROMIUM_FLAGS="--disable-features=SkiaGraphite --no-sandbox"
  ```
  (keep the SkiaGraphite flag — `main.cpp` sets it for issue #1600). Web-engine noise in
  an ASan build is not evidence of a real bug.
- **LoudMax assertions will stop you, but only under the debugger.** Expect roughly two on
  Play and two on Stop:
  `JUCE Assertion failure in juce_VST3PluginFormat.cpp:2230`.
  Under lldb each one halts the whole process, and a halted process does not service its
  run loop, which macOS paints as a **beachball** — it looks exactly like a hang. Under
  plain **Run** they only log, which is why Run is the better choice for quit testing.

  **This is a bug in LoudMax, not in SquareDesk, and it is harmless.** The assertion is in
  `HostToClientParamQueue::addPoint` — the *input* (host → plugin) parameter queue. The
  VST3 spec does not allow a plugin to add points to an incoming queue, so JUCE returns
  `kResultFalse` and asserts to name the offender. The call is rejected safely; nothing is
  corrupted. It is also invisible in shipped builds: assertions are gated on
  `JUCE_DEBUG && !JUCE_DISABLE_ASSERTIONS`, so in Release `jassertfalse` degrades to a
  log-only statement and compiles away entirely with `JUCE_LOG_ASSERTIONS` off.

  There is no fix on our side — we don't control the plugin, and silencing it would mean
  `JUCE_DISABLE_ASSERTIONS`, which would blind us to every real JUCE assertion too. It is
  **not** related to #1266, which is a threading/lifetime race.

  To take LoudMax out of a test run entirely, move the plugin aside:
  ```sh
  mv ~/Library/Audio/Plug-Ins/VST3/LoudMax.vst3 ~/LoudMax.vst3.disabled
  # ...run the test, then put it back...
  mv ~/LoudMax.vst3.disabled ~/Library/Audio/Plug-Ins/VST3/LoudMax.vst3
  ```
  Both `processBlock()` call sites (`audiodecoder.cpp:299` and `:752`) are guarded on
  `pLoudMaxPluginRaw != nullptr`, and the load path bails out cleanly at
  `mainwindow_JUCE.cpp:319`, so a missing plugin is handled — the app just logs
  `Play: pLoudMaxPluginRaw was null`. Note the FX button is a *launcher*, not a toggle;
  it cannot be unchecked to disable the plugin.
- **Don't ship an ASan build.** It is for diagnosis only — no notarization, no DMG.
- **Ignore leak-shaped complaints.** We are hunting a use-after-free. SquareDesk
  intentionally leaves some objects alive until exit.
