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
    explicit SplashScreen(const QString& version = "", QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

public slots:
    void setProgress(int value, const QString& message);

private:
    QProgressBar* progressBar;
    QLabel* statusLabel;
    QLabel* versionLabel;
    QLabel* imageLabel;
    QPixmap roundedPixmap(const QPixmap& input, int radius);
};

#endif // SPLASHSCREEN_H
