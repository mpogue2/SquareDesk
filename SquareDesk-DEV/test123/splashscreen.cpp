// // SplashScreen.cpp
// #include "splashscreen.h"
// #include <QApplication>
// #include <QPixmap>
// #include <QLabel>
// #include <QVBoxLayout>
// #include <QHBoxLayout>
// #include <QProgressBar>
// #include <QScreen>
// #include <QGuiApplication>
// #include <QStyleOption>
// #include <QPainter>

// SplashScreen::SplashScreen(const QString& version, QWidget *parent) : QWidget(parent) {
//     // Create the main layout
//     QVBoxLayout* mainLayout = new QVBoxLayout(this);
//     mainLayout->setContentsMargins(0, 0, 0, 0);  // Remove margins

//     // Create a container for the image and version label
//     QWidget* imageContainer = new QWidget(this);
//     imageContainer->setLayout(new QHBoxLayout());
//     imageContainer->layout()->setContentsMargins(0, 0, 0, 0);

//     // Add the image label
//     QPixmap pixmap(":/graphics/SplashScreen2025.png");
//     imageLabel = new QLabel(this);
//     imageLabel->setPixmap(pixmap);
//     imageLabel->setFixedSize(pixmap.size());
//     imageContainer->setFixedSize(pixmap.size());

//     // Create version label
//     versionLabel = new QLabel(version, imageContainer);
//     versionLabel->setStyleSheet(
//         "QLabel {"
//         "    color: #333333;"
//         "    font-size: 12px;"
//         "    font-weight: bold;"
//         "    background-color: rgba(255, 255, 255, 0);"
//         "    padding: 2px 6px;"
//         "    border-radius: 3px;"
//         "}"
//         );

//     // Add both labels to the container
//     imageContainer->layout()->addWidget(imageLabel);

//     // Position version label using absolute coordinates
//     versionLabel->adjustSize();  // Ensure the label size is calculated
//     int xPos = pixmap.width() - versionLabel->width() - 10;
//     int yPos = (pixmap.height() / 2) - (versionLabel->height() / 2);
//     versionLabel->move(xPos, yPos);
//     versionLabel->raise();  // Ensure it's on top of the image

//     mainLayout->addWidget(imageContainer);

//     // Add status text
//     statusLabel = new QLabel("", this);
//     statusLabel->setAlignment(Qt::AlignCenter); // #05B8CC
//     statusLabel->setStyleSheet(
//         "QLabel {"
//         "    color: #00AB57;"
//         "    font-size: 15px;"
//         "    font-weight: bold;"
//         "}"
//         );
//     mainLayout->addWidget(statusLabel);

//     // Create and setup progress bar
//     progressBar = new QProgressBar(this);
//     progressBar->setRange(0, 100);
//     progressBar->setTextVisible(true);
//     progressBar->setStyleSheet(
//         "QProgressBar {"
//         "   border: 2px solid grey;"
//         "   border-radius: 5px;"
//         "   text-align: center;"
//         "}"
//         "QProgressBar::chunk {"
//         "   background-color: #00AB57;"
//         "   width: 20px;"
//         "}"
//         );
//     mainLayout->addWidget(progressBar);

//     // Set window flags
//     setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
//     setAttribute(Qt::WA_TranslucentBackground);

//     // Center on screen
//     QScreen* screen = QGuiApplication::primaryScreen();
//     move(screen->geometry().center() - frameGeometry().center());
// }

// void SplashScreen::setProgress(int value, const QString& message) {
//     progressBar->setValue(value);
//     statusLabel->setText(message);
//     QApplication::processEvents();
// }


// SplashScreen.cpp
#include "splashscreen.h"
#include <QApplication>
#include <QPixmap>
#include <QLabel>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>

SplashScreen::SplashScreen(const QString& version, QWidget *parent) : QWidget(parent) {
    // Load and check the image
    QPixmap pixmap(":/graphics/SplashScreen2025b.png");
    if (pixmap.isNull()) {
        qDebug() << "Failed to load splash image!";
        pixmap = QPixmap(400, 200);
        pixmap.fill(Qt::blue);
    }
    // qDebug() << "Pixmap size:" << pixmap.size();

    // Set the window size to match the image
    setFixedSize(pixmap.size() + QSize(0,30));

    // Create the image label to fill the entire window
    imageLabel = new QLabel(this);
    imageLabel->setPixmap(pixmap);
    imageLabel->setGeometry(0, 0, pixmap.width(), pixmap.height());

    // Create version label
    versionLabel = new QLabel(version, this);
    versionLabel->setStyleSheet(
        "QLabel {"
        "    color: black;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "    background-color: rgba(255, 255, 255, 0);"
        "    padding: 4px 8px;"
        "    border-radius: 3px;"
        "}"
        );
    versionLabel->adjustSize();
    // Position version label on the right side, midway down
    versionLabel->move(width() - versionLabel->width() - 10, height() / 2);

    // Create a container widget for status and progress bar
    QWidget* overlayWidget = new QWidget(this);
    overlayWidget->setStyleSheet("background-color: rgba(0, 0, 0, 0);"); // semi-transparent background

    // Create layout for the overlay
    QVBoxLayout* overlayLayout = new QVBoxLayout(overlayWidget);
    overlayLayout->setContentsMargins(15, 1, 15, 12);
    overlayLayout->setSpacing(0);

    // Add status text
    statusLabel = new QLabel("Loading...", overlayWidget);
    statusLabel->setStyleSheet("color: #009B57; font-weight: bold;");
    statusLabel->setAlignment(Qt::AlignCenter);
    overlayLayout->addWidget(statusLabel);

    // Create and setup progress bar
    progressBar = new QProgressBar(overlayWidget);
    progressBar->setRange(0, 100);
    progressBar->setTextVisible(true);
    progressBar->setStyleSheet(
        "QProgressBar {"
        "   border: 1px solid white;"
        "   border-radius: 3px;"
        "   text-align: center;"
        "   color: white;"
        "   background-color: rgba(0, 0, 0, 80);"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #009B57;"
        "}"
        );
    progressBar->setFixedHeight(18);
    overlayLayout->addWidget(progressBar);

    // Position the overlay at the bottom of the image
    int overlayHeight = 60; // Adjust this value to control overlay height
    overlayWidget->setGeometry(0, height() - overlayHeight, width(), overlayHeight);

    // Set window flags
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    // Center on screen
    QScreen* screen = QGuiApplication::primaryScreen();
    move(screen->geometry().center() - frameGeometry().center());

    // qDebug() << "Window size:" << size();
}

void SplashScreen::setProgress(int value, const QString& message) {
    progressBar->setValue(value);
    statusLabel->setText(message);
    QApplication::processEvents();
}
