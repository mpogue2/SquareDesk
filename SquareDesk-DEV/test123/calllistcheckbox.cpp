#include "calllistcheckbox.h"
#include "mainwindow.h"

void CallListCheckBox::checkBoxStateChanged(int state)
{
    if (mainWindow && (row >= 0))
    {
        mainWindow->tableWidgetCallList_checkboxStateChanged(row, state);
    }
};
