# third_party

Code in this directory is **not ours**. We did not write it, and we do not edit
it. That is the whole point of the directory: the rule is mechanical, so nobody
has to remember which files are vendored (see issue #1748).

The same rule applies to `SquareDesk-DEV/test123/third_party/`, which holds the
vendored files that have to sit next to the app sources.

| Directory | What it is | Upstream |
|---|---|---|
| `sdlib/` | Sd, the square dance choreography engine | Bill Ackerman -- https://challengedance.org/sd/ |
| `taglib/` | ID3 / audio metadata library | https://taglib.org |
| `kfr/` | DSP library, used for our filters | https://github.com/kfrlib/kfr |
| `qpdfjs/` | PDF.js viewer wrapper | https://github.com/yshurik/qpdfjs |
| `libJUCEstatic/` | JUCE static library project (Linux build) | https://juce.com |
| `Taminations/` | Dance animation web app, vendored as `web.zip` | https://github.com/bradchristie/taminations-flutter |

## Updating one of these

Two of them have update scripts that do the whole job, including fetching
upstream and writing a commit message:

- `sdlib/update_sd.sh`
- `Taminations/update_taminations.sh`

For the rest, replace the tree wholesale rather than patching it in place. If a
local change is genuinely unavoidable, say so in the commit message, because the
next update will silently drop it.

## Exceptions

`SquareDesk-DEV/test123/themes/Themes.qss` is **ours**, despite being a QSS file
of the kind that often is not. It is not in here, and it should not be.
