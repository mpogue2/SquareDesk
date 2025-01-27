#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QScreen>
#include <QGuiApplication>
#include <QVBoxLayout>

class SplashScreen : public QWidget {
    Q_OBJECT
public:
    explicit SplashScreen(QWidget *parent = nullptr);

public slots:
    void setProgress(int value, const QString& message);

private:
    QProgressBar* progressBar;
    QLabel* statusLabel;
};

#endif // SPLASHSCREEN_H
