#ifndef MYTEXTEDIT_H
#define MYTEXTEDIT_H

#include <QTextEdit>
#include <QMimeData>
#include <QImage>
#include <QTextCursor>
#include <QTextDocument>
#include <QUrl>
#include <QImageReader>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QKeyEvent>

class myTextEdit : public QTextEdit {

public:
    explicit myTextEdit(QWidget *parent = nullptr) : QTextEdit(parent) {}
    void smartPaste();  // Public method for Smart Paste (GitHub issue #1552)

protected:
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    // Smart Paste helpers for GitHub issue #1552
    QString performSmartPaste(const QString &clipboardText);
    int findNearestFigureNumber();
    QString transformHeadsSides(const QString &text, int figureNumber, bool isHead);
    bool firstLineContainsHeadOrSides(const QString &text, bool &isHead);
};

#endif // MYTEXTEDIT_H
