#include <iostream>
using namespace std;
#include <QtDebug>
#include "abbrev.h"
#include "mainwindow.h"

static QHash<QString, QString> designators;
static QHash<QString, QString> calls;

void populateFromCSV(QString filename, QHash<QString, QString> &h) {
    QFile inputFile("/Users/mpogue/clean3/SquareDesk/SquareDesk-DEV/test123/" + filename);

    if (inputFile.open(QIODevice::ReadOnly)) { // defaults to Text mode
        QTextStream in(&inputFile);
        QString header = in.readLine();  // read header (and throw away for now), should be "abspath,pitch,tempo"

        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList sl = MainWindow::parseCSV(line);
//            qDebug() << sl[0] << sl[1];
            h[sl[0]] = sl[1];
        }

//        qDebug() << h;
    }
}

void initAbbrev(void) {
    populateFromCSV("designators.csv", designators);
    populateFromCSV("calls.csv", calls);
}

QString abbrev2call(QString s) {
    QString d;
    QString c;
    if (s[0] == "-") {
        if (s[1] == "-") {
          d = "--";   // special case: -- designator
          c = s.mid(2);
        } else {
            d = " -";  // special case: - "same one"
            c = s.mid(1);
        }
    } else {
        if (designators.contains(s.mid(0,2))) {
            d = s.mid(0,2);   // known designator
            c = s.mid(2);
        } else {
            d = ""; // no designator
            c = s;
        }
    }

    return(designators[d] + " " + calls[c]);
}

QString call2abbrev(QString s) {

    return(s);
}

void testAbbrev(void) {
    initAbbrev();

    QString a = "H-SqTh4 --SwThr B-RunR --Feris C-PasTh --AL";
    QRegExp rx("(\\ |\\,|\\.|\\:|\\t)+"); // RegEx for ' ' or ',' or '.' or ':' or '\t'
    QStringList sequence = a.split(rx);

    for ( const auto& a : sequence ) {
        QString ab = abbrev2call(a).simplified();
        std::string stdstr_a = a.simplified().toStdString();
        const char * cstra = stdstr_a.c_str();
        std::string stdstr_ab = ab.toStdString();
        const char * cstrb = stdstr_ab.c_str();
        cout << cstrb << " (" << cstra << ")" << endl;
    }
}
