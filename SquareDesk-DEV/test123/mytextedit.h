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

class myTextEdit : public QTextEdit {

public:
    explicit myTextEdit(QWidget *parent = nullptr) : QTextEdit(parent) {}

protected:
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;
};

#endif // MYTEXTEDIT_H
