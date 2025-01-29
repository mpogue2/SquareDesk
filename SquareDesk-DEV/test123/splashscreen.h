// // SplashScreen.h
// #ifndef SPLASHSCREEN_H
// #define SPLASHSCREEN_H

// #include <QWidget>
// #include <QProgressBar>
// #include <QLabel>
// #include <QScreen>
// #include <QGuiApplication>
// #include <QVBoxLayout>

// class SplashScreen : public QWidget {
//     Q_OBJECT
// public:
//     explicit SplashScreen(const QString& version = "v1.0.0", QWidget *parent = nullptr);

// public slots:
//     void setProgress(int value, const QString& message);

// private:
//     QProgressBar* progressBar;
//     QLabel* statusLabel;
//     QLabel* versionLabel;
//     QLabel* imageLabel;
// };

// #endif // SPLASHSCREEN_H

// SplashScreen.h
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
    explicit SplashScreen(const QString& version = "v1.0.0", QWidget *parent = nullptr);

public slots:
    void setProgress(int value, const QString& message);

private:
    QProgressBar* progressBar;
    QLabel* statusLabel;
    QLabel* versionLabel;
    QLabel* imageLabel;
};

#endif // SPLASHSCREEN_H
