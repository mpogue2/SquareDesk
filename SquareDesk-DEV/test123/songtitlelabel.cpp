#include "mainwindow.h"
#include "songtitlelabel.h"

void SongTitleLabel::mouseDoubleClickEvent(QMouseEvent *e)
{
    mw->titleLabelDoubleClicked(e);
}
