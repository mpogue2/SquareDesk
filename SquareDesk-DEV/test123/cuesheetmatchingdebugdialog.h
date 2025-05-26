#ifndef CUESHEETMATCHINGDEBUGDIALOG_H
#define CUESHEETMATCHINGDEBUGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QFont>

class MainWindow; // Forward declaration

class CuesheetMatchingDebugDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CuesheetMatchingDebugDialog(MainWindow *parent = nullptr);

private slots:
    void onTextChanged();

private:
    MainWindow *mainWindow;
    QLineEdit *songFilenameEdit;
    QLineEdit *cuesheetFilenameEdit;
    QTextEdit *debugOutputEdit;
    
    void setupUI();
    void connectSignals();
};

#endif // CUESHEETMATCHINGDEBUGDIALOG_H
