#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    QSettings MySettings;
    musicPath = MySettings.value("musicPath").toString();
    ui->musicPath->setText(musicPath);
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::on_chooseMusicPathButton_clicked()
{
    QString dir =
        QFileDialog::getExistingDirectory(this, tr("Select Base Directory for Music"),
                                                 QDir::homePath(),
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    if (dir.isNull()) {
//        qDebug() << "User cancelled.";
        return;  // user cancelled the "Select Base Directory for Music" dialog...so don't do anything, just return
    }

//    qDebug() << "User selected directory: " << dir;
    ui->musicPath->setText(dir);
    musicPath = dir;

}
