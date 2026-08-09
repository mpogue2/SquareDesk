# Agent Guidelines for SquareDesk

This document provides guidance for AI agents working on the SquareDesk project.

## Managing AI-Generated Planning Documents

AI assistants often create planning and design documents during development:
- PLAN.md, IMPLEMENTATION.md, ARCHITECTURE.md
- DESIGN.md, CODEBASE_SUMMARY.md, INTEGRATION_PLAN.md
- TESTING_GUIDE.md, TECHNICAL_DESIGN.md, and similar files

**Best Practice: Use a dedicated directory for these ephemeral files**

**Recommended approach:**
- Create a `history/` directory in the project root
- Store ALL AI-generated planning/design docs in `history/`
- Keep the repository root clean and focused on permanent project files
- Only access `history/` when explicitly asked to review past planning

**Example .gitignore entry (optional):**
```
# AI planning documents (ephemeral)
history/
```

**Benefits:**
- ✅ Clean repository root
- ✅ Clear separation between ephemeral and permanent documentation
- ✅ Easy to exclude from version control if desired
- ✅ Preserves planning history for archeological research
- ✅ Reduces noise when browsing the project

## Debugging crashes and hangs: the AddressSanitizer build

**Before spending long on any crash or hang — especially at quit time — read
[ASAN.md](ASAN.md).**

SquareDesk has an AddressSanitizer build, off by default and enabled by giving qmake
`CONFIG+=asan`. Normal Debug and Release builds are completely unaffected; everything lives
in an `asan { ... }` block at the bottom of `test123/test123.pro`.

Why it is worth reaching for early: in issues #1686 and #1266, the macOS crash reports each
named a destructor that turned out to be entirely innocent — the memory was corrupted
somewhere else, and the reported frame was just where it got touched. Reading those reports
produced a detailed and completely wrong theory. ASan named the real cause in one run, both
times, by printing the stack that freed the memory.

`ASAN.md` covers building it, the `ASAN_OPTIONS` worth setting, how to read the report, and
— importantly — **two things that look exactly like a hang but are not**: JUCE assertions
halting the process under lldb, and QtCreator's QML debug server blocking at shutdown. Both
cost real time to diagnose before they were written down.

Note that the QtCreator build/run configuration itself cannot be committed (it lives in
`.pro.user`, which is gitignored and full of machine-specific paths), so the few GUI steps
in `ASAN.md` have to be repeated on each developer's machine.
