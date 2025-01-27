// SplashScreen.cpp

#include "splashscreen.h"
#include <QApplication>
#include <QPixmap>
#include <QLabel>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QScreen>
#include <QGuiApplication>
#include <QStyleOption>
#include <QPainter>

SplashScreen::SplashScreen(QWidget *parent) : QWidget(parent) {
    // Create the layout
    QVBoxLayout* layout = new QVBoxLayout(this);

    // Add the image label
    QPixmap pixmap(":/graphics/SplashScreen2025.png");
    QLabel* imageLabel = new QLabel(this);
    imageLabel->setPixmap(pixmap);
    layout->addWidget(imageLabel);

    // Add status text
    statusLabel = new QLabel("Loading...", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(statusLabel);

    // Create and setup progress bar
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setTextVisible(true);
    progressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 2px solid grey;"
        "   border-radius: 5px;"
        "   text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #42bd48;"
        "   width: 20px;"
        "}"
        );
    layout->addWidget(progressBar);

    // Set window flags
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // Center on screen
    QScreen* screen = QGuiApplication::primaryScreen();
    move(screen->geometry().center() - frameGeometry().center());
}

void SplashScreen::setProgress(int value, const QString& message) {
    progressBar->setValue(value);
    statusLabel->setText(message);
    QApplication::processEvents();
}
