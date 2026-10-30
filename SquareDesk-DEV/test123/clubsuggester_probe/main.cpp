// Probe for the Suggest Clubs review dialog (#1814).
//
// Usage: clubsuggester_probe [--asof yyyy-MM-dd] [--no-clubs] [--theme Light|Dark]
//                            [--screenshot PNG [--select N]] [--accept] [--import ANSWER] DBFILE
//
//   Opens DBFILE (a copy of SquareDesk.sqlite3) read-only, suggests clubs from its play times, and
//   shows the review dialog.  --no-clubs pretends the user has no clubs yet, as a new user would.
//   OK shows what would be saved, and how many plays each club would then hold; nothing is ever
//   written to DBFILE.  --screenshot saves a picture of the dialog (with suggestion N selected)
//   and quits, for reviewing it without opening a window (QT_QPA_PLATFORM=offscreen).  --accept
//   clicks OK at once and prints what would be saved.

#include "suggestclubsdialog.h"
#include "clubsai.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFile>
#include <QFontDatabase>
#include <QPlainTextEdit>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStyle>
#include <QTextStream>
#include <QTimeZone>
#include <QVBoxLayout>

static QTextStream out(stdout);

static void stampTheme(QWidget *widget, const QString &theme)
{
    QList<QWidget *> widgets = widget->findChildren<QWidget *>();
    widgets.prepend(widget);
    for (QWidget *w : std::as_const(widgets)) {
        w->setProperty("theme", theme);
        w->style()->unpolish(w);
        w->style()->polish(w);
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QStringList args = app.arguments().mid(1);
    QDate asOf = QDate::currentDate();
    bool noClubs = false;
    bool acceptNow = false;
    QString importFile;   // an AI's answer: show its clubs instead of the suggester's (Import Club Rules from AI)
    QString theme = "Light";
    QString dbFile, screenshot;
    int select = 0;
    for (int i = 0; i < args.size(); ++i) {
        if (args[i] == "--asof" && i + 1 < args.size()) {
            asOf = QDate::fromString(args[++i], "yyyy-MM-dd");
        } else if (args[i] == "--import" && i + 1 < args.size()) {
            importFile = args[++i];
        } else if (args[i] == "--accept") {
            acceptNow = true;
        } else if (args[i] == "--no-clubs") {
            noClubs = true;
        } else if (args[i] == "--screenshot" && i + 1 < args.size()) {
            screenshot = args[++i];
        } else if (args[i] == "--select" && i + 1 < args.size()) {
            select = args[++i].toInt();
        } else if (args[i] == "--theme" && i + 1 < args.size()) {
            theme = args[++i];
        } else {
            dbFile = args[i];
        }
    }
    if (dbFile.isEmpty() || !asOf.isValid()) {
        out << "Usage: clubsuggester_probe [--asof yyyy-MM-dd] [--no-clubs] [--theme Light|Dark] DBFILE\n";
        return 1;
    }

    QFile qss(THEMES_QSS);
    if (qss.open(QIODevice::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
    } else {
        out << "Note: no " << THEMES_QSS << ", so the dialog is unstyled\n";
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbFile);
    db.setConnectOptions("QSQLITE_OPEN_READONLY");
    if (!db.open()) {
        out << "Can't open " << dbFile << ": " << db.lastError().text() << "\n";
        return 1;
    }

    // SquareDesk's own plays only, as for Age and Recent
    QList<QDateTime> plays;
    QSqlQuery q(db);
    q.exec("SELECT played_on FROM song_plays WHERE origin = 'squaredesk' ORDER BY played_on");
    while (q.next()) {
        QDateTime t = QDateTime::fromString(q.value(0).toString(), "yyyy-MM-dd HH:mm:ss");
        t.setTimeZone(QTimeZone::UTC);
        if (t.isValid()) {
            plays.append(t);
        }
    }

    QList<ClubInfo> clubs;
    if (!noClubs) {
        q.exec("SELECT rowid, name, color FROM clubs WHERE NOT deleted ORDER BY order_number");
        while (q.next()) {
            ClubInfo c;
            c.id = q.value(0).toInt();
            c.name = q.value(1).toString();
            c.color = q.value(2).toString();
            QSqlQuery r(db);
            r.prepare("SELECT kind, weekday, nth, start_minutes, end_minutes, since_date, until_date, label"
                      " FROM club_rules WHERE club_rowid = ? ORDER BY order_number");
            r.addBindValue(c.id);
            r.exec();
            while (r.next()) {
                ClubRule rule;
                rule.kind = r.value(0).toInt();
                rule.weekday = r.value(1).toInt();
                rule.nth = r.value(2).toInt();
                rule.start_minutes = r.value(3).toInt();
                rule.end_minutes = r.value(4).toInt();
                rule.since = QDate::fromString(r.value(5).toString(), "yyyy-MM-dd");
                rule.until = QDate::fromString(r.value(6).toString(), "yyyy-MM-dd");
                rule.label = r.value(7).toString();
                c.rules.append(rule);
            }
            sortClubRules(c.rules);
            clubs.append(c);
        }
    }

    QList<ClubSuggestion> suggestions;
    if (importFile.isEmpty()) {
        suggestions = suggestClubs(plays, asOf);
        matchExistingClubs(suggestions, clubs, asOf);
    } else {
        QFile answer(importFile);
        if (!answer.open(QIODevice::ReadOnly)) {
            out << "Can't read " << importFile << "\n";
            return 1;
        }
        QStringList problems;
        const QList<ClubInfo> imported = clubsFromAIAnswer(QString::fromUtf8(answer.readAll()), &problems);
        for (const QString &p : std::as_const(problems)) {
            out << "Problem: " << p << "\n";
        }
        suggestions = suggestionsForImportedClubs(imported, plays, clubs, asOf);
    }
    out << plays.size() << " plays, " << suggestions.size() << " suggestions, " << clubs.size() << " existing clubs\n";
    if (suggestions.isEmpty()) {
        out << "Nothing to suggest\n";
        return 0;
    }

    SuggestClubsDialog dialog(suggestions, clubs, asOf, theme);
    if (!importFile.isEmpty()) {
        dialog.setIntroText(importedClubsIntroText());
    }
    if (!screenshot.isEmpty()) {
        dialog.show();
        dialog.selectSuggestion(select);
        app.processEvents();
        dialog.grab().save(screenshot);
        return 0;
    }
    if (acceptNow) {
        dialog.accept();
    } else if (dialog.exec() != QDialog::Accepted) {
        out << "Cancelled\n";
        return 0;
    }

    // what OK would save, and what it would mean for Age and Recent
    QString report;
    QTextStream r(&report);
    r << "This is what OK would save.  (This probe never writes to the database.)\n";
    for (const ClubInfo &club : dialog.clubs()) {
        const QList<ClubWindow> windows = clubEventWindowsUtc(club.rules, asOf);
        int inside = 0;
        for (const QDateTime &t : std::as_const(plays)) {
            inside += clubWindowsContain(windows, t) ? 1 : 0;
        }
        int before = -1;
        for (const ClubInfo &old : std::as_const(clubs)) {
            if (old.id == club.id && club.id >= 0) {
                const QList<ClubWindow> oldWindows = clubEventWindowsUtc(old.rules, asOf);
                before = 0;
                for (const QDateTime &t : std::as_const(plays)) {
                    before += clubWindowsContain(oldWindows, t) ? 1 : 0;
                }
            }
        }
        r << "\n" << club.name << "  " << club.color
          << (club.id < 0 ? "  (new)" : "  (existing)") << "  " << inside << " plays";
        if (before >= 0 && before != inside) {
            r << " (was " << before << ")";
        }
        r << "\n";
        int oneOffs = 0;
        for (const ClubRule &rule : club.rules) {
            if (rule.isOneOff()) {
                if (++oneOffs > 8) {
                    continue;
                }
            }
            r << "    " << clubRuleSentence(rule) << "\n";
        }
        if (oneOffs > 8) {
            r << "    ... " << (oneOffs - 8) << " more one-offs (" << oneOffs << " in all)\n";
        }
    }
    r.flush();
    out << report;
    if (acceptNow) {
        return 0;
    }

    QDialog result;
    result.setWindowTitle("What OK Would Save");
    result.resize(820, 600);
    QPlainTextEdit *text = new QPlainTextEdit(report);
    text->setReadOnly(true);
    text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    QDialogButtonBox *close = new QDialogButtonBox(QDialogButtonBox::Close);
    QObject::connect(close, &QDialogButtonBox::rejected, &result, &QDialog::reject);
    QVBoxLayout *layout = new QVBoxLayout(&result);
    layout->addWidget(text);
    layout->addWidget(close);
    stampTheme(&result, theme);
    result.exec();
    return 0;
}
