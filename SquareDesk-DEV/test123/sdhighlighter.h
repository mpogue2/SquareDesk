#ifndef SDHIGHLIGHTER_H
#define SDHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include "common.h"

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text) Q_DECL_OVERRIDE;

private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat couple1Format;
    QTextCharFormat couple2Format;
    QTextCharFormat couple3Format;
    QTextCharFormat couple4Format;

};

#endif // SDHIGHLIGHTER_H
