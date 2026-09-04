#!/bin/bash
#
# update_taminations.sh -- rebuild the bundled copy of Taminations for SquareDesk.
#
# Clones bradchristie/taminations-flutter, applies the SquareDesk-specific
# patches, builds it for the web, lets you test it, and drops the resulting
# web.zip into SquareDesk-DEV/Taminations/web.zip.
#
# The recipe implemented here comes from:
#   https://github.com/mpogue2/SquareDesk/issues/1415  (packaging + ?Speed=Fast)
#   https://github.com/mpogue2/SquareDesk/issues/1467  (fully-offline build)
#   https://github.com/mpogue2/SquareDesk/issues/1456  (default sequencer speed)
#
# At SquareDesk build time, qmake unzips web.zip into
# SquareDesk.app/Contents/Resources/Taminations/ -- so the archive MUST have a
# top-level "web/" directory (see #1437).

set -euo pipefail

UPSTREAM_REPO="https://github.com/bradchristie/taminations-flutter.git"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WEBZIP="$SCRIPT_DIR/web.zip"

VERSION_ARG=""
KEEP_TEMP=0
DO_TEST=1
PORT=8083

TMPDIR_CLONE=""
SERVER_PID=""

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
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    if [ -n "$TMPDIR_CLONE" ] && [ -d "$TMPDIR_CLONE" ]; then
        if [ "$KEEP_TEMP" -eq 1 ]; then
            printf '\n    Scratch directory kept at: %s\n' "$TMPDIR_CLONE"
        else
            rm -rf "$TMPDIR_CLONE"
        fi
    fi
}
trap cleanup EXIT INT TERM

usage() {
    cat <<'USAGE'
Usage: update_taminations.sh [version|ref] [options]

  version|ref   Which Taminations to build. One of:
                  (omitted)      main HEAD -- the latest
                  1.6.110        pubspec.yaml version; resolved via git history
                  1.6.110+280    exact version+build number
                  <sha|branch>   anything else is passed to `git checkout`

Options:
  --port N      Port for the local test server (default 8083)
  --no-test     Skip the browser test step
  --keep        Keep the scratch clone (and the old web.zip backup) on exit
  -h, --help    This message
USAGE
}

while [ $# -gt 0 ]; do
    case "$1" in
        --port)    PORT="${2:-}"; [ -n "$PORT" ] || die "--port needs a value"; shift 2 ;;
        --no-test) DO_TEST=0; shift ;;
        --keep)    KEEP_TEMP=1; shift ;;
        -h|--help) usage; exit 0 ;;
        -*)        die "Unknown option: $1  (try --help)" ;;
        *)
            [ -z "$VERSION_ARG" ] || die "Too many arguments: already have '$VERSION_ARG', got '$1'"
            VERSION_ARG="$1"; shift ;;
    esac
done

# ------------------------------------------------------------- preflight ----

step "Preflight"

for tool in flutter git zip unzip python3; do
    command -v "$tool" >/dev/null 2>&1 || die "'$tool' not found in PATH."
done
info "flutter: $(command -v flutter)  ($(flutter --version 2>/dev/null | head -1))"

[ -f "$WEBZIP" ] || die "Expected existing archive not found: $WEBZIP"

REPO_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel 2>/dev/null)" \
    || die "$SCRIPT_DIR is not inside a git working tree."

# What's installed right now?
OLD_VERSION="$(unzip -p "$WEBZIP" web/version.json 2>/dev/null \
    | python3 -c 'import json,sys
try:
    d = json.load(sys.stdin)
    print("%s+%s" % (d.get("version","?"), d.get("build_number","?")))
except Exception:
    print("unknown")' || echo "unknown")"
info "Currently bundled Taminations: $OLD_VERSION"

# --------------------------------------------------------- clone/resolve ----

TMPDIR_CLONE="$(mktemp -d "${TMPDIR:-/tmp}/taminations-update.XXXXXX")"
SRC="$TMPDIR_CLONE/taminations-flutter"

step "Cloning $UPSTREAM_REPO"
info "into $SRC"
git clone --quiet "$UPSTREAM_REPO" "$SRC" || die "Clone failed."

step "Resolving requested version"

resolve_version_to_commit() {
    # Finds the newest commit whose pubspec.yaml carries the target version.
    # The version line is monotonic along history, so we look for the pubspec
    # commit that bumped *past* the target and take its parent.
    local target="$1"
    python3 - "$SRC" "$target" <<'PY'
import re, subprocess, sys

repo, target = sys.argv[1], sys.argv[2]

def git(*args):
    return subprocess.check_output(["git", "-C", repo, *args], text=True)

def version_at(sha):
    try:
        blob = git("show", "%s:pubspec.yaml" % sha)
    except subprocess.CalledProcessError:
        return None
    m = re.search(r"^version:\s*(\S+)\s*$", blob, re.M)
    return m.group(1) if m else None

def matches(v):
    if v is None:
        return False
    # "1.6.110" matches "1.6.110+280"; "1.6.110+280" must match exactly.
    return v == target or ("+" not in target and v.split("+")[0] == target)

# Newest first.
commits = git("log", "--format=%H", "--", "pubspec.yaml").split()
if not commits:
    print("ERROR:no pubspec.yaml history found")
    sys.exit(0)

hit = None
for i, sha in enumerate(commits):
    if matches(version_at(sha)):
        hit = i
        break

if hit is None:
    seen, order = set(), []
    for sha in commits[:40]:
        v = version_at(sha)
        if v and v not in seen:
            seen.add(v)
            order.append(v)
    print("ERROR:version %r not found. Recent versions: %s" % (target, ", ".join(order)))
    sys.exit(0)

if hit == 0:
    # Target is the current tip's version -- take the branch tip itself, which
    # may include commits after the last pubspec.yaml change.
    sha = git("rev-parse", "HEAD").strip()
else:
    # commits[hit-1] is the pubspec commit that moved off the target version;
    # its first parent is the newest commit still at the target.
    sha = git("rev-parse", "%s^" % commits[hit - 1]).strip()

print("OK:%s:%s" % (sha, version_at(sha)))
PY
}

if [ -z "$VERSION_ARG" ]; then
    TARGET_REF="$(git -C "$SRC" rev-parse HEAD)"
    info "No version given -- using main HEAD."
elif [[ "$VERSION_ARG" =~ ^[0-9]+\.[0-9]+\.[0-9]+(\+[0-9]+)?$ ]]; then
    info "Searching pubspec.yaml history for version $VERSION_ARG ..."
    RESULT="$(resolve_version_to_commit "$VERSION_ARG")"
    case "$RESULT" in
        OK:*)    TARGET_REF="$(printf '%s' "$RESULT" | cut -d: -f2)" ;;
        ERROR:*) die "${RESULT#ERROR:}" ;;
        *)       die "Unexpected output from version resolver: $RESULT" ;;
    esac
else
    info "Treating '$VERSION_ARG' as a branch or commit."
    TARGET_REF="$(git -C "$SRC" rev-parse --verify "$VERSION_ARG^{commit}" 2>/dev/null)" \
        || die "'$VERSION_ARG' is not a valid branch, tag, or commit in the upstream repo."
fi

git -C "$SRC" -c advice.detachedHead=false checkout --quiet "$TARGET_REF" \
    || die "Could not check out $TARGET_REF"

NEW_VERSION="$(grep -m1 '^version:' "$SRC/pubspec.yaml" | awk '{print $2}')"
[ -n "$NEW_VERSION" ] || die "Could not read 'version:' from pubspec.yaml"
NEW_VERSION_SHORT="${NEW_VERSION%%+*}"
NEW_BUILD="${NEW_VERSION#*+}"
SHORT_SHA="$(git -C "$SRC" rev-parse --short HEAD)"
COMMIT_DATE="$(git -C "$SRC" log -1 --format=%ci HEAD)"

printf '\n'
info "Version:  $NEW_VERSION"
info "Commit:   $SHORT_SHA  ($COMMIT_DATE)"
info "Replacing bundled version: $OLD_VERSION"
printf '\n'

if [ "$OLD_VERSION" = "$NEW_VERSION" ]; then
    warn "This is the same version that is already bundled."
fi

# Check the Dart SDK constraint up front. Otherwise `flutter pub get` fails
# several steps later, after we have already cloned and patched.
SDK_CHECK="$(python3 - "$SRC/pubspec.yaml" "$(dart --version 2>&1)" <<'PY'
import re, sys

pubspec, dartver = sys.argv[1], sys.argv[2]
blob = open(pubspec, encoding="utf-8").read()

m = re.search(r"^environment:\s*$(.*?)^\S", blob, re.M | re.S)
env = m.group(1) if m else blob
m = re.search(r"^\s*sdk:\s*[\"']?([^\"'\n]+)", env, re.M)
need = m.group(1).strip() if m else None

m = re.search(r"(\d+)\.(\d+)\.(\d+)", dartver)
have = tuple(int(x) for x in m.groups()) if m else None

if not need or not have:
    print("UNKNOWN::")
    raise SystemExit(0)

m = re.search(r">=\s*(\d+)\.(\d+)\.(\d+)", need)
if not m:
    print("OK::%s" % need)
    raise SystemExit(0)

want = tuple(int(x) for x in m.groups())
status = "OK" if have >= want else "TOOOLD"
print("%s:%s:%s" % (status, ".".join(map(str, have)), need))
PY
)"
SDK_STATUS="${SDK_CHECK%%:*}"
SDK_HAVE="$(printf '%s' "$SDK_CHECK" | cut -d: -f2)"
SDK_NEED="$(printf '%s' "$SDK_CHECK" | cut -d: -f3-)"

case "$SDK_STATUS" in
    OK)
        info "Dart SDK: $SDK_HAVE satisfies '$SDK_NEED'"
        ;;
    TOOOLD)
        die "Your Dart SDK is too old to build this version of Taminations.

    Taminations $NEW_VERSION needs:  sdk $SDK_NEED
    You have:                        Dart $SDK_HAVE  (Flutter $(flutter --version 2>/dev/null | head -1 | awk '{print $2}'))

Either upgrade Flutter:

    flutter upgrade

...or pick an older Taminations that your current SDK can build. Nothing in the
SquareDesk tree has been changed."
        ;;
    *)
        warn "Could not compare the Dart SDK constraint; continuing anyway."
        ;;
esac

confirm "Build this version?" yes || die "Aborted at your request."

# ---------------------------------------------------------- source patch ----

step "Patching lib/main.dart for offline fonts (issue #1467, step 1)"

python3 - "$SRC/lib/main.dart" <<'PY'
import re, sys

path = sys.argv[1]
src = open(path, encoding="utf-8").read()

MARKER = "GoogleFonts.config.allowRuntimeFetching"
if MARKER in src:
    print("SKIP:already patched")
    raise SystemExit(0)

# Upstream writes this as `fm.WidgetsFlutterBinding.ensureInitialized();`
# (flutter/material.dart is imported with the `fm` prefix), so allow an
# optional library prefix.
pat = re.compile(
    r"^([ \t]*)((?:[A-Za-z_]\w*\.)?WidgetsFlutterBinding\.ensureInitialized\(\);)[ \t]*$",
    re.M,
)
m = pat.search(src)
if not m:
    print("FAIL:could not find the WidgetsFlutterBinding.ensureInitialized(); anchor line")
    raise SystemExit(0)

indent = m.group(1)
addition = (
    "\n"
    f"{indent}// SquareDesk: use the bundled google_fonts assets instead of the CDN,\n"
    f"{indent}// so Taminations works with no internet connection. (SquareDesk #1467)\n"
    f"{indent}GoogleFonts.config.allowRuntimeFetching = false;"
)
src = src[: m.end()] + addition + src[m.end():]
open(path, "w", encoding="utf-8").write(src)
print("OK:patched")
PY

PATCH_RESULT="$(python3 - "$SRC/lib/main.dart" <<'PY'
import sys
print("YES" if "GoogleFonts.config.allowRuntimeFetching" in open(sys.argv[1], encoding="utf-8").read() else "NO")
PY
)"
[ "$PATCH_RESULT" = "YES" ] || die "main.dart patch failed.
Upstream has probably refactored main(). Read issue #1467 and re-derive the recipe
by hand; do not ship an unpatched build (it would require internet for fonts)."
ok "main.dart: google_fonts runtime fetching disabled"

# ---------------------------------------------------------------- build -----

step "Building Taminations for the web"
info "(this takes a few minutes)"

( cd "$SRC" && flutter pub get ) || die "flutter pub get failed."
( cd "$SRC" && flutter build web --release \
      --dart-define=FLUTTER_WEB_CANVASKIT_URL=canvaskit/ \
      --no-tree-shake-icons ) || die "flutter build web failed."

BUILD_WEB="$SRC/build/web"
[ -f "$BUILD_WEB/index.html" ] || die "Build finished but $BUILD_WEB/index.html is missing."
ok "Built into $BUILD_WEB"

# ----------------------------------------------------- post-build patches ---

step "Patching flutter_bootstrap.js for local CanvasKit (issue #1467, step 3)"

python3 - "$BUILD_WEB/flutter_bootstrap.js" <<'PY'
import sys

path = sys.argv[1]
src = open(path, encoding="utf-8").read()

# NB: do NOT test the whole file for "canvasKitBaseUrl" -- the minified
# flutter.js loader that is inlined at the top of this file mentions that name
# itself. Only the _flutter.loader.load({...}) call at the bottom matters.
CALL = "_flutter.loader.load({"
start = src.rfind(CALL)
if start < 0:
    print("FAIL:could not find the _flutter.loader.load({ ... }) call")
    raise SystemExit(0)

if "canvasKitBaseUrl" in src[start:]:
    print("SKIP:already configured")
    raise SystemExit(0)

# Brace-match forward from the opening brace of the argument object.
i = start + len(CALL) - 1
depth = 0
end = -1
while i < len(src):
    c = src[i]
    if c == "{":
        depth += 1
    elif c == "}":
        depth -= 1
        if depth == 0:
            end = i
            break
    i += 1

if end < 0:
    print("FAIL:unbalanced braces in the _flutter.loader.load() call")
    raise SystemExit(0)

# Insert `config: {...}` as the last key of the argument object, preserving
# whatever serviceWorkerVersion the build generated.
head = src[:end].rstrip()
if not head.endswith(","):
    head += ","
addition = '\n  config: {\n    canvasKitBaseUrl: "canvaskit/"\n  }\n'
open(path, "w", encoding="utf-8").write(head + addition + src[end:])
print("OK:patched")
PY

# Verify inside the loader.load() call only -- the inlined flutter.js loader
# mentions canvasKitBaseUrl on its own, so a whole-file grep always succeeds.
BOOTSTRAP_OK="$(python3 - "$BUILD_WEB/flutter_bootstrap.js" <<'PY'
import sys
src = open(sys.argv[1], encoding="utf-8").read()
i = src.rfind("_flutter.loader.load({")
print("YES" if i >= 0 and "canvasKitBaseUrl" in src[i:] else "NO")
PY
)"
[ "$BOOTSTRAP_OK" = "YES" ] \
    || die "flutter_bootstrap.js patch failed.
The Flutter SDK has probably changed the shape of the generated bootstrap.
Read issue #1467 and apply the canvasKitBaseUrl config by hand; without it
Taminations fetches CanvasKit from gstatic.com and will not work offline."
ok "flutter_bootstrap.js: canvasKitBaseUrl set to canvaskit/"

step "Patching index.html for URL parameters (issues #1415 / #1456)"

# The snippet below is reproduced verbatim from issue #1415. It runs before
# Flutter loads and seeds localStorage from the query string, which is how
# SquareDesk passes ?Speed=Fast to default the sequencer dancer speed.
URLPARAM_SNIPPET="$TMPDIR_CLONE/urlparam_snippet.html"
cat > "$URLPARAM_SNIPPET" <<'SNIPPET'
  <!-- URL Parameter Processing Script -->
  <script>
    // Process URL parameters before Flutter loads
    (function() {
      try {
        const urlParams = new URLSearchParams(window.location.search);
        console.log('Processing URL parameters:', window.location.search);

        // Map of URL parameters to Flutter settings
        const parameterMappings = {
          'Speed': 'Dancer Speed',
          'Loop': 'Loop',
          'Grid': 'Grid',
          'Paths': 'Paths',
          'Numbers': 'Numbers',
          'Phantoms': 'Phantoms'
        };

        // Process each parameter
        for (const [urlParam, settingName] of Object.entries(parameterMappings)) {
          const value = urlParams.get(urlParam);
          if (value !== null) {
            console.log(`Setting ${settingName} to ${value}`);

            // Flutter uses JSON encoding for localStorage
            try {
              localStorage.setItem(settingName, JSON.stringify(value));
              console.log(`Stored ${settingName} as JSON:`, JSON.stringify(value));
            } catch (e) {
              console.warn('Failed to set localStorage item:', e);
            }
          }
        }

        // Special handling for boolean parameters (checkboxes)
        const booleanParams = ['Loop', 'Grid', 'Paths', 'Phantoms'];
        booleanParams.forEach(param => {
          if (urlParams.has(param)) {
            const value = urlParams.get(param);
            const boolValue = value === null || value === '' || value.toLowerCase() === 'true';
            console.log(`Setting boolean ${param} to ${boolValue}`);
            try {
              localStorage.setItem(param, JSON.stringify(boolValue));
              console.log(`Stored ${param} as JSON:`, JSON.stringify(boolValue));
            } catch (e) {
              console.warn('Failed to set boolean localStorage item:', e);
            }
          }
        });

        // Debug: Show what we stored
        console.log('Current localStorage after processing:');
        for (let i = 0; i < localStorage.length; i++) {
          const key = localStorage.key(i);
          if (key === 'Dancer Speed' || key === 'Speed') {
            console.log(`${key}: ${localStorage.getItem(key)}`);
          }
        }

      } catch (e) {
        console.error('Error processing URL parameters:', e);
      }
    })();
  </script>

SNIPPET

python3 - "$BUILD_WEB/index.html" "$URLPARAM_SNIPPET" <<'PY'
import re, sys

path, snippet_path = sys.argv[1], sys.argv[2]
src = open(path, encoding="utf-8").read()

if "URL Parameter Processing Script" in src:
    print("SKIP:already patched")
    raise SystemExit(0)

snippet = open(snippet_path, encoding="utf-8").read()

# Insert immediately before the flutter_bootstrap.js tag, so the parameters
# land in localStorage before the Flutter app reads its settings.
m = re.search(r'^[ \t]*<script[^>]*src="flutter_bootstrap\.js"[^>]*>\s*</script>[ \t]*$',
              src, re.M)
if not m:
    print("FAIL:could not find the flutter_bootstrap.js <script> tag")
    raise SystemExit(0)

src = src[: m.start()] + snippet + src[m.start():]
open(path, "w", encoding="utf-8").write(src)
print("OK:patched")
PY

grep -q 'URL Parameter Processing Script' "$BUILD_WEB/index.html" \
    || die "index.html patch failed.
The generated index.html no longer has the expected flutter_bootstrap.js script
tag. Read issue #1415 and inject the URL-parameter snippet by hand; without it
the sequencer will not default to Fast speed."
ok "index.html: URL parameter script injected"

# ------------------------------------------------------- offline checks -----

step "Checking that the build is self-contained (issue #1467)"

OFFLINE_OK=1
for d in canvaskit assets/fonts; do
    if [ -d "$BUILD_WEB/$d" ]; then
        info "found $d/"
    else
        warn "missing $BUILD_WEB/$d"
        OFFLINE_OK=0
    fi
done

if find "$BUILD_WEB" -type d -name 'google_fonts' -print -quit | grep -q .; then
    info "found bundled google_fonts assets"
else
    warn "no bundled google_fonts directory found in the build"
    OFFLINE_OK=0
fi

# The minified flutter.js loader inlined at the top of flutter_bootstrap.js
# always carries the gstatic CanvasKit URL as its built-in default -- it is
# present even in builds that are verified to work offline. Only a mention
# *after* the loader.load({...}) call would mean our override didn't take, and
# the dedicated check above already proves canvasKitBaseUrl is in the config.
if [ ! -f "$BUILD_WEB/canvaskit/canvaskit.js" ]; then
    warn "canvaskit/canvaskit.js is missing -- CanvasKit would be fetched from the network"
    OFFLINE_OK=0
fi

if [ "$OFFLINE_OK" -eq 1 ]; then
    ok "Build looks self-contained."
else
    warn "The offline checks above did not all pass."
    warn "Test with SquareDesk disconnected from the network before shipping this."
    confirm "Continue anyway?" no || die "Aborted at your request."
fi

# ---------------------------------------------------------------- test ------

TEST_URL_PATH='/index.html?Speed=Fast#/?main=SEQUENCER&detail=HELP&formation=Squared+Set&helplink=info/sequencer'

if [ "$DO_TEST" -eq 1 ]; then
    step "Starting a local test server"

    # Find a free port, starting at $PORT.
    PORT="$(python3 - "$PORT" <<'PY'
import socket, sys
start = int(sys.argv[1])
for p in range(start, start + 50):
    s = socket.socket()
    try:
        s.bind(("127.0.0.1", p))
        print(p)
        sys.exit(0)
    except OSError:
        continue
    finally:
        s.close()
print(start)
PY
)"

    python3 -m http.server "$PORT" --bind 127.0.0.1 --directory "$BUILD_WEB" >/dev/null 2>&1 &
    SERVER_PID=$!
    sleep 1
    kill -0 "$SERVER_PID" 2>/dev/null || die "Test server failed to start on port $PORT."

    TEST_URL="http://localhost:$PORT$TEST_URL_PATH"
    info "Serving $BUILD_WEB on http://localhost:$PORT"
    info "Opening: $TEST_URL"
    open "$TEST_URL" 2>/dev/null || warn "Could not open a browser -- visit the URL above yourself."

    cat <<EOT

    Things worth checking:
      - Taminations $NEW_VERSION loads and animates
      - the sequencer's Dancer Speed defaults to Fast
      - the Help page is present
      - turn off wi-fi and reload: it should still work

EOT

    if ! confirm "Install this build into SquareDesk?" no; then
        die "Not installed. Nothing in the SquareDesk tree was changed."
    fi

    kill "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
    SERVER_PID=""
else
    info "Skipping the test step (--no-test)."
fi

# -------------------------------------------------------------- package -----

step "Packaging web.zip (issue #1415)"

NEW_ZIP="$SRC/build/web.zip"
rm -f "$NEW_ZIP"

# The archive must contain a top-level "web/" directory: qmake unzips it into
# Resources/Taminations/, producing Resources/Taminations/web/ (see #1437).
( cd "$SRC/build" && zip -r -X -q web.zip web -x '*/.DS_Store' '__MACOSX/*' ) \
    || die "zip failed."

# Belt and braces: strip the macOS junk the way issue #1415 describes. These
# fail harmlessly when there is nothing to delete.
( cd "$SRC/build" && zip -d web.zip '__MACOSX/*'   >/dev/null 2>&1 ) || true
( cd "$SRC/build" && zip -d web.zip '*/.DS_Store'  >/dev/null 2>&1 ) || true

JUNK="$(unzip -l "$NEW_ZIP" | grep -c '__MACOSX\|\.DS_Store' || true)"
[ "$JUNK" -eq 0 ] || die "web.zip still contains $JUNK macOS junk entries."

unzip -l "$NEW_ZIP" | grep -q ' web/index.html$' \
    || die "web.zip does not contain a top-level web/index.html -- the archive
layout is wrong and SquareDesk would unpack it to the wrong place (see #1437)."
unzip -l "$NEW_ZIP" | grep -q ' web/flutter_bootstrap.js$' \
    || die "web.zip is missing web/flutter_bootstrap.js."
unzip -l "$NEW_ZIP" | grep -q ' web/canvaskit/' \
    || warn "web.zip contains no web/canvaskit/ files -- offline use may be broken."

NEW_COUNT="$(unzip -l "$NEW_ZIP" | tail -1 | awk '{print $2}')"
ok "web.zip built: $NEW_COUNT files, $(du -h "$NEW_ZIP" | cut -f1)"

# -------------------------------------------------------------- install -----

step "Installing into $WEBZIP"

BACKUP="$TMPDIR_CLONE/web.zip.previous-$OLD_VERSION"
cp "$WEBZIP" "$BACKUP"
info "Previous archive backed up to $BACKUP"
cp "$NEW_ZIP" "$WEBZIP"

ok "Taminations updated: $OLD_VERSION  ->  $NEW_VERSION"
info "Archive: $WEBZIP  ($(du -h "$WEBZIP" | cut -f1))"

# ------------------------------------------------------- issue + commit -----

step "Recording the update"

ISSUE=""
while [ -z "$ISSUE" ]; do
    read -r -p "    GitHub issue number for this Taminations update (digits only): " ISSUE </dev/tty || true
    ISSUE="$(printf '%s' "$ISSUE" | tr -d ' #')"
    if ! [[ "$ISSUE" =~ ^[0-9]+$ ]]; then
        warn "Please enter just the issue number, e.g. 1523"
        ISSUE=""
    fi
done

COMMIT_MSG="#$ISSUE: Update Taminations to $NEW_VERSION_SHORT (build $NEW_BUILD)

Rebuilt from bradchristie/taminations-flutter@$SHORT_SHA using
Taminations/update_taminations.sh. Previous bundled version: $OLD_VERSION.

Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>"

MSG_FILE="$SCRIPT_DIR/.update_taminations_commit_msg.txt"
printf '%s\n' "$COMMIT_MSG" > "$MSG_FILE"

cat <<EOT

$B    NEXT: build and test SquareDesk before committing.$N

      1. Build SquareDesk (qmake will unzip the new web.zip into
         SquareDesk.app/Contents/Resources/Taminations/web).
      2. Open the Taminations tab and confirm it works, including offline.

EOT

if confirm "Have you ALREADY built and tested SquareDesk with this new Taminations, and want to commit now?" no; then
    git -C "$REPO_ROOT" add "$WEBZIP" || die "git add failed."
    git -C "$REPO_ROOT" commit -F "$MSG_FILE" || die "git commit failed."
    rm -f "$MSG_FILE"
    ok "Committed."
    printf '\n'
    info "Not pushed. When you are ready:"
    info "    git -C $REPO_ROOT push"
    info "Then comment the commit details on issue #$ISSUE and label it 'Ready for final check'."
else
    printf '\n'
    info "Nothing committed. web.zip is updated but unstaged."
    info "After you have built and tested SquareDesk:"
    printf '\n'
    info "    git -C $REPO_ROOT add $WEBZIP"
    info "    git -C $REPO_ROOT commit -F $MSG_FILE"
    info "    git -C $REPO_ROOT push"
    printf '\n'
    info "To undo instead:"
    info "    git -C $REPO_ROOT checkout -- $WEBZIP"
fi

printf '\n'
ok "Done."
