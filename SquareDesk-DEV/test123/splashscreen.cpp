// SplashScreen.cpp
#include "splashscreen.h"
#include <QApplication>
#include <QPixmap>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QScreen>
#include <QGuiApplication>
#include <QStyleOption>
#include <QPainter>

SplashScreen::SplashScreen(const QString& version, QWidget *parent) : QWidget(parent) {
    // Create the main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);  // Remove margins

    // Create a container for the image and version label
    QWidget* imageContainer = new QWidget(this);
    imageContainer->setLayout(new QHBoxLayout());
    imageContainer->layout()->setContentsMargins(0, 0, 0, 0);

    // Add the image label
    QPixmap pixmap(":/graphics/SplashScreen2025.png");
    imageLabel = new QLabel(this);
    imageLabel->setPixmap(pixmap);
    imageLabel->setFixedSize(pixmap.size());
    imageContainer->setFixedSize(pixmap.size());

    // Create version label
    versionLabel = new QLabel(version, imageContainer);
    versionLabel->setStyleSheet(
        "QLabel {"
        "    color: #333333;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "    background-color: rgba(255, 255, 255, 0);"
        "    padding: 2px 6px;"
        "    border-radius: 3px;"
        "}"
        );

    // Add both labels to the container
    imageContainer->layout()->addWidget(imageLabel);

    // Position version label using absolute coordinates
    versionLabel->adjustSize();  // Ensure the label size is calculated
    int xPos = pixmap.width() - versionLabel->width() - 10;
    int yPos = (pixmap.height() / 2) - (versionLabel->height() / 2);
    versionLabel->move(xPos, yPos);
    versionLabel->raise();  // Ensure it's on top of the image

    mainLayout->addWidget(imageContainer);

    // Add status text
    statusLabel = new QLabel("", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);

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
        "   background-color: #05B8CC;"
        "   width: 20px;"
        "}"
        );
    mainLayout->addWidget(progressBar);

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
