#include "clickablelabel.h"

clickableLabel::clickableLabel(QWidget* parent)
    : QLabel(parent)
{
}

clickableLabel::clickableLabel(const QString& text, QWidget* parent)
    : QLabel(parent)
{
    setText(text);
}

clickableLabel::~clickableLabel()
{
}

void clickableLabel::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    emit clicked();
}
