#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QSettings>

#include "common_enums.h"

namespace Ui
{
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();

    QString musicPath;
    void setFontSizes();

    void setColorSwatches(QString patter, QString singing, QString called, QString extras);
    QColor patterColor;
    QColor singingColor;
//    QColor calledColor;
//    QString calledColor;
    QColor extrasColor;

    void setDefaultColors(QString patter, QString singing, QString called, QString extras);
    QString defaultPatterColor, defaultSingingColor, defaultCalledColor, defaultExtrasColor;

/* See the large comment at the top of prefs_options.h */

#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_STRING(control, name, default) QString Get##name(); void Set##name(QString value);
#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) bool Get##name(); void Set##name(bool value);
#define CONFIG_ATTRIBUTE_COMBO(control, name, default) int Get##name(); void Set##name(int value);
#define CONFIG_ATTRIBUTE_COLOR(control, name, default) QString Get##name(); void Set##name(QString value);
    #include "prefs_options.h"
#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR
#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS


private slots:
    void on_chooseMusicPathButton_clicked();

    void on_calledColorButton_clicked();
    void on_extrasColorButton_clicked();
    void on_patterColorButton_clicked();
    void on_singingColorButton_clicked();

private:
    Ui::PreferencesDialog *ui;
};

#endif // PREFERENCESDIALOG_H
