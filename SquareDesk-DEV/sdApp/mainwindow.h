#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QTextEdit>
#include "console.h"
#include "sdhighlighter.h"
#include "renderarea.h"
#include "common.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void writeData(const QByteArray &data);
    void readData();

private:
    Ui::MainWindow *ui;
    Console *console;
    QProcess *sd;  // sd process
    Highlighter *highlighter;
    RenderArea *renderArea;
    QTextEdit *currentSequenceWidget;

    QString uneditedData;
    QString editedData;

};

#endif // MAINWINDOW_H
