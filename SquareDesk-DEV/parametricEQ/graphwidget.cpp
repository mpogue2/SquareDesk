/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

/****************************************************************************
**
** Copyright (C) 2016, 2017, 2018 Mike Pogue, Dan Lyke
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

#include "graphwidget.h"
#include "edge.h"
#include "node.h"

#include <math.h>

#include <QKeyEvent>
#include <QDebug>

qreal GraphWidget::freq2x(qreal f) {
    QRectF sceneRect = this->sceneRect();
    qreal left = sceneRect.left()+15;
    qreal right = sceneRect.right() - 25;
    qreal x = left + (right-left)*(log10(f/20000)+3)/3;

    return x;
}

qreal GraphWidget::x2freq(qreal x) {
    QRectF sceneRect = this->sceneRect();
    qreal left = sceneRect.left();
    qreal right = sceneRect.right() - 25;
    qreal f = 20000.0*pow(10.0,(3.0*(x - left)/(right-left)-3.0));

    return f;
}

qreal GraphWidget::dB2y(qreal dB) {
    QRectF sceneRect = this->sceneRect();
    qreal top = sceneRect.top();
    qreal bottom = sceneRect.bottom();
    qreal three_dB_in_pixels = 0.5*(bottom-top)/18;
    qreal centerY = sceneRect.center().y();
    qreal upperY = centerY - dB * three_dB_in_pixels;

    return upperY;
}

qreal GraphWidget::y2dB(qreal y) {
    QRectF sceneRect = this->sceneRect();
    qreal top = sceneRect.top();
    qreal bottom = sceneRect.bottom();
    qreal three_dB_in_pixels = 0.5*(bottom-top)/18;
    qreal centerY = sceneRect.center().y();
    qreal dB = -1.0*(y - centerY)/three_dB_in_pixels;

    return dB;
}

GraphWidget::GraphWidget(QWidget *parent)
    : QGraphicsView(parent), timerId(0)
{
    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    scene->setSceneRect(-200, -150, 400, 300);
    setScene(scene);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(AnchorUnderMouse);
    scale(qreal(0.8), qreal(0.8));
    setMinimumSize(400, 300);
    setWindowTitle(tr("Multi-band Parametric EQ"));

    node1 = new Node(this, 1, freq2x(100),   dB2y(0));
    node2 = new Node(this, 2, freq2x(250),   dB2y(0));
    node3 = new Node(this, 3, freq2x(1000),  dB2y(0));
    node4 = new Node(this, 4, freq2x(2500),  dB2y(0));
    node5 = new Node(this, 5, freq2x(10000), dB2y(0));

    edge1 = new Edge(this);
    scene->addItem(edge1);

    scene->addItem(node1);
    scene->addItem(node2);
    scene->addItem(node3);
    scene->addItem(node4);
    scene->addItem(node5);

    node1->addEdge(edge1);
    node2->addEdge(edge1);
    node3->addEdge(edge1);
    node4->addEdge(edge1);
    node5->addEdge(edge1);
}

void GraphWidget::itemMoved()
{
//    qDebug() << node1->getFreq() << node1->getdB() <<
//                node2->getFreq() << node2->getdB() <<
//                node3->getFreq() << node3->getdB() <<
//                node4->getFreq() << node4->getdB() <<
//                node5->getFreq() << node5->getdB()
//                ;
}

qreal GraphWidget::filterResponseInDB(qreal freq)
{
    qreal pi = 3.14159265;

    qreal filterx = node1->x();
    qreal x = freq2x(freq);
    qreal offsetx = abs(filterx - x);
    qreal q = 3.0;
    qreal offsetr = q*2*pi*offsetx/360.0;
    offsetr = fmax(-pi, fmin(pi, offsetr));
    qreal val = (1 + cos(offsetr)) * node1->getdB()/2.0;

    filterx = node2->x();
    x = freq2x(freq);
    offsetx = abs(filterx - x);
    q = 3.0;
    offsetr = q*2*pi*offsetx/360.0;
    offsetr = fmax(-pi, fmin(pi, offsetr));
    val += (1 + cos(offsetr)) * node2->getdB()/2.0;

    filterx = node3->x();
    x = freq2x(freq);
    offsetx = abs(filterx - x);
    q = 3.0;
    offsetr = q*2*pi*offsetx/360.0;
    offsetr = fmax(-pi, fmin(pi, offsetr));
    val += (1 + cos(offsetr)) * node3->getdB()/2.0;

    filterx = node4->x();
    x = freq2x(freq);
    offsetx = abs(filterx - x);
    q = 3.0;
    offsetr = q*2*pi*offsetx/360.0;
    offsetr = fmax(-pi, fmin(pi, offsetr));
    val += (1 + cos(offsetr)) * node4->getdB()/2.0;

    filterx = node5->x();
    x = freq2x(freq);
    offsetx = abs(filterx - x);
    q = 3.0;
    offsetr = q*2*pi*offsetx/360.0;
    offsetr = fmax(-pi, fmin(pi, offsetr));
    val += (1 + cos(offsetr)) * node5->getdB()/2.0;

    return val;
}


void GraphWidget::keyPressEvent(QKeyEvent *event)
{
        QGraphicsView::keyPressEvent(event);
}

void GraphWidget::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
}

#if QT_CONFIG(wheelevent)
void GraphWidget::wheelEvent(QWheelEvent *event)
{
    Q_UNUSED(event)
}
#endif

// ========================================================================
void GraphWidget::drawBackground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(rect);

    QRectF sceneRect = this->sceneRect();

    // Fill
    QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
    gradient.setColorAt(0, QColor(60,60,60));
    gradient.setColorAt(1, QColor(60,60,60));
    painter->fillRect(rect.intersected(sceneRect), gradient);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(sceneRect);

    // Lines
    painter->setRenderHint(QPainter::Antialiasing, false);

    qreal left = sceneRect.left();
    qreal top = sceneRect.top();
    qreal bottom = sceneRect.bottom();
    qreal centerY = sceneRect.center().y();
    qreal three_dB_in_pixels = 0.5*(bottom-top)/18;

    QFont font = painter->font();
    font.setPointSize(14);
    painter->setFont(font);
    painter->setPen(Qt::lightGray);

    // H lines
    int sz = 13;
    qreal scl = 1.5;
    painter->setPen(palette().dark().color());
    for (int i = 0; i <= 17; i+=3) {
        qreal upperY = dB2y(i);
        qreal lowerY = dB2y(-i);
        painter->setPen(Qt::darkGray);
        painter->drawLine(freq2x(30),upperY, freq2x(20000),upperY);
        painter->drawLine(freq2x(30),lowerY, freq2x(20000),lowerY);

        QRectF tRectUpper(QPointF(freq2x(18), upperY-0.5*three_dB_in_pixels), QSizeF(sz*scl,sz));
        QRectF tRectLower(QPointF(freq2x(18), lowerY-0.5*three_dB_in_pixels), QSizeF(sz*scl,sz));
        painter->setPen(Qt::white);
        painter->drawText(tRectUpper, Qt::AlignCenter, QString::number(i));
        painter->drawText(tRectLower, Qt::AlignCenter, QString::number(-i));
    }
    painter->setPen(QColor(255,255,255,255));
    painter->drawLine(freq2x(30),centerY, freq2x(20000),centerY);  // midline

    // V lines
    qreal vlines[] = {30,40,50,60,70,80,90,100,
                      200,300,400,500,600,700,800,900,1000,
                      2000,3000,4000,5000,6000,7000,8000,9000,10000, 20000};
    for (unsigned int i = 0; i < sizeof(vlines)/sizeof(qreal); i++){
        qreal x = freq2x(vlines[i]);
        if (vlines[i]==100 || vlines[i]==1000 || vlines[i]==10000) {
            painter->setPen(Qt::white);
        } else {
            painter->setPen(Qt::darkGray);
        }
        painter->drawLine(x, centerY + 15 * three_dB_in_pixels, x, centerY - 15 * three_dB_in_pixels);
    }

    // H axis labels
    qreal vlabelsX[] = {50,100,200,500,1000,2000,5000,10000, 20000};
    QString vlabelsText[] = {"50","100","200","500","1K","2K","5K","10K","20K"};
    painter->setPen(Qt::white);
    for (unsigned int i = 0; i < sizeof(vlabelsX)/sizeof(qreal); i++){
        qreal x = freq2x(vlabelsX[i]);
        QRectF tRectText(QPointF(x-13,bottom-18), QSizeF(1.5*sz*scl,sz));
        painter->drawText(tRectText, Qt::AlignCenter, QString(vlabelsText[i]));
    }

}
void GraphWidget::scaleView(qreal scaleFactor)
{
    Q_UNUSED(scaleFactor);
}

void GraphWidget::shuffle()
{
}

void GraphWidget::zoomIn()
{
}

void GraphWidget::zoomOut()
{
}
