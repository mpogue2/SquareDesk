/****************************************************************************
**
** Copyright (C) 2016-2025 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usage
** For commercial licensing terms and conditions, contact the authors via the
** email address above.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appear in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.
**
** $SQUAREDESK_END_LICENSE$
**
****************************************************************************/

#include "svgWaveformSlider.h"
#include "math.h"
#include <QFileInfo>
// GOOD:
// https://stackoverflow.com/questions/70206279/qslider-not-displaying-transparency-properly-when-overlapped-with-qlabel-pyqt5

// this is used in a couple of places, and it's not adjustable by the user right now
#define CURRENTPOSPENWIDTH 2

// -------------------------------------
// from: https://stackoverflow.com/questions/7537632/custom-look-svg-gui-widgets-in-qt-very-bad-performance
svgWaveformSlider::svgWaveformSlider(QWidget *parent) :
    QSlider(parent)
{
    bgPixmap = nullptr;
    bg = nullptr;
    loadingMessage = nullptr;
    currentPos = nullptr;
    leftLoopMarker = nullptr;
    rightLoopMarker = nullptr;
    leftLoopCover = nullptr;
    rightLoopCover = nullptr;

    cachedWaveform = nullptr;

    // bare minimum init --------
    drawLoopPoints = false;
    singingCall = false;
    SetDefaultIntroOutroPositions(false, 0.0, false, 0.0, 0.0, 0.0);
    origin = 0;

    nowDestroying = false; // true when we're in the destructor, shutting everything down (don't want any paint events or updates)

    wholeTrackPeak = 1.0; // disable waveform scaling by default

    // install the wheel/scroll eater --------
    this->installEventFilter(this);  // eventFilter() is called, where wheel/touchpad scroll events are eaten

//    finishInit();

#ifndef DEBUG_LIGHT_MODE
    setcurrentPositionColor(QColor("green"));
    setdarkeningColor(QColor("#80000000"));
    setloopColor(QColor("#00a7f4"));
#endif
}

svgWaveformSlider::~svgWaveformSlider()
{
    if (bgPixmap)       delete bgPixmap;
    if (bg)             delete bg;
    if (loadingMessage) delete loadingMessage;
    if (currentPos)     delete currentPos;
    if (leftLoopMarker) delete leftLoopMarker;
    if (rightLoopMarker) delete rightLoopMarker;
    if (leftLoopCover)  delete leftLoopCover;
    if (rightLoopCover) delete rightLoopCover;

    // if (cachedWaveform) delete cachedWaveform; // NO, DO NOT DELETE THIS.  The float* is owned by the mainWindow, we just have a copy of the pointer.
}

// ------------------------------------
void svgWaveformSlider::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED(pe)
    // qDebug() << "paintEvent()" << pe->rect() << bgColor();
    // QSlider::paintEvent(pe);

#ifdef DEBUG_LIGHT_MODE
    QPainter p(this);
    p.fillRect(QRect(-2,-2,10,10), QColor("green")); // if you see this rect, something is wrong!
#endif
    // p.setRenderHint(QPainter::Antialiasing);
    // p.fillRect(QRect(-2,-2,10,10), QColor("#FF888888")); // fill everything with red
    // p.setPen(QColor("#888888"));
    // p.drawLine(0, 0, 100, 100);

    // QPainter p(this);
    // p.drawLine(0, 0, 100, 50);
    // p.fillRect(pe->rect(), bgColor()); // fill everything with the QSS background color
}

void svgWaveformSlider::resizeEvent(QResizeEvent *re)
{
    Q_UNUSED(re)
//    qDebug() << "RESIZE:" << re->size().width() << re->size().height();

    updateBgPixmap((float*)1, 1); // stretch/squish the bgPixmap, using cached values
}

void svgWaveformSlider::setFloatValue(float val) {
    QSlider::setValue((int)val);  // set the value of the parent class

    if (currentPos == nullptr) {
        return; // slider is not fully initialized yet
    }

    // note that position is float here, so that currentPosition can move by a smaller number of pixels at a time
    int currentPositionX = round((float)val * ((float)(width()-4)/(float)maximum()));

    currentPos->setPos(currentPositionX, 0);

    // TODO: factor this out *******************
    // Iterate over the items in the group, set their pen to the new color
    QList<QGraphicsItem*> items = currentPos->childItems();
    // qDebug() << "setFloatValue ***** Number of items in group:" << items.size() << currentPositionColor();

    for (QGraphicsItem* item : items) {
        QGraphicsLineItem* line = qgraphicsitem_cast<QGraphicsLineItem*>(item);
        if (line) {
            // qDebug() << "Line Item:" << line;
            QPen currentPosPen(currentPositionColor(), CURRENTPOSPENWIDTH);
            line->setPen(currentPosPen);
        } else {
            QGraphicsPolygonItem* poly = qgraphicsitem_cast<QGraphicsPolygonItem*>(item);
            QBrush currentPosBrush(currentPositionColor());
            poly->setBrush(currentPosBrush);
        }
    }

}

void svgWaveformSlider::setValue(int val) {
    QSlider::setValue(val);  // set the value of the parent class

    if (currentPos == nullptr) {
        return; // slider is not fully initialized yet
    }

    int currentPositionX = round((float)val * ((float)(width()-4)/(float)maximum()));

    currentPos->setPos(currentPositionX, 0);

    // TODO: factor this out *******************
    // Iterate over the items in the group, set their pen to the new color
    QList<QGraphicsItem*> items = currentPos->childItems();
    // qDebug() << "setValue ***** Number of items in group:" << items.size() << currentPositionColor();

    for (QGraphicsItem* item : items) {
        QGraphicsLineItem* line = qgraphicsitem_cast<QGraphicsLineItem*>(item);
        if (line) {
            // qDebug() << "Line Item:" << line;
            QPen currentPosPen(currentPositionColor(), CURRENTPOSPENWIDTH);
            line->setPen(currentPosPen);
        } else {
            QGraphicsPolygonItem* poly = qgraphicsitem_cast<QGraphicsPolygonItem*>(item);
            QBrush currentPosBrush(currentPositionColor());
            poly->setBrush(currentPosBrush);
        }
    }

};

// MOUSE EVENTS ============
#define XADJUST 3
void svgWaveformSlider::mousePressEvent(QMouseEvent* e) {
    double val;

    switch (e->button()) {
        case Qt::LeftButton:
            // val = this->maximum() * (fmax(0,(e->pos().rx() - 6))/(width()-4));
            val = this->maximum() * (fmax(0,(e->pos().rx()-XADJUST))/(width()-4));
            val = round(val);
            // qDebug() << "LEFT BUTTON:" << e->pos().rx() << width()-4 << this->maximum() << val;
            this->setValue(val);
            // qDebug() << "* mousePressEvent, set to:" << val;
            emit sliderMoved(val);
            break;
        case Qt::MiddleButton:
        case Qt::RightButton:
            break;
        default:
            break;
    }
}

void svgWaveformSlider::mouseMoveEvent(QMouseEvent* e) {
    // Q_UNUSED(e)
    double val = this->maximum() * (fmax(0,(e->pos().rx()-XADJUST))/(width()-4));
    val = round(val);
    this->setValue(val);
    // qDebug() << "* mouseMoveEvent, set to:" << val;
    emit sliderMoved(val);
}

void svgWaveformSlider::mouseDoubleClickEvent(QMouseEvent* e) {
    Q_UNUSED(e)
    // qDebug() << "mouseDoubleClickEvent: " << e;
}

void svgWaveformSlider::mouseReleaseEvent(QMouseEvent* e) {
    // Q_UNUSED(e)
    double val = this->maximum() * (fmax(0,(e->pos().rx()-XADJUST))/(width()-4));
    val = round(val);
    this->setValue(val);
    // qDebug() << "* mouseReleaseEvent, set to:" << val << e->pos().rx();
    emit sliderMoved(val);
}

void svgWaveformSlider::finishInit() {
    setValue(0.0);    // set to beginning
    setFixedHeight(61);

    // BACKGROUND PIXMAP --------
    bgPixmap = new QPixmap(WAVEFORMSAMPLES, 61);  // NOTE: we gotta make this bug to start, or the QSlider will never figure out that it can draw bigger.
#ifdef DEBUG_LIGHT_MODE
    bgPixmap->fill(bgColor());
    // bgPixmap->fill(QColor("#888888"));
#else
    bgPixmap->fill(QColor("#FA0000")); // RED for debugging (should not see this)
#endif
//    DEBUG: save to a file so we can look at it ----
//    bgPixmap->save("myPixmap.png");
//    scene.addPixmap(*bgPixmap);

    bg = new QGraphicsPixmapItem(*bgPixmap);

    // LOADING MESSAGE --------
    loadingMessage = new QGraphicsTextItem("Loading...");
    loadingMessage->setFont(QFont("Avenir Next", 18));
    loadingMessage->setPos(20, 5);
    loadingMessage->setVisible(false);
    loadingMessage->setDefaultTextColor(QColor("#7f7f7f")); // visible in both dark and light modes

    // CURRENT POSITION MARKER -------
#ifndef DEBUG_LIGHT_MODE
    QPen currentPosPen(QColor("#00FF33"), 1);
    QBrush currentPosBrush(QColor("#00FF33"));
#else
    QPen currentPosPen(currentPositionColor(), CURRENTPOSPENWIDTH);
    QBrush currentPosBrush(currentPositionColor());
#endif

    currentPos = new QGraphicsItemGroup();

    QGraphicsLineItem *vLine = new QGraphicsLineItem(0,0+2, 0,60-2); // initial position of the line
    vLine->setPen(currentPosPen);

    QPolygonF p;
    float d = 5.0;
    p << QPointF(-d, 0.0+1) << QPointF(d, 0.0+1) << QPointF(0.0, d+1) << QPointF(-d, 0.0+1);
    QGraphicsPolygonItem *topTri = new QGraphicsPolygonItem(p);
    topTri->setBrush(currentPosBrush);

    QPolygonF p2;
    p2 << QPointF(-d, 60.0) << QPointF(d, 60.0) << QPointF(0.0, 60.0-d) << QPointF(-d, 60.0);
    QGraphicsPolygonItem *botTri = new QGraphicsPolygonItem(p2);
    botTri->setBrush(currentPosBrush);

    currentPos->addToGroup(vLine);
    currentPos->addToGroup(topTri);
    currentPos->addToGroup(botTri);

    currentPos->setPos(100, 0);
    currentPos->setVisible(false);  // initially false, since nothing is loaded yet

    // LOOP INDICATORS -------
    QPen currentLoopPen(QColor("#26A4ED"), 2);

    // qDebug() << "finishInit() darkeningColor:" << darkeningColor();

#ifndef DEBUG_LIGHT_MODE
    QBrush loopDarkeningBrush(QColor("#80000000"));
#else
    QBrush loopDarkeningBrush(darkeningColor());
#endif

    // left --
    leftLoopMarker = new QGraphicsItemGroup();

    QGraphicsLineItem *top = new QGraphicsLineItem(0,1, 4,1);
    top->setPen(currentLoopPen);
    QGraphicsLineItem *middle = new QGraphicsLineItem(0,0, 0,61);
    middle->setPen(currentLoopPen);
    QGraphicsLineItem *bot = new QGraphicsLineItem(0,61-1, 4,61-1);
    bot->setPen(currentLoopPen);

    leftLoopMarker->addToGroup(top);
    leftLoopMarker->addToGroup(middle);
    leftLoopMarker->addToGroup(bot);

    leftLoopMarker->setPos(100,0);

    // left cover --
    leftLoopCover = new QGraphicsRectItem(99,0,100,61); // 99 is to fix a bug with extra vert line on the left when loadMP3
    leftLoopCover->setBrush(loopDarkeningBrush);
    leftLoopCover->setPen(QColor(darkeningColor()));

    // right --
    rightLoopMarker = new QGraphicsItemGroup();

    QGraphicsLineItem *topR = new QGraphicsLineItem(0,1, 4,1);
    topR->setPen(currentLoopPen);
    QGraphicsLineItem *middleR = new QGraphicsLineItem(4,0, 4,61);
    middleR->setPen(currentLoopPen);
    QGraphicsLineItem *botR = new QGraphicsLineItem(0,61-1, 4,61-1);
    botR->setPen(currentLoopPen);

    rightLoopMarker->addToGroup(topR);
    rightLoopMarker->addToGroup(middleR);
    rightLoopMarker->addToGroup(botR);

    rightLoopMarker->setPos(300,0);

    // right cover --
//    rightLoopCover = new QGraphicsRectItem(300,0,WAVEFORMWIDTH,61);
    rightLoopCover = new QGraphicsRectItem(300,0,width() - 4,61);
    rightLoopCover->setBrush(loopDarkeningBrush);

    this->setLoop(false);  // turn off to start

    // ---------------------
    view.setDisabled(true);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    scene.addItem(bg);  // on BOTTOM
    scene.addItem(loadingMessage);
    scene.addItem(leftLoopCover);
    scene.addItem(leftLoopMarker);
    scene.addItem(rightLoopCover);
    scene.addItem(rightLoopMarker);
    scene.addItem(currentPos);  // on TOP

    view.setScene(&scene);
    view.setParent(this);
}

bool svgWaveformSlider::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj)
//    qDebug() << "obj: " << obj << ", event: " << event->type();

    if (event->type() == QEvent::Wheel || event->type() == QEvent::Scroll) {
        // eat wheel and scroll events
        return true;
    }
    return false;  // don't stop anything else
    }

void svgWaveformSlider::SetDefaultIntroOutroPositions(bool tempoIsBPM, double estimatedBPM,
                                             bool songIsSingerType,
                                             double songStart_sec, double songEnd_sec, double songLength_sec)
{
    double loopStart_frac;
    double loopEnd_frac;

    if (!tempoIsBPM) {
        // if we don't have an estimatedBPM at all, we have to guess
        if (songIsSingerType) {
            // take a guess on a singer or vocal, when we don't know the BPM
            double defaultSingerLengthInBeats = 16 + (64 * 7) + 8;  // 16 beat intro + 7 64-beat sections + 8 beat tag
            introPosition = (16 / defaultSingerLengthInBeats);           // 0.0 - 1.0
            outroPosition = (1.0 - 8 / defaultSingerLengthInBeats );     // 0.0 - 1.0
        } else {
            // take a guess on a patter or xtra or unknown, when we don't know the BPM
            introPosition = 0.033;         // 0.0 - 1.0
            outroPosition = 1.0 - 0.033;   // 0.0 - 1.0
        }
        // we need to keep both updated, updateBgPixmap uses Frac's
        introFrac = introPosition;
        outroFrac = outroPosition;
    } else {
        if (songIsSingerType) {
            // qDebug() << "songIsSingerType";
            // it's SINGER or VOCAL type, so let's estimate using BPM
            // we are using BPM (not %)
            //        double songLength_beats = (songEnd_sec - songStart_sec)/60.0 * estimatedBPM;
            double phraseLength_beats = 64.0;  // everything is 64-beat phrases
            double phraseLength_sec = 60.0 * phraseLength_beats/estimatedBPM;

            double loopStart_sec = songStart_sec + 0.033 * (songEnd_sec - songStart_sec);
            loopStart_frac = loopStart_sec/songLength_sec;   // 0.0 - 1.0

            double loopEnd_sec = loopStart_sec + 7 * phraseLength_sec; // exactly 7 sections, probably
            loopEnd_frac = loopEnd_sec/songLength_sec;  // 0.0 - 1.0

        } else {
            // qDebug() << "songIsPatterType";
            // it's PATTER type, so let's estimate using BPM
            // we are using BPM (not %)
            //        double songLength_beats = (songEnd_sec - songStart_sec)/60.0 * estimatedBPM;
            double phraseLength_beats = 32.0;  // everything is 32-beat phrases
            double phraseLength_sec = 60.0 * phraseLength_beats/estimatedBPM;
            //        double songLength_phrases = songLength_beats/phraseLength_beats;
            double loopStart_sec = songStart_sec + 0.033 * (songEnd_sec - songStart_sec);
            loopStart_frac = loopStart_sec/songLength_sec;   // 0.0 - 1.0

            int numPhrasesThatFit = static_cast<int>((songEnd_sec - loopStart_sec)/phraseLength_sec);
            double loopEnd_sec = loopStart_sec + phraseLength_sec * numPhrasesThatFit;
            loopEnd_frac = loopEnd_sec/songLength_sec;  // 0.0 - 1.0

            if (songLength_sec - loopEnd_sec < 7) {
                // if too close to the end of the song (because an integer number of phrases just happens to fit)
                //  average phrase is about 15 sec, so 7 is a little less than 1/2 of a phrase
                // qDebug() << "TOO CLOSE:" << loopStart_sec << numPhrasesThatFit << loopEnd_sec;
                loopEnd_sec -= phraseLength_sec;  // go back one
                loopEnd_frac = loopEnd_sec/songLength_sec;  // and recalculate it
                // qDebug() << "TOO CLOSE TRIGGERED!";
            }
            // qDebug() << "PATTER:" << loopStart_sec << numPhrasesThatFit << loopEnd_sec;
        }
        // we need to keep both updated, updateBgPixmap uses Frac's
        introFrac = loopStart_frac;
        outroFrac = loopEnd_frac;
    }

    introPosition = introFrac * (width()-4);
    outroPosition = outroFrac * (width()-4);
}

// ================================================================
void svgWaveformSlider::updateBgPixmap(float *f, size_t t) {
    Q_UNUSED(t)

    // qDebug() << "updateBgPixmap" << f;

    // Normalization
    double normalizationScaleFactor = 1.0;
    if (wholeTrackPeak > 0.1) {
        normalizationScaleFactor = 1.0/wholeTrackPeak; // 1.0 = disabled, else > 1.0
    }

    // qDebug() << "normalizationScaleFactor:" << normalizationScaleFactor;

    // let's make a new pixmap to draw on
    if (bgPixmap) {
        delete bgPixmap; // let's throw away the old one before we make a new one
    }

    if (f != nullptr && f != (float*)1) {
        cachedWaveform = f;  // squirrel this away (but we don't own it, so do NOT delete it in the destructor!)
    }
    // qDebug() << "updatePixmap" << width();
    // int waveformSliderWidth = fmin(width()-4, 784); // BUG: don't ask me why it can't do more...
    int waveformSliderWidth = fmin(width()+2, 784); // BUG: don't ask me why it can't do more...
    int waveformSliderHeight = height();

    // qDebug() << "DIMENSIONS: " << waveformSliderWidth << waveformSliderHeight;

    bgPixmap = new QPixmap(waveformSliderWidth, waveformSliderHeight); // TODO: get size from current size of widget
    QPainter *paint = new QPainter(bgPixmap);

#ifndef DEBUG_LIGHT_MODE
    bgPixmap->fill(QColor("#1A1A1A"));
#else
    bgPixmap->fill(bgColor());

    // DEBUG --------
    // paint->setPen(QColor("red"));
    // paint->drawRect(-1,0, 3,3); // DEBUG

    // DEBUG --------
    // bgPixmap->fill(QColor("#888888"));
    // paint->setPen(QColor("red"));
    // paint->drawRect(0,0,60,60);
#endif

    QColor colors[4] = { QColor("#8868A1BB"), QColor("#88bf312c"), QColor("#8824a494"), QColor("#885368c9")}; // singing call colors
    int colorMap[] = {1,2,3,1,2,3,1};

    float h;

    // center line always drawn
    paint->setPen(colors[0]);
    paint->drawLine(0,30,waveformSliderWidth,30);

    const double fullScaleInPixels = 25.0;

    if (f != nullptr) { // f == nullptr means "no waveform at all"

        if (f == (float*)1) {
            // 1 means use cached f
            if (cachedWaveform == nullptr) {
                // BUT, if we don't have a cachedWaveform yet, just return
                paint->end();
                delete paint;

                if (bg != nullptr) {
                    bg->setPixmap(*bgPixmap);
                }
                return;
            }
            f = cachedWaveform;
        }

        // show normal waveform ----
        if (currentPos != nullptr) currentPos->setVisible(true);

        setLoop(drawLoopPoints);  // turn drawing of [ and ] etc ON/OFF

        // if (leftLoopMarker != nullptr) leftLoopMarker->setVisible(true);
        // if (rightLoopMarker != nullptr) rightLoopMarker->setVisible(true);
        // if (leftLoopCover != nullptr) leftLoopCover->setVisible(true);
        // if (rightLoopCover != nullptr) rightLoopCover->setVisible(true);

        if (loadingMessage != nullptr) loadingMessage->setVisible(false);

        if (singingCall) {
            // Singing call ---------------------
            int whichColor = 0;
            int currentWidth = width();
            float currentRatio = (float)WAVEFORMSAMPLES/(float)currentWidth;

            // single snapshot of these variables prevents intro/outro coloring from moving around when window resized
            int introP2 = (int)(introFrac * currentWidth);
            int outroP2 = (int)(outroFrac * currentWidth);

            for (int i = 0; i < currentWidth; i++) {

                if (i <= introP2 || i >= outroP2) {
                    whichColor = 0;
                } else {
                    int inSeg = i - introP2;
                    int whichSeg = inSeg / ((outroP2 - introP2)/7.0);
                    if (whichSeg < 0 || whichSeg > 6) {
                        whichColor = 0; // if outroP2 == introP2, e.g. not initialized, or something else bad, then this protects us
                    } else {
                        whichColor = colorMap[whichSeg];
                    }
                }

                paint->setPen(colors[whichColor]);

//                h = fullScaleInPixels * f[(int)(i * currentRatio)];  // truncates downward

                // let's do some smoothing instead...much less sparkly when resizing!
                int baseI = (int)(i*currentRatio); // truncates downward
                int baseIminus1 = (baseI >= 1 ? baseI - 1 : baseI);
                int baseIplus1  = (baseI <= WAVEFORMSAMPLES - 2 ? baseI + 1 : baseI);
                float h = normalizationScaleFactor * fullScaleInPixels * (0.25 * f[baseIminus1] + 0.5 * f[baseI] + 0.25 * f[baseIplus1]); // simple 3-point gaussian

               paint->drawLine(i,30.0 + h,i,30.0 - h);
            }
            // singing calls don't show the loop markers, because intro/outro in a singing call are not loop start/end
            if (leftLoopMarker != nullptr) leftLoopMarker->setVisible(false);
            if (rightLoopMarker != nullptr) rightLoopMarker->setVisible(false);
            if (leftLoopCover != nullptr) leftLoopCover->setVisible(false);
            if (rightLoopCover != nullptr) rightLoopCover->setVisible(false);
        } else {
            // Patter, etc. ----------------------
//            qDebug() << "updateBgPixmap 9" << WAVEFORMSAMPLES << width() << normalizationScaleFactor << fullScaleInPixels << f;

            // TODO: actually calculate the power by each 1/481 of the duration in seconds (1 pixel at a time)
            //  by peeking at the actual data.
            // TODO: Make L and R go up and down respectively?  Or, should all samples go just UP?

            // PATTER colors:
#define SET 1
#if SET==1
            // SET1 from Brewer
            // "#E41A1C" "#377EB8" "#4DAF4A" "#984EA3" "#FF7F00" "#FFFF33" "#A65628" "#F781BF"
            QColor segmentColors[9] = { QColor("#8868A1BB"),
                                       QColor("#88E41A1C"),
                                       QColor("#88377EB8"),
                                       QColor("#884DAF4A"),
                                       QColor("#88984EA3"),
                                       QColor("#88FF7F00"),
                                       QColor("#88FFFF33"),
                                       QColor("#88A65628"),
                                       QColor("#88F781BF")
            }; // 0 = default, then 1,2,3,4,5,6,7,8 are separate colors for A,B,C
            QMap<QString, int> MapSectionNameToInt{{"N", 0}, {"A", 1}, {"B", 2}, {"C", 3}, {"D", 4}, {"E", 5}, {"F", 6}, {"G", 7}, {"H", 8}};
#elif SET==2
            // DARK2 from Brewer
            // "#1B9E77" "#D95F02" "#7570B3" "#E7298A" "#66A61E" "#E6AB02" "#A6761D" "#666666"
            QColor segmentColors[9] = { QColor("#8868A1BB"),
                                        QColor("#881B9E77"),
                                        QColor("#88D95F02"),
                                        QColor("#887570B3"),
                                        QColor("#88E7298A"),
                                        QColor("#8866A61E"),
                                        QColor("#88E6AB02"),
                                        QColor("#88A6761D"),
                                        QColor("#88666666")
            }; // 0 = default, then 1,2,3,4,5,6,7,8 are separate colors for A,B,C
            QMap<QString, int> MapSectionNameToInt{{"N", 0}, {"A", 1}, {"B", 2}, {"C", 3}, {"D", 4}, {"E", 5}, {"F", 6}, {"G", 7}, {"H", 8}};
#else
            // SINGING CALL COLORS
            QColor segmentColors[4] = { QColor("#8868A1BB"),
                                       QColor("#88bf312c"),
                                       QColor("#8824a494"),
                                       QColor("#885368c9")
            }; // 0 = default, then 1,2,3 are separate colors for A,B,C
            QMap<QString, int> MapSectionNameToInt{{"N", 0}, {"A", 1}, {"B", 2}, {"C", 3}, {"D", 1}, {"E", 2}, {"F", 3}, {"G", 1}, {"H", 2}};
#endif

            struct sectionEntry
            {
                double startTime_sec;
                double duration_sec;
                QString sectionName;
                QColor sectionColor;
            };
            QVector<sectionEntry> sections;

            // read the segment file, if it exists
//            QString segmentFilename = "/Users/mpogue/Library/CloudStorage/Box-Box/__squareDanceMusic_Box/.squaredesk/bulk/patter/RIV 914 - About Time.mp3.results.txt";

            QFileInfo segmentFile(absolutePathToSegmentFile);
            if (segmentFile.exists()) {
                QFile segFile(absolutePathToSegmentFile);
                if (segFile.open(QIODevice::ReadOnly))
                {
                    QTextStream in(&segFile);
                    //        int line_number = 0;
                    while (!in.atEnd()) //  && line_number < 10)
                    {
                        //            line_number++;
                        QString line = in.readLine().trimmed();
                        QStringList p1 = line.split(":");
                        bool ok1, ok2;
                        double startTime = (p1[0].split(", ")[0]).toDouble(&ok1);
                        double duration  = (p1[0].split(", ")[1]).toDouble(&ok2);
                        QString section   = p1[1].trimmed().split(" ")[1];
                        if (section.startsWith("N")) {
                            section = "N";  // all "N"s have the same color
                        }

                        // qDebug() << "LINE:" << line << "startTime/duration/section: " << startTime << duration << section << MapSectionNameToInt[section] << segmentColors[MapSectionNameToInt[section]];
                        sectionEntry s;
                        s.startTime_sec = startTime;
                        s.duration_sec = duration;
                        s.sectionName   = section;
                        s.sectionColor = segmentColors[MapSectionNameToInt[section]];
                        sections.append(s);
                    }

                    // qDebug() << "sections:";
                    // for (int i = 0; i < sections.count(); i++) {
                    //     qDebug() << "section[" << i << "] = " << sections.at(i).startTime_sec << "," << sections.at(i).startTime_sec << " : "<< sections.at(i).sectionName << " : " << sections.at(i).sectionColor;
                    // }
                }
            }

            if (sections.count() >= 2) {
                int currentSegment = 0;  // starts out with the first segment
                double songLength_sec = sections.last().startTime_sec + sections.last().duration_sec;
                // qDebug() << "songLength_sec:" << songLength_sec;
                bool firstLineInSection = true;

                float currentRatio = (float)WAVEFORMSAMPLES/(float)width();
                for (int i = 0; i < width(); i++) {
                    h = normalizationScaleFactor * fullScaleInPixels * f[(int)(i * currentRatio)];  // truncates downward

                    if (firstLineInSection) {
                        paint->setPen(segmentColors[0]); // first line in a segment is always gray
                        firstLineInSection = false;
                    } else {
                        paint->setPen(sections[currentSegment].sectionColor);
                    }
                    paint->drawLine(i,30.0 + h,i,30.0 - h);

                    if (currentSegment + 1 == sections.count()) {
                        // no change to currentSegment
                    } else if ( songLength_sec * ((float)i / (float)width()) > sections[currentSegment+1].startTime_sec) {
                        currentSegment++;  // bump to the next segment
                        firstLineInSection = true;
                    }
                }
            } else {
                // no valid section info, so just paint in the default color
                paint->setPen(segmentColors[0]); // all the lines are gray for this song
                float currentRatio = (float)WAVEFORMSAMPLES/(float)width();
                for (int i = 0; i < width(); i++) {
                    h = normalizationScaleFactor * fullScaleInPixels * f[(int)(i * currentRatio)];  // truncates downward
                    paint->drawLine(i,30.0 + h,i,30.0 - h);
                }
            }
        }
    } else {
        // show just bg, centerline, and loading message ----
        // qDebug() << "just show LOADING message";
        if (currentPos != nullptr) currentPos->setVisible(false);
        if (leftLoopMarker != nullptr) leftLoopMarker->setVisible(false);
        if (rightLoopMarker != nullptr) rightLoopMarker->setVisible(false);
        if (leftLoopCover != nullptr) leftLoopCover->setVisible(false);
        if (rightLoopCover != nullptr) rightLoopCover->setVisible(false);

        if (loadingMessage != nullptr) loadingMessage->setVisible(true);
    }

    paint->end();
    delete paint;

    // and load it
    if (bg != nullptr) bg->setPixmap(*bgPixmap);

    setIntro(introFrac); // refresh the [
    setOutro(outroFrac); // refresh the ]
    setValue(value());   // refresh the |
}

// ----------------------
void svgWaveformSlider::setSingingCall(bool b) {
    //        qDebug() << "*** setSingingCall:" << b;
    singingCall = b;
}

void svgWaveformSlider::setOrigin(int i) {
    origin = i;
}

// LOOPS -------
void svgWaveformSlider::setLoop(bool b) {
    drawLoopPoints = b;

    if (leftLoopMarker != nullptr && leftLoopMarker != nullptr) {
        // qDebug() << "setLoop: " << drawLoopPoints;
        leftLoopCover->setVisible(drawLoopPoints);
        leftLoopMarker->setVisible(drawLoopPoints);
        rightLoopCover->setVisible(drawLoopPoints);
        rightLoopMarker->setVisible(drawLoopPoints);
    }

}

void svgWaveformSlider::setIntro(double frac) {
//    introPosition = frac * WAVEFORMWIDTH;
    introPosition = frac * (width()-4);
    introFrac = frac;

    //        qDebug() << "*** setIntro:" << introPosition;
    if (leftLoopMarker != nullptr && leftLoopCover != nullptr) {
        leftLoopMarker->setX(introPosition);
        // Iterate over the items in the group, set their pen to the new color
        QList<QGraphicsItem*> items = leftLoopMarker->childItems();
        // qDebug() << "Number of items in group:" << items.size();

        for (QGraphicsItem* item : items) {
            QGraphicsLineItem* line = qgraphicsitem_cast<QGraphicsLineItem*>(item);
            if (line) {
                // qDebug() << "Line Item:" << line;
                QPen currentLoopPen(loopColor(), 2);
                line->setPen(currentLoopPen);
            }
        }
        leftLoopCover->setPen(QPen(darkeningColor()));
        leftLoopCover->setBrush(QBrush(darkeningColor()));
        leftLoopCover->setRect(-3,0, introPosition+3,61); // -3 is to cover up bugs on left hand side
    }

    // qDebug() << "*** setIntro darkeningColor" << darkeningColor();
}

void svgWaveformSlider::setOutro(double frac) {
    outroPosition = frac * (width()-4);
    outroFrac = frac;

    //        qDebug() << "*** setOutro:" << outroPosition;
    if (rightLoopMarker != nullptr && rightLoopCover != nullptr) {
        rightLoopMarker->setX(outroPosition - 3); // 3 is to compensate for the width of the green bracket

        // Iterate over the items in the group, set their pen to the new color
        QList<QGraphicsItem*> items = rightLoopMarker->childItems();
        // qDebug() << "Number of items in group:" << items.size();

        for (QGraphicsItem* item : items) {
            QGraphicsLineItem* line = qgraphicsitem_cast<QGraphicsLineItem*>(item);
            if (line) {
                // qDebug() << "Line Item:" << line;
                QPen currentLoopPen(loopColor(), 2);
                line->setPen(currentLoopPen);
            }
        }

        rightLoopCover->setRect(outroPosition+1,0, width()+1-outroPosition,61);
        rightLoopCover->setBrush(QBrush(darkeningColor()));
        rightLoopCover->setPen(QPen(darkeningColor()));
    }
    // qDebug() << "*** setOutro darkeningColor" << darkeningColor();
}

double svgWaveformSlider::getIntro() {
    return(introPosition);
}

double svgWaveformSlider::getOutro() {
    return(outroPosition);
}

double svgWaveformSlider::getIntroFrac() {
    return(introFrac);
}

double svgWaveformSlider::getOutroFrac() {
    return(outroFrac);
}

void svgWaveformSlider::setWholeTrackPeak(double p) {
    // qDebug() << "wholeTrackPeak is now:" << p;
    wholeTrackPeak = p;
}

void svgWaveformSlider::setAbsolutePathToSegmentFile(QString s) {
    // qDebug() << "absolutePathToSegmentFile: " << absolutePathToSegmentFile;
    absolutePathToSegmentFile = s;
}
