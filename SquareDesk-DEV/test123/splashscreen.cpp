// // SplashScreen.cpp
// #include "splashscreen.h"
// #include <QApplication>
// #include <QPixmap>
// #include <QLabel>
// #include <QVBoxLayout>
// #include <QProgressBar>
// #include <QScreen>
// #include <QGuiApplication>
// #include <QDebug>

// SplashScreen::SplashScreen(const QString& version, QWidget *parent) : QWidget(parent) {
//     // Load and check the image
//     QPixmap pixmap(":/graphics/SplashScreen2025b.png");
//     if (pixmap.isNull()) {
//         qDebug() << "Failed to load splash image!";
//         pixmap = QPixmap(400, 200);
//         pixmap.fill(Qt::blue);
//     }
//     // qDebug() << "Pixmap size:" << pixmap.size();

//     // Set the window size to match the image
//     setFixedSize(pixmap.size() + QSize(0,30));

//     // Create the image label to fill the entire window
//     imageLabel = new QLabel(this);
//     imageLabel->setPixmap(pixmap);
//     imageLabel->setGeometry(0, 0, pixmap.width(), pixmap.height());

//     // Create version label
//     versionLabel = new QLabel(version, this);
//     versionLabel->setStyleSheet(
//         "QLabel {"
//         "    color: black;"
//         "    font-size: 14px;"
//         "    font-weight: bold;"
//         "    background-color: rgba(255, 255, 255, 0);"
//         "    padding: 4px 8px;"
//         "    border-radius: 3px;"
//         "}"
//         );
//     versionLabel->adjustSize();
//     // Position version label on the right side, midway down
//     versionLabel->move(width() - versionLabel->width() - 10, height() / 2);

//     // Create a container widget for status and progress bar
//     QWidget* overlayWidget = new QWidget(this);
//     overlayWidget->setStyleSheet("background-color: rgba(0, 0, 0, 0);"); // semi-transparent background

//     // Create layout for the overlay
//     QVBoxLayout* overlayLayout = new QVBoxLayout(overlayWidget);
//     overlayLayout->setContentsMargins(15, 1, 15, 12);
//     overlayLayout->setSpacing(0);

//     // Add status text
//     statusLabel = new QLabel("Loading...", overlayWidget);
//     statusLabel->setStyleSheet("color: #009B57; font-weight: bold;");
//     statusLabel->setAlignment(Qt::AlignCenter);
//     overlayLayout->addWidget(statusLabel);

//     // Create and setup progress bar
//     progressBar = new QProgressBar(overlayWidget);
//     progressBar->setRange(0, 100);
//     progressBar->setTextVisible(true);
//     progressBar->setStyleSheet(
//         "QProgressBar {"
//         "   border: 1px solid white;"
//         "   border-radius: 3px;"
//         "   text-align: center;"
//         "   color: white;"
//         "   background-color: rgba(0, 0, 0, 80);"
//         "}"
//         "QProgressBar::chunk {"
//         "   background-color: #009B57;"
//         "}"
//         );
//     progressBar->setFixedHeight(18);
//     overlayLayout->addWidget(progressBar);

//     // Position the overlay at the bottom of the image
//     int overlayHeight = 60; // Adjust this value to control overlay height
//     overlayWidget->setGeometry(0, height() - overlayHeight, width(), overlayHeight);

//     // Set window flags
//     setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

//     // Center on screen
//     QScreen* screen = QGuiApplication::primaryScreen();
//     move(screen->geometry().center() - frameGeometry().center());

//     // qDebug() << "Window size:" << size();
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
#include <QPainter>
#include <QPainterPath>

QPixmap SplashScreen::roundedPixmap(const QPixmap& input, int radius) {
    QPixmap rounded(input.size());
    rounded.fill(Qt::transparent); // Important for transparency

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rounded.rect(), radius, radius);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, input);

    return rounded;
}

void SplashScreen::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Create a rounded rect path for the window shape
    QPainterPath path;
    path.addRoundedRect(rect(), 10, 10);

    // Set the window mask for the rounded corners
    setMask(QRegion(path.toFillPolygon().toPolygon()));
}

SplashScreen::SplashScreen(const QString& version, QWidget *parent) : QWidget(parent) {
    // Enable transparency
    setAttribute(Qt::WA_TranslucentBackground);

    // Load and check the image
    QPixmap pixmap(":/graphics/SplashScreen2025c.png");
    if (pixmap.isNull()) {
        qDebug() << "Failed to load splash image!";
        pixmap = QPixmap(400, 200);
        pixmap.fill(Qt::blue);
    }
    // qDebug() << "Pixmap size:" << pixmap.size();

    // Set the window size to match the image
    setFixedSize(pixmap.size());

    // Create the image label to fill the entire window
    imageLabel = new QLabel(this);
    // Round the corners of the image
    imageLabel->setPixmap(roundedPixmap(pixmap, 10));
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
    versionLabel->move(width() - versionLabel->width() - 10, height() / 2);

    // Create a container widget for status and progress bar
    QWidget* overlayWidget = new QWidget(this);
    overlayWidget->setStyleSheet(
        "QWidget {"
        "   background-color: rgba(0, 0, 0, 0);"
        "   border-bottom-left-radius: 10px;"  // Match the window corner radius
        "   border-bottom-right-radius: 10px;"
        "}"
        );

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
    int overlayHeight = 60;
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
