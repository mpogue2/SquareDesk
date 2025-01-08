/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usage
** For commercial licensing terms and conditions, contact the authors via the
** email address above.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appear in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.
**
** $SQUAREDESK_END_LICENSE$
**
****************************************************************************/

// **********
// THIS IS EXPERIMENTAL CHOREO MGMT STUFF, THAT REQUIRES "EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT"
//   TO BE DEFINED.
// **********

// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "utility.h"
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
#include <QCoreApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QClipboard>
#include <QtSvg/QSvgGenerator>
#include <algorithm>  // for random_shuffle

QString processSequence(QString sequence,
                        const QStringList &include,
                        const QStringList &exclude)
{
    static QRegularExpression regexEmpty("^[\\s\\n]*$");
    QRegularExpressionMatch match = regexEmpty.match(sequence);
    if (match.hasMatch())
    {
        return QString();
    }

    for (int i = 0; i < exclude.length(); ++i)
    {
        if (sequence.contains(exclude[i], Qt::CaseInsensitive))
        {
            return QString();
        }
    }
    for (int i = 0; i < include.length(); ++i)
    {
        if (!sequence.contains(include[i], Qt::CaseInsensitive))
        {
            return QString();
        }
    }

    return sequence.trimmed();
}

void extractSequencesFromFile(QStringList &sequences,
                                 const QString &filename,
                                 const QString &program,
                                 const QStringList &include,
                                 const QStringList &exclude)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&file);
    bool isSDFile(false);
    bool firstSDLine(false);
    QString thisProgram = "";
    QString title(program);

    if (filename.contains(program, Qt::CaseInsensitive))
    {
        thisProgram = program;
    }

    // Sun Jan 10 17:03:38 2016     Sd38.58:db38.58     Plus
    static QRegularExpression regexIsSDFile("^(Mon|Tue|Wed|Thur|Fri|Sat|Sun)\\s+" // Sun
                                           "(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\\s+" // Jan
                                           "\\d+\\s+\\d+\\:\\d+\\:\\d+\\s+\\d\\d\\d\\d\\s+" // 10 17:03:38 2016
                                           "Sd\\d+\\.\\d+\\:db\\d+\\.\\d+\\s+" //Sd38.58:db38.58
                                           "(\\w+)\\s*$"); // Plus

    QString sequence;

    while (!in.atEnd())
    {
        QString line(in.readLine());

        QRegularExpressionMatch match = regexIsSDFile.match(line);

        if (match.hasMatch())
        {
            if (0 == thisProgram.compare(program, Qt::CaseInsensitive))
            {
                sequences << processSequence(sequence, include, exclude);
            }
            isSDFile = true;
            firstSDLine = true;
            thisProgram = match.captured(3);
            sequence.clear();
            title.clear();
        }
        else if (!isSDFile)
        {
            QString line_simplified = line.simplified();
            if (line_simplified.startsWith("Basic", Qt::CaseInsensitive))
            {
                if (0 == thisProgram.compare(title, program, Qt::CaseInsensitive))
                {
                    sequences << processSequence(sequence, include, exclude);
                }
                thisProgram = "Basic";
                sequence.clear();
            }
            else if (line_simplified.startsWith("+", Qt::CaseInsensitive)
                || line_simplified.startsWith("Plus", Qt::CaseInsensitive))
            {
                if (0 == thisProgram.compare(title, program, Qt::CaseInsensitive))
                {
                    sequences << processSequence(sequence, include, exclude);
                }
                thisProgram = "Plus";
                sequence.clear();
            }
            else if (line_simplified.length() == 0)
            {
                if (0 == thisProgram.compare(title, program, Qt::CaseInsensitive))
                {
                    sequences << processSequence(sequence, include, exclude);
                }
                sequence.clear();
            }
            else
            {
                sequence += line + "\n";
            }

        }
        else // is SD file
        {
            QString line_simplified = line.simplified();

            if (firstSDLine)
            {
                if (line_simplified.length() == 0)
                {
                    firstSDLine = false;
                }
                else
                {
                    title += line;
                }
            }
            else
            {
                if (!(line_simplified.length() == 0))
                {
                    sequence += line + "\n";
                }
            }
        }
    }
    sequences << processSequence(sequence, include, exclude);
}


// =========================
void MainWindow::filterChoreography()
{
//    QStringList exclude(getUncheckedItemsFromCurrentCallList());
//    QString program = ui->comboBoxCallListProgram->currentText();

#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    QStringList include = ui->lineEditChoreographySearch->text().split(",");
    for (int i = 0; i < include.length(); ++i)
    {
        include[i] = include[i].simplified();
    }

    if (ui->comboBoxChoreographySearchType->currentIndex() == 0)
    {
        exclude.clear();
    }

    QStringList sequences;

    for (int i = 0; i < ui->listWidgetChoreographyFiles->count()
             && sequences.length() < 128000; ++i)
    {
        QListWidgetItem *item = ui->listWidgetChoreographyFiles->item(i);
        if (item->checkState() == Qt::Checked)
        {
            QString filename = item->data(1).toString();
            extractSequencesFromFile(sequences, filename, program,
                                     include, exclude);
        }
    }

    ui->listWidgetChoreographySequences->clear();
    for (auto sequence : sequences)
    {
        if (!sequence.isEmpty())
        {
            QListWidgetItem *item = new QListWidgetItem(sequence);
            ui->listWidgetChoreographySequences->addItem(item);
        }
    }
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
}

#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
void MainWindow::on_listWidgetChoreographySequences_itemDoubleClicked(QListWidgetItem * /* item */)
{
    QListWidgetItem *choreoItem = new QListWidgetItem(item->text());
    ui->listWidgetChoreography->addItem(choreoItem);
}

void MainWindow::on_listWidgetChoreography_itemDoubleClicked(QListWidgetItem * /* item */)
{
    ui->listWidgetChoreography->takeItem(ui->listWidgetChoreography->row(item));
}


void MainWindow::on_lineEditChoreographySearch_textChanged()
{
    filterChoreography();
}

void MainWindow::on_listWidgetChoreographyFiles_itemChanged(QListWidgetItem * /* item */)
{
    filterChoreography();
}
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT

void MainWindow::loadChoreographyList()
{
#ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
    ui->listWidgetChoreographyFiles->clear();

    QListIterator<QString> iter(*pathStack);

    while (iter.hasNext()) {
        QString s = iter.next();

        if (s.endsWith(".txt", Qt::CaseInsensitive)
            && (s.contains("sequence", Qt::CaseInsensitive)
                || s.contains("singer", Qt::CaseInsensitive)))
        {
            QStringList sl1 = s.split("#!#");
            QString type = sl1[0];  // the type (of original pathname, before following aliases)
            QString origPath = sl1[1];  // everything else

            QFileInfo fi(origPath);
//            QStringList section = fi.canonicalPath().split("/");
            QString name = fi.completeBaseName();
            QListWidgetItem *item = new QListWidgetItem(name);
            item->setData(1,origPath);
            item->setCheckState(Qt::Unchecked);
            ui->listWidgetChoreographyFiles->addItem(item);
        }
    }
#endif // ifdef EXPERIMENTAL_CHOREOGRAPHY_MANAGEMENT
}

