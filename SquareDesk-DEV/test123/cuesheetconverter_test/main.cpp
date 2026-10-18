// Test harness for the cuesheet converter (#1789).
//
// Usage: cuesheetconverter_test [--mode auto|italic|nobold] [--no-case] [--filter GLOB] TESTDIR
//        cuesheetconverter_test --dump FILE      (print the flattened lines of one file)
//
//   Converts every TESTDIR/source/**/GLOB.htm[l] (default GLOB "*"; GLOB may be a
//   comma-separated list, e.g. "RIV*,ARROW*") into
//   TESTDIR/out/**, and writes TESTDIR/index.html, which shows each source
//   (left) beside its conversion (right).

#include "cuesheetconverter.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>

static const char *kIndexTemplate = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Cuesheet Converter Test</title>
<style>
:root { --bg:#f6f5f0; --fg:#1d1d1f; --mut:#6e6e73; --card:#fff; --line:#d9d7cf; --acc:#0a6cff; --warn:#b25e00; --ok:#1a7f37; }
@media (prefers-color-scheme: dark) { :root { --bg:#1c1c1e; --fg:#f2f2f7; --mut:#9a9aa0; --card:#2a2a2d; --line:#3d3d41; --acc:#4c9aff; --warn:#ffb454; --ok:#4cc26a; } }
* { box-sizing: border-box; }
html, body { height: 100%; margin: 0; }
body { font: 13px/1.4 -apple-system, BlinkMacSystemFont, "Helvetica Neue", sans-serif; background: var(--bg); color: var(--fg);
       display: grid; grid-template-columns: 260px 1fr; }
#list { overflow: auto; border-right: 1px solid var(--line); background: var(--card); }
#list h1 { font-size: 13px; margin: 0; padding: 10px; border-bottom: 1px solid var(--line); position: sticky; top: 0; background: var(--card); }
#list h1 small { display: block; color: var(--mut); font-weight: normal; }
.it { padding: 6px 10px; border-bottom: 1px solid var(--line); cursor: pointer; }
.it.sel { background: color-mix(in srgb, var(--acc) 18%, transparent); }
.it small { display: block; color: var(--ok); }
.it small.w { color: var(--warn); }
#main { display: grid; grid-template-rows: auto 1fr; min-width: 0; }
#bar { padding: 8px 12px; border-bottom: 1px solid var(--line); display: flex; gap: 12px; align-items: baseline; flex-wrap: wrap; }
#bar b { font-size: 14px; }
#msg { color: var(--warn); }
#msg.ok { color: var(--ok); }
#panes { display: grid; grid-template-columns: 1fr 1fr; min-height: 0; }
.pane { display: grid; grid-template-rows: auto 1fr; min-width: 0; min-height: 0; border-left: 1px solid var(--line); }
.pane .hd { padding: 3px 8px; color: var(--mut); font-size: 11px; display: flex; gap: 12px; align-items: baseline; }
.pane .hd span { text-transform: uppercase; letter-spacing: .05em; }
.pane .hd label { margin-left: auto; }
iframe, textarea { width: 100%; height: 100%; border: 0; background: #fff; }
textarea { font: 12px/1.4 ui-monospace, Menlo, monospace; background: var(--card); color: var(--fg); resize: none; padding: 8px; }
</style>
</head>
<body>
<div id="list"><h1>Cuesheet Converter Test<small>__SUMMARY__ &middot; &uarr;/&darr; or j/k</small></h1></div>
<div id="main">
  <div id="bar"><b id="name"></b><span id="msg"></span></div>
  <div id="panes">
    <div class="pane"><div class="hd"><span>Before (source)</span><label><input type="checkbox" id="rawBefore"> show HTML source</label></div>
      <iframe id="before"></iframe><textarea id="srcBefore" readonly hidden></textarea></div>
    <div class="pane"><div class="hd"><span>After (converted)</span><label><input type="checkbox" id="raw"> show converted HTML source</label></div>
      <iframe id="after"></iframe><textarea id="src" readonly hidden></textarea></div>
  </div>
</div>
<script>
const DATA = __DATA__;
const $ = id => document.getElementById(id);
const enc = p => p.split('/').map(encodeURIComponent).join('/');
let sel = 0;
try { const s = parseInt(localStorage.getItem('sel')); if (s >= 0 && s < DATA.length) sel = s; } catch (e) {}
DATA.forEach((f, i) => {
  const d = document.createElement('div');
  d.className = 'it';
  d.textContent = f.name;
  const s = document.createElement('small');
  s.className = f.warnings.length ? 'w' : '';
  s.textContent = f.warnings.length ? '⚠ ' + f.warnings.length + ' warning(s) · ' + f.info : f.info;
  d.appendChild(s);
  d.onclick = () => show(i);
  $('list').appendChild(d);
});
function show(i) {
  sel = i;
  try { localStorage.setItem('sel', i); } catch (e) {}
  const items = document.querySelectorAll('.it');
  items.forEach((el, n) => el.classList.toggle('sel', n === i));
  items[i].scrollIntoView({ block: 'nearest' });
  const f = DATA[i];
  $('name').textContent = f.src;
  $('msg').className = f.warnings.length ? '' : 'ok';
  $('msg').textContent = f.warnings.length ? f.warnings.join(' ') : f.info;
  $('before').src = enc(f.src);
  $('after').src = enc(f.out);
  $('src').value = f.html;
  $('srcBefore').value = f.source;
}
$('raw').onchange = () => { $('src').hidden = !$('raw').checked; $('after').hidden = $('raw').checked; };
$('rawBefore').onchange = () => { $('srcBefore').hidden = !$('rawBefore').checked; $('before').hidden = $('rawBefore').checked; };
document.addEventListener('keydown', e => {
  if (e.key === 'ArrowDown' || e.key === 'j') { e.preventDefault(); show(Math.min(sel + 1, DATA.length - 1)); }
  if (e.key === 'ArrowUp' || e.key === 'k') { e.preventDefault(); show(Math.max(sel - 1, 0)); }
});
if (DATA.length) show(sel);
</script>
</body>
</html>
)HTML";

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);    // QTextDocument needs fonts

    CuesheetConverter::Mode mode = CuesheetConverter::Mode::Auto;
    QString filter = "*";
    QString testDir;
    bool fixCase = true;
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); i++) {
        if (args[i] == "--mode" && i + 1 < args.size()) {
            const QString m = args[++i];
            mode = m == "italic" ? CuesheetConverter::Mode::Italic
                 : m == "nobold" ? CuesheetConverter::Mode::NoBold
                                 : CuesheetConverter::Mode::Auto;
        } else if (args[i] == "--dump" && i + 1 < args.size()) {
            QFile f(args[++i]);
            if (f.open(QIODevice::ReadOnly)) {
                QTextStream(stdout) << CuesheetConverter::dumpLines(CuesheetConverter::decode(f.readAll()));
            }
            return 0;
        } else if (args[i] == "--no-case") {
            fixCase = false;
        } else if (args[i] == "--filter" && i + 1 < args.size()) {
            filter = args[++i];
        } else {
            testDir = args[i];
        }
    }
    if (testDir.isEmpty()) {
        QTextStream(stderr) << "usage: cuesheetconverter_test [--mode auto|italic|nobold] [--no-case] [--filter GLOB] TESTDIR\n";
        return 1;
    }

    const QDir root(QFileInfo(testDir).absoluteFilePath());    // a relative "." breaks relativeFilePath() below
    const QDir sourceDir(root.filePath("source"));
    QStringList files;
    QStringList globs;
    for (const QString &f : filter.split(',', Qt::SkipEmptyParts)) {
        globs << f.trimmed() + ".htm" << f.trimmed() + ".html";
    }
    QDirIterator it(sourceDir.path(), globs, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        files << sourceDir.relativeFilePath(it.next());
    }
    files.sort(Qt::CaseInsensitive);

    QJsonArray data;
    int nWarned = 0;
    for (const QString &rel : std::as_const(files)) {
        QFile in(sourceDir.filePath(rel));
        if (!in.open(QIODevice::ReadOnly)) {
            QTextStream(stderr) << "cannot read " << rel << "\n";
            continue;
        }
        const QString source = CuesheetConverter::decode(in.readAll());
        const CuesheetConverter::Result r = CuesheetConverter::convert(source, mode, "1.2.2", fixCase, rel);

        const QString outRel = "out/" + rel;
        QDir().mkpath(QFileInfo(root.filePath(outRel)).path());
        QFile out(root.filePath(outRel));
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            out.write(r.html.toUtf8());
        }

        QJsonObject o;
        o["name"] = QFileInfo(rel).completeBaseName();
        o["src"] = "source/" + rel;
        o["out"] = outRel;
        o["info"] = r.info;
        o["warnings"] = QJsonArray::fromStringList(r.warnings);
        o["html"] = r.html;
        // The source, for "show HTML source", with embedded images shortened.
        static const QRegularExpression dataUri(QStringLiteral("data:[^\"')]{200,}"));
        QString shortSource = source;
        shortSource.replace(dataUri, QStringLiteral("data:...(embedded data omitted)"));
        o["source"] = shortSource;
        data.append(o);
        if (!r.warnings.isEmpty()) {
            nWarned++;
        }
        QTextStream(stdout) << rel << ": " << r.info
                            << (r.warnings.isEmpty() ? QString() : "  [" + r.warnings.join(" ") + "]") << "\n";
    }

    QString index = QString::fromUtf8(kIndexTemplate);
    QString json = QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact));
    json.replace("</", "<\\/");     // keep "</script>" inside the data from ending the script
    index.replace("__SUMMARY__", QString("%1 files, %2 with warnings").arg(data.size()).arg(nWarned));
    index.replace("__DATA__", json);
    QFile idx(root.filePath("index.html"));
    if (idx.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        idx.write(index.toUtf8());
    }
    QTextStream(stdout) << data.size() << " files converted, " << nWarned << " with warnings -> "
                        << root.filePath("index.html") << "\n";
    return 0;
}
