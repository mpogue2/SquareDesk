# =========================================================================================
# ADDRESS SANITIZER BUILD (Issue #1686)
#
#   Off by default. Nothing below affects a normal Debug or Release build -- it is all
#   inside the asan{} block, which is only entered when qmake is given CONFIG+=asan.
#
#   In QtCreator:
#     Projects -> Build & Run -> (your Qt kit) -> Build -> clone the Debug configuration,
#     name the clone "ASan", give it its own build directory (e.g.
#     build-SquareDesk-Qt_6_10_3_for_macOS-ASan), and add
#         CONFIG+=asan
#     to "Additional arguments" on the qmake build step. Build it, then just Run.
#
#   From the command line:
#     mkdir build-asan && cd build-asan
#     <QtDir>/macos/bin/qmake ../SquareDesk-DEV/SquareDesk.pro CONFIG+=asan CONFIG+=debug
#     make -j8
#
#   ASan finds the bug at the moment it happens (the second free), and prints BOTH the
#   stack that freed it the first time and the stack that allocated it -- which is exactly
#   what the quit-time crash reports cannot tell us. Run it, use the app normally, then
#   quit: if the double-free is real, ASan reports it instead of crashing in _free.
#
#   Recommended environment when running (full instructions are in ../ASAN.md):
#     ASAN_OPTIONS=abort_on_error=1:malloc_context_size=50:detect_leaks=0
#
asan {
    message("*** ADDRESS SANITIZER BUILD ENABLED (CONFIG+=asan) ***")

    # -O1 and -fno-omit-frame-pointer keep the stack traces readable; -g gives us line
    #   numbers. These come after qmake's own flags, so they win.
    ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1 -g

    QMAKE_CFLAGS   += $$ASAN_FLAGS
    QMAKE_CXXFLAGS += $$ASAN_FLAGS
    QMAKE_LFLAGS   += -fsanitize=address

    # so code can tell (e.g. to skip a known-noisy path) that it is an ASan build
    DEFINES += SQUAREDESK_ASAN
}
