#ifndef UPDATEID3TAGSDIALOG_H
#define UPDATEID3TAGSDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QDebug>

namespace Ui
{
class MainWindow;
}

class MainWindow;

namespace Ui
{
class updateID3TagsDialog;
}

class updateID3TagsDialog : public QDialog
{
    Q_OBJECT

public:
    updateID3TagsDialog(QWidget *parent = nullptr); // 0);
    ~updateID3TagsDialog();

    Ui::updateID3TagsDialog *ui;

    double currentBPM;
    double currentTBPM;
    int    newTBPM;
    bool newTBPMValid;

    uint32_t currentLoopStart;
    uint32_t newLoopStart;
    bool newLoopStartValid;

    uint32_t currentLoopLength;
    uint32_t newLoopLength;
    bool newLoopLengthValid;

private slots:
    void on_newTBPMEditBox_textChanged(const QString &arg1);

    void on_newLoopStartEditBox_textChanged(const QString &arg1);

    void on_newLoopLengthEditBox_textChanged(const QString &arg1);

private:
    MainWindow *mw;  // so we can access MainWindow members directly
};

#endif // UPDATEID3TAGSDIALOG_H
