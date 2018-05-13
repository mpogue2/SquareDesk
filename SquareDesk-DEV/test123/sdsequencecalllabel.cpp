#include "mainwindow.h"
#include "sdsequencecalllabel.h"

void SDSequenceCallLabel::mouseDoubleClickEvent(QMouseEvent *e)
{
    mw->sdSequenceCallLabelDoubleClicked(e);
}
