#include "globaldefines.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"
#include "utility.h"

#ifdef DEBUG_LIGHT_MODE

void MainWindow::themesFileModified()
{
    // Q_UNUSED(s)
    // qDebug() << "***** FILEWATCHER: Themes.qss has been modified...";

    qApp->setStyleSheet("");  // reset it to NOTHING before setting it again

    QString app_path = qApp->applicationDirPath();
    QFile lightModeStyleSheet(app_path + "/../Resources/Themes.qss");

    lightModeStyleSheet.open(QIODevice::ReadOnly);
    QString lightStyle( lightModeStyleSheet.readAll() );
    qApp->setStyleSheet(lightStyle);
    // qDebug() << "DONE.";
}

// helper function for BOOLEAN properties ------------
void MainWindow::setProp(QWidget *theWidget, const char *thePropertyName, bool b) {
    theWidget->setProperty(thePropertyName, b);
    theWidget->style()->unpolish(theWidget);
    theWidget->style()->polish(theWidget);
}

// helper function for STRING properties ------------
void MainWindow::setProp(QWidget *theWidget, const char *thePropertyName, QString s) {
    theWidget->setProperty(thePropertyName, s);
    theWidget->style()->unpolish(theWidget);
    theWidget->style()->polish(theWidget);
}

void MainWindow::svgClockStateChanged(QString newStateName) {
    if (newStateName != currentAnalogClockState) {
        currentAnalogClockState = newStateName;
        // qDebug() << "*** svgClockStateChanged to something new:" << newStateName;
        setProp(ui->darkWarningLabel,     "state", newStateName);
        setProp(ui->warningLabelCuesheet, "state", newStateName);
        setProp(ui->warningLabelSD, "state", newStateName);
    }
}

// // HELPER FUNCTIONS for setting properties on all widgets
// //   the qss can then use darkmode and !darkmode
// void setDynamicPropertyRecursive(QWidget* widget, const QString& propertyName, const QVariant& value) {
//     if (widget) {
//         widget->setProperty(propertyName.toStdString().c_str(), value);
//         widget->style()->unpolish(widget);
//         widget->style()->polish(widget);
//         const QList<QObject*> children = widget->children();
//         for (QObject* child : children) {
//             if (QWidget* childWidget = qobject_cast<QWidget*>(child)) {
//                 setDynamicPropertyRecursive(childWidget, propertyName, value);
//             }
//         }
//     }
// }

// void setDynamicPropertyOnAllWidgets(const QString& propertyName, const QVariant& value) {
//     const QList<QWidget*> topLevelWidgets = QApplication::topLevelWidgets();
//     for (QWidget* widget : topLevelWidgets) {
//         setDynamicPropertyRecursive(widget, propertyName, value);
//     }
// }

void MainWindow::themeTriggered(QAction * action) {
    // qDebug() << "***** themeTriggered()" << action << action->isChecked() << action->text();
    action->setChecked(true);  // check this one (the new one)

    QApplication::setOverrideCursor(Qt::WaitCursor); // tell user this will be a while...

    prefsManager.SetactiveTheme(action->text()); // currently just {Light, Dark}
    // qDebug() << "activeTheme now set to: " << prefsManager.GetactiveTheme();

    setDynamicPropertyOnAllWidgets("theme", action->text()); // use this info in the QSS
    ui->darkSeekBar->updateBgPixmap((float*)1, 1);           // update the cached bg pixmap, too
    ui->theSVGClock->finishInit();                           // update the tick colors on the clock too

    currentThemeString = action->text(); // remember for popups
    ui->theSVGClock->setTheme(action->text()); // tell the clock, too (it does not have access to mw)

    ui->darkTrebleKnob->reinit();  // update the cached files
    ui->darkMidKnob->reinit();  // update the cached files
    ui->darkBassKnob->reinit();  // update the cached files

    ui->darkPitchSlider->reinit(); // update
    ui->darkTempoSlider->reinit();
    ui->darkVolumeSlider->reinit();

    // and update the colors in the darkSongTable, because they MIGHT have changed
    //  well, they probably did.
    // set initial colors for text in songTable, also used for shading the clock
    patterColorString = prefsManager.GetpatterColorString();
    singingColorString = prefsManager.GetsingingColorString();
    calledColorString = prefsManager.GetcalledColorString();
    extrasColorString = prefsManager.GetextrasColorString();

    if (!doNotCallDarkLoadMusicList) {    // I hate this.
        darkLoadMusicList(nullptr, currentTypeFilter, true, true); // refresh whatever is there
        reloadPaletteSlots();
    }

    QApplication::restoreOverrideCursor(); // back to normal cursor...
}

#endif
