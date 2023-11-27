#include "svgWaveformSlider.h"
#include "math.h"

// GOOD:
// https://stackoverflow.com/questions/70206279/qslider-not-displaying-transparency-properly-when-overlapped-with-qlabel-pyqt5

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
    SetDefaultIntroOutroPositions(false, 0.0, 0.0, 0.0, 0.0);
    origin = 0;

    nowDestroying = false; // true when we're in the destructor, shutting everything down (don't want any paint events or updates)

    wholeTrackPeak = 1.0; // disable waveform scaling by default

    // install the wheel/scroll eater --------
    this->installEventFilter(this);  // eventFilter() is called, where wheel/touchpad scroll events are eaten

//    finishInit();
}

svgWaveformSlider::~svgWaveformSlider()
{
}

// ------------------------------------
void svgWaveformSlider::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED(pe)
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
}

void svgWaveformSlider::setValue(int val) {
    QSlider::setValue(val);  // set the value of the parent class

    if (currentPos == nullptr) {
        return; // slider is not fully initialized yet
    }

    int currentPositionX = round((float)val * ((float)(width()-4)/(float)maximum()));

    currentPos->setPos(currentPositionX, 0);
};

// MOUSE EVENTS ============
void svgWaveformSlider::mousePressEvent(QMouseEvent* e) {
    double val;

    switch (e->button()) {
        case Qt::LeftButton:
            val = this->maximum() * (fmax(0,(e->pos().rx() - 6))/(width()-4));
            this->setValue(val);
            break;
        case Qt::MiddleButton:
        case Qt::RightButton:
            break;
        default:
            break;
    }
}

void svgWaveformSlider::mouseMoveEvent(QMouseEvent* e) {
    Q_UNUSED(e)
    double val = this->maximum() * (fmax(0,(e->pos().rx() - 6))/(width()-4));
    this->setValue(val);
}

void svgWaveformSlider::mouseDoubleClickEvent(QMouseEvent* e) {
    qDebug() << "mouseDoubleClickEvent: " << e;
}

void svgWaveformSlider::mouseReleaseEvent(QMouseEvent* e) {
    Q_UNUSED(e)
}

void svgWaveformSlider::finishInit() {
    setValue(0.0);    // set to beginning
    setFixedHeight(61);

    // BACKGROUND PIXMAP --------
    bgPixmap = new QPixmap(WAVEFORMSAMPLES, 61);  // NOTE: we gotta make this bug to start, or the QSlider will never figure out that it can draw bigger.
    bgPixmap->fill(QColor("#FA0000"));

//    DEBUG: save to a file so we can look at it ----
//    bgPixmap->save("myPixmap.png");
//    scene.addPixmap(*bgPixmap);

    bg = new QGraphicsPixmapItem(*bgPixmap);

    // LOADING MESSAGE --------
    loadingMessage = new QGraphicsTextItem("Loading...");
    loadingMessage->setFont(QFont("Avenir Next", 18));
    loadingMessage->setPos(20, 5);
    loadingMessage->setVisible(false);

    // CURRENT POSITION MARKER -------
    QPen currentPosPen(QColor("#00FF33"), 1);
    QBrush currentPosBrush(QColor("#00FF33"));

    currentPos = new QGraphicsItemGroup();

    QGraphicsLineItem *vLine = new QGraphicsLineItem(0,0, 0,61); // initial position of the line
    vLine->setPen(currentPosPen);

    QPolygonF p;
    float d = 5.0;
    p << QPointF(-d, 0.0) << QPointF(d, 0.0) << QPointF(0.0, d) << QPointF(-d, 0.0);
    QGraphicsPolygonItem *topTri = new QGraphicsPolygonItem(p);
    topTri->setBrush(currentPosBrush);

    QPolygonF p2;
    p2 << QPointF(-d, 60.0) << QPointF(d, 60.0) << QPointF(0.0, 60.0-d) << QPointF(-d, 60.0);
    QGraphicsPolygonItem *botTri = new QGraphicsPolygonItem(p2);
    botTri->setBrush(currentPosBrush);

    currentPos->addToGroup(vLine);
    currentPos->addToGroup(topTri);
    currentPos->addToGroup(botTri);

    currentPos->setPos(0, 0);
    currentPos->setVisible(false);  // initially false, since nothing is loaded yet

    // LOOP INDICATORS -------
    QPen currentLoopPen(QColor("#26A4ED"), 2);
    QBrush loopDarkeningBrush(QColor("#80000000"));

    // left --
    leftLoopMarker = new QGraphicsItemGroup();

    QGraphicsLineItem *top = new QGraphicsLineItem(0,1, 4,1);
    top->setPen(currentLoopPen);
    QGraphicsLineItem *middle = new QGraphicsLineItem(0,0, 0,61);
    middle->setPen(currentLoopPen);
    QGraphicsLineItem *bot = new QGraphicsLineItem(0,61-2, 4,61-2);
    bot->setPen(currentLoopPen);

    leftLoopMarker->addToGroup(top);
    leftLoopMarker->addToGroup(middle);
    leftLoopMarker->addToGroup(bot);

    leftLoopMarker->setPos(100,0);

    // left cover --
    leftLoopCover = new QGraphicsRectItem(0,0,100,61);
    leftLoopCover->setBrush(loopDarkeningBrush);

    // right --
    rightLoopMarker = new QGraphicsItemGroup();

    QGraphicsLineItem *topR = new QGraphicsLineItem(0,1, 4,1);
    topR->setPen(currentLoopPen);
    QGraphicsLineItem *middleR = new QGraphicsLineItem(4,0, 4,61);
    middleR->setPen(currentLoopPen);
    QGraphicsLineItem *botR = new QGraphicsLineItem(0,61-2, 4,61-2);
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
    //    view.setParent(this, Qt::FramelessWindowHint);
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
                                             double songStart_sec, double songEnd_sec, double songLength_sec)
{
    if (!tempoIsBPM) {
        double defaultSingerLengthInBeats = (64 * 7 + 24);  // 16 beat intro + 8 beat tag = 24
        introPosition = (16 / defaultSingerLengthInBeats);           // 0.0 - 1.0
        outroPosition = (1.0 - 8 / defaultSingerLengthInBeats );     // 0.0 - 1.0
        //        qDebug() << "MySlider::SetDefault" << introPosition << outroPosition;
    } else {
        // we are using BPM (not %)
        //        double songLength_beats = (songEnd_sec - songStart_sec)/60.0 * estimatedBPM;
        double phraseLength_beats = 32.0;  // everything is 32-beat phrases
        double phraseLength_sec = 60.0 * phraseLength_beats/estimatedBPM;
        //        double songLength_phrases = songLength_beats/phraseLength_beats;
        double loopStart_sec = songStart_sec + 0.05 * (songEnd_sec - songStart_sec);
        double loopStart_frac = loopStart_sec/songLength_sec;   // 0.0 - 1.0

        int numPhrasesThatFit = static_cast<int>((songEnd_sec - loopStart_sec)/phraseLength_sec);
        double loopEnd_sec = loopStart_sec + phraseLength_sec * numPhrasesThatFit;
        double loopEnd_frac = loopEnd_sec/songLength_sec;  // 0.0 - 1.0

        if (songLength_sec - loopEnd_sec < 7) {
            // if too close to the end of the song (because an integer number of phrases just happens to fit)
            //  average phrase is about 15 sec, so 7 is a little less than 1/2 of a phrase
            loopEnd_sec -= phraseLength_sec;  // go back one
            loopEnd_frac = loopEnd_sec/songLength_sec;  // and recalculate it
            //            qDebug() << "TOO CLOSE TRIGGERED!";
        }

        ////        qDebug() << "MySlider::songLength_beats" << songLength_beats;
        ////        qDebug() << "MySlider::songLength_phrases" << songLength_phrases;
        //        qDebug() << "=====================";
        //        qDebug() << "MySlider::tempoIsBPM" << tempoIsBPM;
        //        qDebug() << "MySlider::estimatedBPM" << estimatedBPM;
        //        qDebug() << "MySlider::songStart_sec" << songStart_sec;
        //        qDebug() << "MySlider::songEnd_sec" << songEnd_sec;
        //        qDebug() << "MySlider::songLength_sec" << songLength_sec;
        //        qDebug() << "--------------";
        //        qDebug() << "MySlider::phraseLength_beats" << phraseLength_beats;
        //        qDebug() << "MySlider::phraseLength_sec" << phraseLength_sec;
        //        qDebug() << "MySlider::loopStart_sec" << loopStart_sec;
        //        qDebug() << "MySlider::loopStart_frac" << loopStart_frac;
        //        qDebug() << "MySlider::numPhrasesThatFit" << numPhrasesThatFit;
        //        qDebug() << "MySlider::loopEnd_sec" << loopEnd_sec;
        //        qDebug() << "MySlider::loopEnd_frac" << loopEnd_frac;
        ////        qDebug() << "--------------";

         // we need to keep both updated, updateBgPixmap uses Frac's
        introFrac = loopStart_frac;
        outroFrac = loopEnd_frac;
        introPosition = introFrac * (width()-4);
        outroPosition = outroFrac * (width()-4);
    }
}

// ================================================================
void svgWaveformSlider::updateBgPixmap(float *f, size_t t) {
    Q_UNUSED(t)

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
        cachedWaveform = f;  // squirrel this away
    }

    int waveformSliderWidth = fmin(width()-4, 784); // BUG: don't ask me why it can't do more...
    int waveformSliderHeight = height();

//    qDebug() << "DIMENSIONS: " << waveformSliderWidth << waveformSliderHeight;

    bgPixmap = new QPixmap(waveformSliderWidth, waveformSliderHeight); // TODO: get size from current size of widget
    QPainter *paint = new QPainter(bgPixmap);

    bgPixmap->fill(QColor("#1A1A1A"));

    QColor colors[4] = { QColor("#8868A1BB"), QColor("#88bf312c"), QColor("#8824a494"), QColor("#885368c9")};
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
        if (leftLoopMarker != nullptr) leftLoopMarker->setVisible(true);
        if (rightLoopMarker != nullptr) rightLoopMarker->setVisible(true);
        if (leftLoopCover != nullptr) leftLoopCover->setVisible(true);
        if (rightLoopCover != nullptr) rightLoopCover->setVisible(true);

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

            float currentRatio = (float)WAVEFORMSAMPLES/(float)width();
            for (int i = 0; i < width(); i++) {
                h = normalizationScaleFactor * fullScaleInPixels * f[(int)(i * currentRatio)];  // truncates downward
                paint->drawLine(i,30.0 + h,i,30.0 - h);
            }
        }
    } else {
        // show just bg, centerline, and loading message ----
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
        leftLoopCover->setRect(0,0, introPosition,61);
    }
}

void svgWaveformSlider::setOutro(double frac) {
    outroPosition = frac * (width()-4);
    outroFrac = frac;

    //        qDebug() << "*** setOutro:" << outroPosition;
    if (rightLoopMarker != nullptr && rightLoopCover != nullptr) {
        rightLoopMarker->setX(outroPosition - 3); // 3 is to compensate for the width of the green bracket
        rightLoopCover->setRect(outroPosition,0, width()-4-outroPosition,61);
    }
}

double svgWaveformSlider::getIntro() {
    return(introPosition);
}

double svgWaveformSlider::getOutro() {
    return(outroPosition);
}

void svgWaveformSlider::setWholeTrackPeak(double p) {
    // qDebug() << "wholeTrackPeak is now:" << p;
    wholeTrackPeak = p;
}
