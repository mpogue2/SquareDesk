#include "mytextedit.h"
#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>

void myTextEdit::smartPaste()
{
    // Public method to perform Smart Paste (GitHub issue #1552)
    // Can be called from context menu or other UI elements
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasHtml() &&
        mimeData->html().startsWith("<!DOCTYPE HTML PUBLIC") &&
        mimeData->html().contains("qrichtext")) {
        // Source is from INSIDE SquareDesk - preserve HTML formatting
        // Use plain text for analysis but transform the HTML
        QString plainText = mimeData->text();
        QString originalHtml = mimeData->html();

        // Check if transformation is needed based on plain text content
        bool isHead = false;
        if (!firstLineContainsHeadOrSides(plainText, isHead)) {
            // No transformation needed, just insert the HTML as-is
            QMimeData modified;
            modified.setHtml(originalHtml);
            insertFromMimeData(&modified);
            return;
        }

        // Find FIGURE number
        int figureNumber = findNearestFigureNumber();
        if (figureNumber == 0) {
            // No FIGURE found, insert as-is
            QMimeData modified;
            modified.setHtml(originalHtml);
            insertFromMimeData(&modified);
            return;
        }

        // Transform the HTML content
        QString transformedHtml = transformHeadsSides(originalHtml, figureNumber, isHead);

        // Insert the transformed HTML at cursor position
        QMimeData modified;
        modified.setHtml(transformedHtml);
        insertFromMimeData(&modified);
    } else if (mimeData->hasText()) {
        // Source is from OUTSIDE SquareDesk or plain text - transform plain text only
        QString originalText = mimeData->text();
        QString transformedText = performSmartPaste(originalText);

        // Insert the transformed text at cursor position
        QMimeData modified;
        modified.setText(transformedText);
        insertFromMimeData(&modified);
    }
}

void myTextEdit::keyPressEvent(QKeyEvent *event)
{
    // Intercept OPTION-CMD-V (ALT-CTRL-V) for Smart Paste (GitHub issue #1552)
    // Must intercept at key press level to prevent OPTION-V from producing √ character
    if ((event->modifiers() & Qt::AltModifier) &&
        (event->modifiers() & Qt::ControlModifier) &&
        event->key() == Qt::Key_V) {
        // This is OPTION-CMD-V, trigger smart paste
        smartPaste();
        event->accept();
        return;
    }

    // Default handling for all other keys
    QTextEdit::keyPressEvent(event);
}

bool myTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
    // This is NOT called during paste.  I think it might only be called
    //   when a drag and drop into the QTextEdit is encountered.
    // qDebug() << "canInsertFromMimeData" << source;
    return QTextEdit::canInsertFromMimeData(source);
}

void myTextEdit::insertFromMimeData(const QMimeData *source)
{
    // Original paste logic (Smart Paste now handled via keyPressEvent and context menu)
    // qDebug() << "insertFromMimeData" << source->formats();
    // qDebug() << "insertFromMimeData" << source->hasText() << source->text();
    // qDebug() << "insertFromMimeData" << source->hasHtml() << source->html();

    if (source->hasHtml() &&
        source->html().startsWith("<!DOCTYPE HTML PUBLIC") &&
        source->html().contains("qrichtext")) {
        // source is from INSIDE SquareDesk
        // qDebug() << "insertFromMimeData INSIDE:" << source->hasHtml() << source->html();
        QTextEdit::insertFromMimeData(source); // insert normally to preserve semantic tags
    } else if (source->hasText()) {
        // source is from OUTSIDE SquareDesk
        QMimeData modified;
        modified.setText(source->text()); // take only the text
        // qDebug() << "insertFromMimeData OUTSIDE:" << modified.hasText() << modified.text() << modified.hasHtml() << modified.html();
        QTextEdit::insertFromMimeData(&modified);
    } else {
        // insert "normally" for now
        // qDebug() << "insertFromMimeData UNKNOWN:" << source->hasHtml() << source->html();
        QTextEdit::insertFromMimeData(source);
    }
}

// Smart Paste helper functions for GitHub issue #1552 ========================================

QString myTextEdit::performSmartPaste(const QString &clipboardText)
{
    // Check if first line contains HEAD/HEADS or SIDE/SIDES
    bool isHead = false;
    if (!firstLineContainsHeadOrSides(clipboardText, isHead)) {
        // No HEAD/HEADS/SIDE/SIDES in first line, return as-is
        return clipboardText;
    }

    // Find nearest FIGURE marker above cursor
    int figureNumber = findNearestFigureNumber();

    // If no FIGURE found, return as-is (regular paste)
    if (figureNumber == 0) {
        return clipboardText;
    }

    // Perform transformation if needed
    return transformHeadsSides(clipboardText, figureNumber, isHead);
}

int myTextEdit::findNearestFigureNumber()
{
    QTextCursor cursor = textCursor();

    // Start from current cursor position
    cursor.movePosition(QTextCursor::StartOfLine);

    // Search upward line by line
    while (cursor.position() > 0) {
        // Move up one line
        if (!cursor.movePosition(QTextCursor::Up)) {
            break; // At start of document
        }

        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        QString lineText = cursor.selectedText();

        // Check for FIGURE markers (case-insensitive)
        // Pattern: "FIGURE 1", "FIGURE 2", "FIGURE 3", "FIGURE 4"
        QRegularExpression figureRegex("FIGURE\\s+([1-4])",
                                       QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = figureRegex.match(lineText);

        if (match.hasMatch()) {
            return match.captured(1).toInt();
        }
    }

    return 0; // No FIGURE found
}

QString myTextEdit::transformHeadsSides(const QString &text, int figureNumber, bool isHead)
{
    // Determine if swap should occur based on context
    bool shouldSwap = false;
    if (isHead && (figureNumber == 3 || figureNumber == 4)) {
        shouldSwap = true; // HEAD/HEADS in clipboard, FIGURE 3/4 context
    } else if (!isHead && (figureNumber == 1 || figureNumber == 2)) {
        shouldSwap = true; // SIDE/SIDES in clipboard, FIGURE 1/2 context
    }

    if (!shouldSwap) {
        return text; // No swap needed
    }

    QString result = text;

    // Perform TRUE BIDIRECTIONAL swap: HEAD ↔ SIDE and HEADS ↔ SIDES
    // This means ALL occurrences swap, regardless of which direction
    // \b = word boundary, \s = trailing space
    // Always output uppercase

    // Use placeholder strings to avoid partial replacements
    // Step 1: Replace HEAD/HEADS with temporary placeholders
    QRegularExpression headsRegex("\\bHEADS\\s",
                                  QRegularExpression::CaseInsensitiveOption);
    QRegularExpression headRegex("\\bHEAD\\s",
                                 QRegularExpression::CaseInsensitiveOption);

    result.replace(headsRegex, "__TEMP_HEADS__ ");  // Temporary placeholder
    result.replace(headRegex, "__TEMP_HEAD__ ");     // Temporary placeholder

    // Step 2: Replace SIDE/SIDES with HEAD/HEADS
    QRegularExpression sidesRegex("\\bSIDES\\s",
                                  QRegularExpression::CaseInsensitiveOption);
    QRegularExpression sideRegex("\\bSIDE\\s",
                                 QRegularExpression::CaseInsensitiveOption);

    result.replace(sidesRegex, "HEADS ");
    result.replace(sideRegex, "HEAD ");

    // Step 3: Replace placeholders with SIDE/SIDES
    result.replace("__TEMP_HEADS__ ", "SIDES ");
    result.replace("__TEMP_HEAD__ ", "SIDE ");

    return result;
}

bool myTextEdit::firstLineContainsHeadOrSides(const QString &text, bool &isHead)
{
    // Get first line
    int newlineIndex = text.indexOf('\n');
    QString firstLine = (newlineIndex == -1) ? text : text.left(newlineIndex);

    // Check for HEAD/HEADS or SIDE/SIDES (with trailing space, case-insensitive)
    // HEADS? matches both HEAD and HEADS
    QRegularExpression headRegex("\\bHEADS?\\s",
                                 QRegularExpression::CaseInsensitiveOption);
    QRegularExpression sideRegex("\\bSIDES?\\s",
                                 QRegularExpression::CaseInsensitiveOption);

    bool hasHead = headRegex.match(firstLine).hasMatch();
    bool hasSide = sideRegex.match(firstLine).hasMatch();

    if (hasHead) {
        isHead = true;
        return true;
    } else if (hasSide) {
        isHead = false;
        return true;
    }

    return false; // No HEAD/HEADS/SIDE/SIDES found in first line
}
