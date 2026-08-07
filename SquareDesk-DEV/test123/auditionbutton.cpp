#include "auditionbutton.h"

auditionButton::auditionButton(QWidget *parent)
    : QPushButton(parent)   // parent was previously accepted and then thrown away (Issue #1687)
{
    origPath = "NOT SET YET";
}
