#!/bin/bash
#
# update_sd.sh -- update SquareDesk's internal copy of sd.
#
# Downloads the latest sd source from challengedance.org, copies the files we
# carry into sdlib/, re-applies the SquareDesk-specific hand patches, rebuilds
# sd_calls.dat, fetches the latest sd_doc.pdf, and bumps the version constants.
#
# The recipe implemented here comes from:
#   https://github.com/mpogue2/SquareDesk/issues/1713  (source integration, patches, mainstream26/plus26)
#   https://github.com/mpogue2/SquareDesk/issues/1110  (rebuilding sd_calls.dat)
#
# Only the latest sd source is published upstream, so there is no version
# argument -- you always get whatever challengedance.org is currently serving.
#
# What this touches:
#   sdlib/*.cpp, *.h, sd_calls.txt   (from the upstream zip; sdlib.pro is ours)
#   sdlib/sd_doc.pdf                 (test123.pro copies it to Resources/)
#   test123/sd_calls.dat             (test123.pro copies it to Resources/)
#   test123/globaldefines.h          (SD_VERSION)
#   test123/mainwindow.cpp           (reference/<N>.SD_V<version>.pdf number)
#
# What it does NOT touch: the Mainstream -> mainstream26 / Plus -> plus26
# mapping in test123/mainwindow_sd.cpp.  That is a one-time source change, not
# a per-update one; this script only checks that it is still in place.

set -euo pipefail

SOURCE_URL="https://challengedance.org/sd/sd_source.zip"
DOC_URL="https://challengedance.org/sd/sd_doc.pdf"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDLIB="$SCRIPT_DIR"
DEVROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST123="$DEVROOT/test123"

ZIP_ARG=""
PDF_ARG=""
DO_PDF=1
DO_DAT=1
KEEP_TEMP=0

TMPDIR_WORK=""

# ---------------------------------------------------------------- output ----

if [ -t 1 ]; then
    B=$'\033[1m'; RED=$'\033[31m'; GRN=$'\033[32m'; YEL=$'\033[33m'; N=$'\033[0m'
else
    B=""; RED=""; GRN=""; YEL=""; N=""
fi

step() { printf '\n%s==> %s%s\n' "$B" "$*" "$N"; }
info() { printf '    %s\n' "$*"; }
warn() { printf '%s[warn]%s %s\n' "$YEL" "$N" "$*" >&2; }
die()  { printf '%s[error]%s %s\n' "$RED" "$N" "$*" >&2; exit 1; }
ok()   { printf '%s[ok]%s %s\n' "$GRN" "$N" "$*"; }

# Ask a yes/no question. Default is No unless $2 is "yes".
confirm() {
    local prompt="$1" default="${2:-no}" reply="" hint=""
    if [ "$default" = "yes" ]; then hint="[Y/n]"; else hint="[y/N]"; fi
    read -r -p "$prompt $hint " reply </dev/tty || reply=""
    reply="$(printf '%s' "$reply" | tr '[:upper:]' '[:lower:]')"
    if [ -z "$reply" ]; then reply="$default"; fi
    case "$reply" in y|yes) return 0 ;; *) return 1 ;; esac
}

cleanup() {
    if [ -n "$TMPDIR_WORK" ] && [ -d "$TMPDIR_WORK" ]; then
        if [ "$KEEP_TEMP" -eq 1 ]; then
            printf '\n    Scratch directory kept at: %s\n' "$TMPDIR_WORK"
        else
            rm -rf "$TMPDIR_WORK"
        fi
    fi
}
trap cleanup EXIT INT TERM

usage() {
    cat <<'USAGE'
Usage: update_sd.sh [options]

Updates SquareDesk's bundled copy of sd to the latest version published at
https://challengedance.org/sd/ .  Upstream only ships the newest source, so
there is nothing to choose.

Options:
  --zip PATH    Use an already-downloaded sd_source.zip instead of fetching it
  --pdf PATH    Use an already-downloaded sd_doc.pdf instead of fetching it
  --no-pdf      Leave sdlib/sd_doc.pdf alone
  --no-dat      Do not rebuild test123/sd_calls.dat
  --keep        Keep the scratch directory on exit
  -h, --help    This message
USAGE
}

while [ $# -gt 0 ]; do
    case "$1" in
        --zip)     ZIP_ARG="${2:-}"; [ -n "$ZIP_ARG" ] || die "--zip needs a value"; shift 2 ;;
        --pdf)     PDF_ARG="${2:-}"; [ -n "$PDF_ARG" ] || die "--pdf needs a value"; shift 2 ;;
        --no-pdf)  DO_PDF=0; shift ;;
        --no-dat)  DO_DAT=0; shift ;;
        --keep)    KEEP_TEMP=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *)         die "Unknown option: $1  (try --help)" ;;
    esac
done

# ------------------------------------------------------------- preflight ----

step "Preflight"

for tool in curl unzip make g++ python3 git od; do
    command -v "$tool" >/dev/null 2>&1 || die "'$tool' not found in PATH."
done

[ -f "$SDLIB/sdmain.cpp" ]           || die "Not found: $SDLIB/sdmain.cpp"
[ -f "$SDLIB/sdlib.pro" ]            || die "Not found: $SDLIB/sdlib.pro"
[ -f "$TEST123/globaldefines.h" ]    || die "Not found: $TEST123/globaldefines.h"
[ -f "$TEST123/mainwindow.cpp" ]     || die "Not found: $TEST123/mainwindow.cpp"

REPO_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel 2>/dev/null)" \
    || die "$SCRIPT_DIR is not inside a git working tree."

read_version_string() {
    # VERSION_STRING out of an sdmain.cpp, e.g. 39.71a
    sed -n 's/^#define VERSION_STRING "\([^"]*\)".*/\1/p' "$1" | head -1
}

OLD_VERSION="$(read_version_string "$SDLIB/sdmain.cpp")"
[ -n "$OLD_VERSION" ] || die "Could not read VERSION_STRING from $SDLIB/sdmain.cpp"
info "Currently bundled sd: $OLD_VERSION"

# A dirty tree makes it impossible to tell our changes from yours if a patch
# fails halfway through. Warn, don't block -- you may have deliberate edits.
DIRTY="$(git -C "$REPO_ROOT" status --porcelain -- "$SDLIB" "$TEST123/sd_calls.dat" \
             "$TEST123/globaldefines.h" "$TEST123/mainwindow.cpp" 2>/dev/null || true)"
if [ -n "$DIRTY" ]; then
    warn "These files already have uncommitted changes:"
    printf '%s\n' "$DIRTY" | sed 's/^/        /' >&2
    confirm "Continue anyway?" no || die "Aborted at your request."
fi

TMPDIR_WORK="$(mktemp -d "${TMPDIR:-/tmp}/sd-update.XXXXXX")"
SRC="$TMPDIR_WORK/sdsrc"
mkdir -p "$SRC"

# --------------------------------------------------------------- fetch ------

step "Fetching the sd source"

ZIP="$TMPDIR_WORK/sd_source.zip"
if [ -n "$ZIP_ARG" ]; then
    [ -f "$ZIP_ARG" ] || die "No such file: $ZIP_ARG"
    info "Using local zip: $ZIP_ARG"
    cp "$ZIP_ARG" "$ZIP"
else
    info "$SOURCE_URL"
    curl -fsSL -o "$ZIP" "$SOURCE_URL" || die "Download failed: $SOURCE_URL"
fi

unzip -oq "$ZIP" -d "$SRC" || die "Could not unpack $ZIP"

# Upstream ships a flat archive, but be tolerant of a single wrapper directory.
if [ ! -f "$SRC/sdmain.cpp" ]; then
    INNER="$(find "$SRC" -mindepth 2 -maxdepth 2 -name sdmain.cpp -print -quit || true)"
    [ -n "$INNER" ] || die "The archive does not contain sdmain.cpp -- is this really sd_source.zip?"
    SRC="$(dirname "$INNER")"
    info "Source is nested; using $SRC"
fi

NEW_VERSION="$(read_version_string "$SRC/sdmain.cpp")"
[ -n "$NEW_VERSION" ] || die "Could not read VERSION_STRING from the downloaded sdmain.cpp"

# The doc filename regex in mainwindow.cpp only accepts digits and dots, so
# strip any trailing letter from e.g. "39.71a".
NEW_VERSION_NUM="$(printf '%s' "$NEW_VERSION" | tr -cd '0-9.')"

printf '\n'
info "Bundled version:  $OLD_VERSION"
info "Upstream version: $NEW_VERSION"
printf '\n'

if [ "$OLD_VERSION" = "$NEW_VERSION" ]; then
    warn "That is the same version that is already bundled."
fi

confirm "Update SquareDesk's sd to $NEW_VERSION?" yes || die "Aborted at your request."

# ----------------------------------------------------------- file check -----

step "Checking the file set"

# Everything we carry in sdlib/ must exist upstream, except these two, which
# are ours: sdlib.pro is the qmake project, sd_doc.pdf is fetched separately.
OURS_ONLY="sdlib.pro sd_doc.pdf update_sd.sh"

MISSING=""
TAKE=""
for path in "$SDLIB"/*; do
    f="$(basename "$path")"
    [ -f "$path" ] || continue
    case " $OURS_ONLY " in *" $f "*) continue ;; esac
    if [ -f "$SRC/$f" ]; then
        TAKE="$TAKE $f"
    else
        MISSING="$MISSING $f"
    fi
done

if [ -n "$MISSING" ]; then
    die "These files are in sdlib/ but not in the upstream archive:$MISSING

Upstream has renamed, removed, or split them. Work out the new file set by
hand, update sdlib.pro to match, then re-run. Nothing has been changed."
fi

NUM_TAKE="$(printf '%s' "$TAKE" | wc -w | tr -d ' ')"
info "$NUM_TAKE files will be taken from upstream"

# Did upstream add a source file that belongs in our build? We only compile a
# subset of sd (no UI, no printing, no mkcalls), so most new files are not ours
# -- but a *split* of a file we do carry would silently drop code.
NEWFILES=""
for path in "$SRC"/*.cpp "$SRC"/*.h; do
    [ -f "$path" ] || continue
    f="$(basename "$path")"
    [ -f "$SDLIB/$f" ] && continue
    grep -q "[^a-zA-Z0-9_]$f\([^a-zA-Z0-9_]\|$\)" "$SDLIB/sdlib.pro" && continue
    NEWFILES="$NEWFILES $f"
done
if [ -n "$NEWFILES" ]; then
    info "Upstream files we do not carry (normally fine -- UI, printing, mkcalls):"
    printf '       %s\n' "$NEWFILES"
    info "If any of those is a NEW split-out of a file we do carry, add it to sdlib.pro."
fi

# ---------------------------------------------------------------- copy ------

step "Copying upstream sources into sdlib/"

for f in $TAKE; do
    cp "$SRC/$f" "$SDLIB/$f"
done
ok "Copied $NUM_TAKE files"

# --------------------------------------------------------------- patch ------

step "Re-applying the SquareDesk patches (issue #1713)"

PATCH_OUT="$TMPDIR_WORK/patch_results.txt"

python3 - "$SDLIB" >"$PATCH_OUT" <<'PY'
import sys

sdlib = sys.argv[1]

# The upstream sources are CRLF. Read and write them with newline="" so Python
# leaves the line endings alone, and fit the patch literals below (which are
# written LF) to whatever the file actually uses. Rewriting a CRLF file as LF
# would turn a three-line patch into a whole-file diff, and would break the
# "differs from upstream only in the patched region" check on the next run.
_eol = {}

def read(name):
    with open("%s/%s" % (sdlib, name), encoding="utf-8", newline="") as f:
        raw = f.read()
    crlf = raw.count("\r\n")
    _eol[name] = "\r\n" if crlf and crlf * 2 >= raw.count("\n") else "\n"
    return raw

def write(name, text):
    with open("%s/%s" % (sdlib, name), "w", encoding="utf-8", newline="") as f:
        f.write(text)

def fit(name, text):
    """Convert an LF patch literal to the line ending this file uses."""
    return text.replace("\n", _eol[name]) if _eol[name] == "\r\n" else text

results = []
def say(status, name, msg):
    results.append("%s\t%s\t%s" % (status, name, msg))

# ---------------------------------------------------------------------------
# mapcachefile.cpp -- sd builds the map cache filename from the source file
# names, which for us is a long absolute path inside the .app bundle, in a
# directory we cannot write to. Hard-code the cache to /tmp instead.
#
# Our copy of this file differs from upstream ONLY in this region, so it is a
# straight region substitution. Verify the upstream text is exactly what we
# expect before touching it: if upstream reworked this code we must not guess.
# ---------------------------------------------------------------------------

MAPCACHE_UPSTREAM = """   int i, j;

   for (i=0 ; i<numsourcefiles ; i++)
      mapfilenamesize += strlen(srcnames[i]);

   innards->mapfilename = new char [mapfilenamesize];

   int filenamepos = 0;

   // Append the source file base names.
   for (i=0 ; i<numsourcefiles ; i++) {
      const char *this_src = srcnames[i];
      for (j=strlen(this_src)-1 ; ; j--) {
         if (j <= 0 || this_src[j] == '.') {
            if (j <= 0) j = strlen(this_src);
            ::memcpy(innards->mapfilename+filenamepos, this_src, j);
            filenamepos += j;
            if (i != numsourcefiles-1)
               innards->mapfilename[filenamepos++] = '+';
            break;
         }
      }
   }

   innards->mapfilename[filenamepos++] = '.';
   ::strcpy(innards->mapfilename+filenamepos, mapext);"""

MAPCACHE_PATCHED = """   int i; //, j;  // -mpogue, 2025.05.16

   for (i=0 ; i<numsourcefiles ; i++)
      mapfilenamesize += strlen(srcnames[i]);

   innards->mapfilename = new char [mapfilenamesize];

   // -mpogue, always put the cache file (only one for us) in /tmp, 2025.05.16
   // int filenamepos = 0;
   //
   // // Append the source file base names.
   // for (i=0 ; i<numsourcefiles ; i++) {
   //    const char *this_src = srcnames[i];
   //    for (j=strlen(this_src)-1 ; ; j--) {
   //       if (j <= 0 || this_src[j] == '.') {
   //          if (j <= 0) j = strlen(this_src);
   //          ::memcpy(innards->mapfilename+filenamepos, this_src, j);
   //          filenamepos += j;
   //          if (i != numsourcefiles-1)
   //             innards->mapfilename[filenamepos++] = '+';
   //          break;
   //       }
   //    }
   // }

   // innards->mapfilename[filenamepos++] = '.';
   // ::strcpy(innards->mapfilename+filenamepos, mapext);

   // here numsourcefiles should be 1, and it's always the sd_calls.dat file
   //   that needs to be cached.  So, let's just hard-code the name.
   char tempDir[] = "/tmp/sd_calls.";
   ::strcpy(innards->mapfilename, tempDir);
   ::strcat(innards->mapfilename, mapext);

   // printf("SD mapfilename: %s\\n", innards->mapfilename); // mpogue
   // -mpogue, END MODIFICATIONS, 2025.05.16"""

src = read("mapcachefile.cpp")
want = fit("mapcachefile.cpp", MAPCACHE_UPSTREAM)
have = fit("mapcachefile.cpp", MAPCACHE_PATCHED)
if have in src:
    say("SKIP", "mapcachefile.cpp", "already patched")
elif src.count(want) == 1:
    write("mapcachefile.cpp", src.replace(want, have))
    say("OK", "mapcachefile.cpp", "map cache redirected to /tmp/sd_calls.<ext>")
else:
    say("FAIL", "mapcachefile.cpp",
        "the upstream mapfilename-building block is not the text we expect. "
        "Upstream has reworked how the cache filename is built; redo this patch by hand.")

# ---------------------------------------------------------------------------
# sdinit.cpp -- outfile_string accumulates level suffixes across restarts,
# giving "sequence.Plus.A1.C1". Re-initialize it every time through.
# ---------------------------------------------------------------------------

SDINIT_ANCHOR = """   if (calling_level != l_nonexistent_concept)
      strncat(outfile_string, filename_strings[calling_level], MAX_FILENAME_LENGTH-80);"""

SDINIT_ADDITION = ("   snprintf(outfile_string, 10, SEQUENCE_FILENAME); "
                   "// -mpogue, 2025/01/21: initialize the string EVERY time "
                   "to avoid sequence.Plus.A1.C1 bug\n")

src = read("sdinit.cpp")
anchor = fit("sdinit.cpp", SDINIT_ANCHOR)
addition = fit("sdinit.cpp", SDINIT_ADDITION)
if "snprintf(outfile_string, 10, SEQUENCE_FILENAME)" in src:
    say("SKIP", "sdinit.cpp", "already patched")
elif src.count(anchor) == 1:
    write("sdinit.cpp", src.replace(anchor, addition + anchor))
    say("OK", "sdinit.cpp", "outfile_string re-initialized on every startup")
else:
    say("FAIL", "sdinit.cpp",
        "could not find the single 'strncat(outfile_string, filename_strings[calling_level], ...)' "
        "anchor. Re-derive this patch by hand or sequence filenames will accumulate level suffixes.")

# ---------------------------------------------------------------------------
# sdmatch.cpp -- matching a direction suffix walks direction_names[] one entry
# too far and crashes.
# ---------------------------------------------------------------------------

SDMATCH_UPSTREAM = "            for (i=1; i<=last_direction_kind; ++i) {"
SDMATCH_PATCHED = (
    "            // for (i=1; i<=last_direction_kind; ++i) {\n"
    "            for (i=1; i<=last_direction_kind-1; ++i) { "
    "// I'm not sure about this (eliminates crash), -mpogue: 2025/01/21")

src = read("sdmatch.cpp")
want = fit("sdmatch.cpp", SDMATCH_UPSTREAM)
have = fit("sdmatch.cpp", SDMATCH_PATCHED)
if "last_direction_kind-1; ++i" in src:
    say("SKIP", "sdmatch.cpp", "already patched")
elif src.count(want) == 1:
    write("sdmatch.cpp", src.replace(want, have))
    say("OK", "sdmatch.cpp", "direction_names[] overrun fixed")
else:
    say("FAIL", "sdmatch.cpp",
        "could not find the single 'for (i=1; i<=last_direction_kind; ++i) {' anchor. "
        "Re-derive this patch by hand or sd will crash on direction suffixes.")

# ---------------------------------------------------------------------------
# sd.h -- id_bit_table entries do not all fit in a signed int32.
# Upstream fixed this in 39.84; we only need to act if it ever regresses.
# ---------------------------------------------------------------------------

src = read("sd.h")
if "typedef uint32_t id_bit_table[4];" in src:
    say("INFO", "sd.h", "id_bit_table is already uint32_t upstream -- our patch is obsolete")
elif src.count("typedef int id_bit_table[4];") == 1:
    write("sd.h", src.replace(
        "typedef int id_bit_table[4];",
        "typedef uint32_t id_bit_table[4]; // -mpogue, 2025/01/21: some values don't "
        "fit in a signed int32, but all are OK in unsigned int32"))
    say("OK", "sd.h", "id_bit_table widened to uint32_t")
else:
    say("FAIL", "sd.h",
        "id_bit_table is neither 'int' nor 'uint32_t'. Check by hand that its entries "
        "still fit the declared type.")

print("\n".join(results))
PY

FAILED=0
while IFS=$'\t' read -r status name msg; do
    case "$status" in
        OK)   ok   "$name: $msg" ;;
        SKIP) info "$name: $msg" ;;
        INFO) info "$name: $msg" ;;
        FAIL) printf '%s[FAIL]%s %s: %s\n' "$RED" "$N" "$name" "$msg" >&2; FAILED=1 ;;
    esac
done < "$PATCH_OUT"

if [ "$FAILED" -eq 1 ]; then
    die "At least one patch could not be applied.

sdlib/ now holds the new upstream sources but is NOT fully patched, so it will
not build correctly. Either fix the failing patch by hand (see issue #1713 for
what each one is for), or throw the update away with:

    git -C $REPO_ROOT checkout -- $SDLIB"
fi

# ------------------------------------------------------------ sd_calls -----

if [ "$DO_DAT" -eq 1 ]; then
    step "Rebuilding sd_calls.dat (issue #1110)"

    # mkcalls only needs mkcalls.cpp, common.cpp and the headers -- none of the
    # files we patch -- so build it from the pristine unpacked source.
    ( cd "$SRC" && make -f makefile.mac mkcalls >/dev/null 2>&1 ) \
        || die "Could not build mkcalls. Try 'make -f makefile.mac mkcalls' in $SRC by hand."
    ( cd "$SRC" && ./mkcalls ./sd_calls.txt ) \
        || die "mkcalls failed on sd_calls.txt."

    [ -s "$SRC/sd_calls.dat" ] || die "mkcalls produced no sd_calls.dat."

    # The version string is stored near the top of the file. Confirm the data
    # file we are about to ship really is the version we think it is.
    DAT_HEAD="$(od -A n -c -N 32 "$SRC/sd_calls.dat" | tr -d ' \n')"
    case "$DAT_HEAD" in
        *"$NEW_VERSION_NUM"*) info "sd_calls.dat reports version $NEW_VERSION_NUM" ;;
        *) warn "Could not find '$NEW_VERSION_NUM' in the sd_calls.dat header."
           warn "Check it by hand with: od -c $SRC/sd_calls.dat | head"
           confirm "Install it anyway?" no || die "Aborted at your request." ;;
    esac

    cp "$SRC/sd_calls.dat" "$TEST123/sd_calls.dat"
    ok "test123/sd_calls.dat rebuilt ($(du -h "$TEST123/sd_calls.dat" | cut -f1))"
else
    info "Skipping the sd_calls.dat rebuild (--no-dat)."
fi

# ----------------------------------------------------------------- doc ------

if [ "$DO_PDF" -eq 1 ]; then
    step "Fetching sd_doc.pdf"

    NEWPDF="$TMPDIR_WORK/sd_doc.pdf"
    if [ -n "$PDF_ARG" ]; then
        [ -f "$PDF_ARG" ] || die "No such file: $PDF_ARG"
        info "Using local PDF: $PDF_ARG"
        cp "$PDF_ARG" "$NEWPDF"
    else
        info "$DOC_URL"
        curl -fsSL -o "$NEWPDF" "$DOC_URL" || die "Download failed: $DOC_URL"
    fi

    [ "$(head -c 4 "$NEWPDF")" = "%PDF" ] || die "$NEWPDF is not a PDF."

    # The title page carries "for Sd version <n>". Check it matches the source
    # we just installed -- upstream has been known to update one before the other.
    if command -v pdftotext >/dev/null 2>&1; then
        DOCVER="$(pdftotext -f 1 -l 1 "$NEWPDF" - 2>/dev/null \
                   | sed -n 's/.*for Sd version \([0-9.]*\).*/\1/p' | head -1)"
        if [ -z "$DOCVER" ]; then
            warn "Could not read a version out of the PDF title page; continuing."
        elif [ "$DOCVER" != "$NEW_VERSION_NUM" ]; then
            warn "sd_doc.pdf says version $DOCVER, but the source is $NEW_VERSION_NUM."
            confirm "Install this PDF anyway?" no || die "Aborted at your request."
        else
            info "sd_doc.pdf is for Sd version $DOCVER"
        fi
    else
        info "pdftotext not installed -- skipping the PDF version check."
    fi

    cp "$NEWPDF" "$SDLIB/sd_doc.pdf"
    ok "sdlib/sd_doc.pdf updated ($(du -h "$SDLIB/sd_doc.pdf" | cut -f1))"
else
    info "Skipping the sd_doc.pdf update (--no-pdf)."
fi

# ---------------------------------------------------------- constants -------

step "Bumping the version constants"

CONST_OUT="$TMPDIR_WORK/const_results.txt"

python3 - "$TEST123" "$NEW_VERSION_NUM" "$DO_PDF" >"$CONST_OUT" <<'PY'
import re, sys

test123, version, do_pdf = sys.argv[1], sys.argv[2], sys.argv[3] == "1"
results = []

def say(status, name, msg):
    results.append("%s\t%s\t%s" % (status, name, msg))

# --- SD_VERSION in globaldefines.h ---------------------------------------
path = "%s/globaldefines.h" % test123
src = open(path, encoding="utf-8").read()
m = re.search(r'^(#define\s+SD_VERSION\s+")([^"]*)(")', src, re.M)
version_changed = False
if not m:
    say("FAIL", "globaldefines.h", "could not find the SD_VERSION #define")
elif m.group(2) == version:
    say("SKIP", "globaldefines.h", "SD_VERSION is already %s" % version)
else:
    version_changed = True
    old = m.group(2)
    src = src[:m.start()] + m.group(1) + version + m.group(3) + src[m.end():]
    open(path, "w", encoding="utf-8").write(src)
    say("OK", "globaldefines.h", "SD_VERSION %s -> %s" % (old, version))

# --- reference/<N>.SD_V<version>.pdf number in mainwindow.cpp ------------
# Each sd update installs a NEW numbered copy of the doc alongside the old
# one, so the number goes up by one every time the version changes.
#
# Only bump when SD_VERSION actually changed. Re-running the script for a
# version that is already installed must not keep advancing the number --
# that would strand the doc under a number nothing ever writes.
path = "%s/mainwindow.cpp" % test123
src = open(path, encoding="utf-8").read()
m = re.search(r'(QString\s+latestSDFilename\s*=\s*")(\d+)(\.SD_V")', src)
if not m:
    say("FAIL", "mainwindow.cpp",
        'could not find the \'QString latestSDFilename = "<N>.SD_V"\' line')
elif not do_pdf:
    say("SKIP", "mainwindow.cpp", "left at %s (--no-pdf)" % m.group(2))
elif not version_changed:
    say("SKIP", "mainwindow.cpp",
        "left at %s -- SD_VERSION was already %s" % (m.group(2), version))
else:
    old = int(m.group(2))
    new = old + 1
    src = src[:m.start()] + m.group(1) + str(new) + m.group(3) + src[m.end():]
    open(path, "w", encoding="utf-8").write(src)
    say("OK", "mainwindow.cpp",
        "reference doc will install as %d.SD_V%s.pdf (was numbered %d)" % (new, version, old))

print("\n".join(results))
PY

FAILED=0
while IFS=$'\t' read -r status name msg; do
    case "$status" in
        OK)   ok   "$name: $msg" ;;
        SKIP) info "$name: $msg" ;;
        FAIL) printf '%s[FAIL]%s %s: %s\n' "$RED" "$N" "$name" "$msg" >&2; FAILED=1 ;;
    esac
done < "$CONST_OUT"

[ "$FAILED" -eq 0 ] || die "Could not bump a version constant. Fix it by hand before building."

# ------------------------------------------------- level mapping check ------

step "Checking the Mainstream26 / Plus26 mapping (issue #1713)"

# SquareDesk maps its Dance Program > Mainstream to sd's mainstream26 (l_xyz)
# and > Plus to plus26 (l_pqr). Those are source changes in test123/, not
# something this script owns -- but if upstream renames the enum values, the
# build breaks in a confusing way, so say so plainly here.
LEVEL_OK=1

for sym in l_xyz l_pqr; do
    if grep -q "^   $sym," "$SDLIB/database.h"; then
        info "database.h still defines $sym"
    else
        warn "database.h no longer defines $sym -- upstream renamed the 2026 dance levels."
        LEVEL_OK=0
    fi
done

for want in mainstream26 plus26; do
    if grep -rq "\"$want\"" "$TEST123/mainwindow_sd.cpp"; then
        info "mainwindow_sd.cpp still passes \"$want\" to sd"
    else
        warn "mainwindow_sd.cpp does not mention \"$want\"."
        LEVEL_OK=0
    fi
done

if [ "$LEVEL_OK" -eq 1 ]; then
    ok "Dance Program mapping looks intact."
else
    warn "Fix the mapping in test123/mainwindow_sd.cpp before shipping; see issue #1713."
    warn "Without it, Dance Program > Mainstream and > Plus select the OLD Callerlab lists."
fi

# ------------------------------------------------------- issue + commit -----

step "Recording the update"

printf '\n'
info "Changed:"
git -C "$REPO_ROOT" status --porcelain -- "$SDLIB" "$TEST123/sd_calls.dat" \
    "$TEST123/globaldefines.h" "$TEST123/mainwindow.cpp" | sed 's/^/        /'
printf '\n'

ISSUE=""
while [ -z "$ISSUE" ]; do
    read -r -p "    GitHub issue number for this sd update (digits only): " ISSUE </dev/tty || true
    ISSUE="$(printf '%s' "$ISSUE" | tr -d ' #')"
    if ! [[ "$ISSUE" =~ ^[0-9]+$ ]]; then
        warn "Please enter just the issue number, e.g. 1713"
        ISSUE=""
    fi
done

COMMIT_MSG="#$ISSUE: Update SD to $NEW_VERSION

Refreshed sdlib/ using sdlib/update_sd.sh, from
$SOURCE_URL
Re-applied the SquareDesk patches, rebuilt test123/sd_calls.dat, and updated
sd_doc.pdf. Previous bundled version: $OLD_VERSION.

Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>"

MSG_FILE="$SDLIB/.update_sd_commit_msg.txt"
printf '%s\n' "$COMMIT_MSG" > "$MSG_FILE"

cat <<EOT

$B    NEXT: build and test SquareDesk before committing.$N

      1. Build SquareDesk (qmake copies sd_calls.dat and sd_doc.pdf into
         SquareDesk.app/Contents/Resources).
      2. Open the SD tab and run a sequence at each Dance Program level.
      3. Check SD > Dance Program > Mainstream and > Plus really select the
         2026 Callerlab lists (e.g. Cloverleaf is NOT legal at Mainstream).
      4. Check Help > SD Help opens the new doc, and that a fresh music
         directory gets reference/<N>.SD_V$NEW_VERSION_NUM.pdf.

EOT

printf '    Nothing has been committed.\n'
printf '    After you have built and tested:\n\n'
printf '        git -C %s add %s %s %s %s\n' \
    "$REPO_ROOT" "$SDLIB" "$TEST123/sd_calls.dat" \
    "$TEST123/globaldefines.h" "$TEST123/mainwindow.cpp"
printf '        git -C %s commit -F %s\n' "$REPO_ROOT" "$MSG_FILE"
printf '        git -C %s push\n\n' "$REPO_ROOT"
printf '    To undo instead:\n\n'
printf '        git -C %s checkout -- %s %s %s %s\n\n' \
    "$REPO_ROOT" "$SDLIB" "$TEST123/sd_calls.dat" \
    "$TEST123/globaldefines.h" "$TEST123/mainwindow.cpp"

ok "sd updated: $OLD_VERSION  ->  $NEW_VERSION"
