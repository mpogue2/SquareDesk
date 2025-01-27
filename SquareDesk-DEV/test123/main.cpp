/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usage
** For commercial licensing terms and conditions, contact the authors via the
** email address above.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appear in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.
**
** $SQUAREDESK_END_LICENSE$
**
****************************************************************************/

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include <QApplication>
#include <QSplashScreen>
#include "prefsmanager.h"
#include "perftimer.h"

//#include "startupwizard.h"
//#include <execinfo.h>  // for debugging using backtrace()

// ==============================================================================================================
int main(int argc, char *argv[])
{
    PerfTimer t("main", __LINE__);
    t.start(__LINE__);

// From: https://stackoverflow.com/questions/4954140/how-to-redirect-qdebug-qwarning-qcritical-etc-output
//    QCoreApplication::addLibraryPath(".");  // for windows

#ifdef USE_JUCE
    // https://forum.juce.com/t/leaked-objects-detected-1-instance-s-of-class-timerthread/57798/5
    juce::ScopedJuceInitialiser_GUI init; // needed to prevent memory leak reporting
#endif

    t.elapsed(__LINE__);

    QApplication a(argc, argv);
    a.setApplicationName("SquareDesk");
    a.setOrganizationName("Zenstar Software");
    a.setOrganizationDomain("zenstarstudio.com");

    PreferencesManager prefsManager;

    t.elapsed(__LINE__);

//    prefsManager.SetdarkMode(false);

    bool darkmode = prefsManager.GetdarkMode();

#ifdef DEBUG_LIGHT_MODE
    darkmode = true;  // override the Light Mode preference for Dark GUI, go with the new one
    QString resourcesPath = qApp->applicationDirPath() + "/../Resources";
    QDir::addSearchPath("themes", resourcesPath); // url(images:foo.png) will look in Resources dir for foo.png

    t.elapsed(__LINE__);

#else
    if (darkmode) {
        // DARK MODE ======================================
        // from: https://gist.github.com/QuantumCD/6245215
        a.setStyle(QStyleFactory::create("Fusion"));

        QPalette newPalette;
        newPalette.setColor(QPalette::Window,          QColor( 37,  37,  37));
        newPalette.setColor(QPalette::WindowText,      QColor(212, 212, 212));
        newPalette.setColor(QPalette::Base,            QColor( 60,  60,  60));
        newPalette.setColor(QPalette::AlternateBase,   QColor( 45,  45,  45));
        newPalette.setColor(QPalette::PlaceholderText, QColor(127, 127, 127));
        newPalette.setColor(QPalette::Text,            QColor(212, 212, 212));
        newPalette.setColor(QPalette::Button,          QColor( 45,  45,  45));
        newPalette.setColor(QPalette::ButtonText,      QColor(212, 212, 212));
        newPalette.setColor(QPalette::BrightText,      QColor(240, 240, 240));
        newPalette.setColor(QPalette::Highlight,       QColor( 38,  79, 120));
        newPalette.setColor(QPalette::HighlightedText, QColor(240, 240, 240));

        newPalette.setColor(QPalette::Light,           QColor( 60,  60,  60));
        newPalette.setColor(QPalette::Midlight,        QColor( 52,  52,  52));
        newPalette.setColor(QPalette::Dark,            QColor( 30,  30,  30) );
        newPalette.setColor(QPalette::Mid,             QColor( 37,  37,  37));
        newPalette.setColor(QPalette::Shadow,          QColor( 0,    0,   0));

        newPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));

        a.setPalette(newPalette);

        a.setStyleSheet("QToolTip{border: 1px solid orange; padding: 2px; border-radius: 3px; opacity: 200; background-color:#121113; color:#ABA7AC; }");
    }
#endif

    // splash screen ------
    QPixmap pixmap(":/graphics/SplashScreen2025.png");

    //  draw the version number into the splashscreen pixmap
    QPainter painter( &pixmap );
    painter.setFont( QFont("Arial") );
    painter.drawText( QPoint(225, 140), QString("V") + VERSIONSTRING );

    t.elapsed(__LINE__);

    QSplashScreen splash(pixmap);
    splash.show();

    // Force the splash screen to be shown immediately
    a.processEvents();

    t.elapsed(__LINE__);

    // If running from QtCreator, use normal debugging -------------
    QByteArray envVar = qgetenv("QTDIR");       //  check if the app is ran in Qt Creator

    if (envVar.isEmpty()) {
        // if running as a standalone app, log to a file instead of the console
        qInstallMessageHandler(MainWindow::customMessageOutput); // custom message handler for debugging
    } else {
        // if running inside QtCreator, suppress some of the messages going to the Application Output pane
        qDebug() << "***** NOTE: some Application Output warnings suppressed by MainWindow::customMessageOutputQt() *****";
        qInstallMessageHandler(MainWindow::customMessageOutputQt); // custom message handler for debugging inside QtCreator
    }

   a.processEvents();

    t.elapsed(__LINE__);

    MainWindow w(&splash, darkmode);  // setMessage() will be called several times in here while loading...
   a.processEvents();  // force events to be processed, before closing the window

    t.elapsed(__LINE__);

    splash.finish(&w); // tell splash screen to go away when window is up

    // put window back where it was last time (modulo the screen size, which
    //   is automatically taken care of.
    QSettings settings;
    w.restoreGeometry(settings.value("geometry").toByteArray());
    w.restoreState(settings.value("windowState").toByteArray());

    w.show();

    t.elapsed(__LINE__);

    w.setVisible(false);  // This works around a bug (not sure when introduced) whereby the first selection
    w.setVisible(true);   //   of an item in a context menu does NOT call the lambda, UNLESS the app is started, then the app focus
                          //   is changed to another non-SquareDesk window, and then back to SquareDesk.  The second selection
                          //   always seems to work, no matter what.  This change seems to put the window into a good state again,
                          //   such that the first selection of a context menu item actually calls the lambda.
                          //  ALSO NOTE: The bad behavior is never seen when running from within QtCreator, OR
                          //   when executing ./squaredesk in CLI.  It is ONLY seen when double-clicking the final application,
                          //   e.g. in /test123 .  This made it very hard to track down!
                          //  Not showing the splash screen also fixes the problem, but I wanted to keep that, so this seems like a
                          //   simple workaround.  I have no idea why this works, but it seems to solve the problem 100% of the time.
                          //  This bug still exists as of 2025.01.27, on Qt v6.8.1 .

    // http://stackoverflow.com/questions/6516299/qt-c-icons-not-showing-up-when-program-is-run-under-windows-o-s
    QString sDir = QCoreApplication::applicationDirPath();
    a.addLibraryPath(sDir + "/plugins");

    a.installEventFilter(new GlobalEventFilter(w.ui));

    t.elapsed(__LINE__);

    Q_INIT_RESOURCE(startupwizard); // resources for the startup wizard

    // this is needed, because when the splash screen happens, we don't get the usual appState change message
    //  so, SD doesn't work until you click on another window and come back.
    w.changeApplicationState(a.applicationState()); // make sure we get one event with current state

//    // DEBUG ========================================================
//    void* callstack[128];
//    int i, frames = backtrace(callstack, 128);
//    char** strs = backtrace_symbols(callstack, frames);
//    for (i = 0; i < frames; ++i) {
//        printf("%s\n", strs[i]);
//    }
//    free(strs);
//    // DEBUG ========================================================

    t.elapsed(__LINE__);

    int ret = a.exec();
    if (ret == RESTART_SQUAREDESK)
    {
        //restart application

        w.close();

        QString program = qApp->arguments()[0];
        QStringList arguments = qApp->arguments().mid(1);
//        qDebug() << "Restarting: " << program << ";" << arguments;
        QProcess::startDetached(program, arguments);

        // NOTE: This will NOT work from within QtCreator (for some unknown reason), but it does work
        //   when the executable is run normally by a user.
    }
    return ret;
}
