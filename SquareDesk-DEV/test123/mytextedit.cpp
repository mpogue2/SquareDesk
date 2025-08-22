#include "mytextedit.h"

bool myTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
    // This is NOT called during paste.  I think it might only be called
    //   when a drag and drop into the QTextEdit is encountered.
    // qDebug() << "canInsertFromMimeData" << source;
    return QTextEdit::canInsertFromMimeData(source);
}

void myTextEdit::insertFromMimeData(const QMimeData *source)
{
    // qDebug() << "insertFromMimeData" << source->formats();
    // qDebug() << "insertFromMimeData" << source->hasText() << source->text();
    // qDebug() << "insertFromMimeData" << source->hasHtml() << source->html();

    if (source->hasHtml() &&
        source->html().startsWith("<!DOCTYPE HTML PUBLIC") &&
        source->html().contains("qrichtext")) {
        // source is definitely from INSIDE SquareDesk
        // qDebug() << "insertFromMimeData INSIDE:" << source->hasHtml() << source->html();
        QTextEdit::insertFromMimeData(source); // insert normally to preserve semantic tags
    } else if (source->hasText()) {
        // source is from OUTSIDE SquareDesk, so strip it down
        QMimeData modified;
        modified.setText(source->text()); // take only the text
        // qDebug() << "insertFromMimeData OUTSIDE:" << modified.hasText() << modified.text() << modified.hasHtml() << modified.html();
        QTextEdit::insertFromMimeData(&modified);
    } else {
        // insert "normally" for now (I don't know that this ever occurs) and we get what we get...
        // qDebug() << "insertFromMimeData UNKNOWN:" << source->hasHtml() << source->html();
        QTextEdit::insertFromMimeData(source);
    }
}
