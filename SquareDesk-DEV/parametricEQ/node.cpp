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

#include "edge.h"
#include "node.h"
#include "graphwidget.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOption>

Node::Node(GraphWidget *graphWidget, int which, qreal xx, qreal yy)
    : which(which),
      graph(graphWidget)
{
    setPos(xx, yy); // set initial position
    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);
    setCacheMode(DeviceCoordinateCache);
    setZValue(-1);
}

void Node::addEdge(Edge *edge1) {
    edge = edge1;
    edge->adjust();  // draw yourself (eventually)
}

QRectF Node::boundingRect() const
{
    qreal adjust = 2;
    return QRectF( -10 - adjust, -10 - adjust, 23 + adjust, 23 + adjust);
}

QPainterPath Node::shape() const
{
    QPainterPath path;
    path.addEllipse(-10, -10, 20, 20);
    return path;
}

void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
//    painter->setPen(Qt::NoPen);
//    painter->setBrush(Qt::darkGray);
//    painter->drawEllipse(-7, -7, 20, 20);

    QRadialGradient gradient(-3, -3, 10);
    QColor light, dark;
    switch (which) {
    case 1: light = QColor(Qt::red);
            dark  = QColor(Qt::darkRed);
            break;
    case 2: light = QColor(Qt::yellow);
            dark  = QColor(Qt::darkYellow);
            break;
    case 3: light = QColor(Qt::green);
            dark  = QColor(Qt::darkGreen);
            break;
    case 4: light = QColor(Qt::blue);
            dark  = QColor(Qt::darkBlue);
            break;
    case 5: light = QColor(Qt::magenta);
            dark  = QColor(Qt::darkMagenta);
            break;
    }
    if (option->state & QStyle::State_Sunken) {
        gradient.setCenter(3, 3);
        gradient.setFocalPoint(3, 3);
        QColor lightL = light.light(120);
        QColor darkL  = dark.light(120);
//        lightL.setAlpha(127);
//        darkL.setAlpha(127);
        gradient.setColorAt(1, lightL);
        gradient.setColorAt(0, darkL);
    } else {
        gradient.setColorAt(0, light);
        gradient.setColorAt(1, dark);
    }
    painter->setBrush(gradient);

    painter->setPen(QPen(Qt::black, 0));
    painter->drawEllipse(-10, -10, 20, 20);
}

QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene()) {

        // value is the new position.
        QPointF newPos1 = value.toPointF();

        // Don't cross the streams!
        switch (which) {
        case 1:
            newPos1.setX(qMin(newPos1.x(), graph->node2->pos().x()));  // 1 can't go right of 2
            break;
        case 2:
            newPos1.setX(qMin(qMax(newPos1.x(), graph->node1->pos().x()), graph->node3->pos().x()));  // 2 can't go left of 1, right of 3
            break;
        case 3:
            newPos1.setX(qMin(qMax(newPos1.x(), graph->node2->pos().x()), graph->node4->pos().x()));  // 3 can't go left of 2, right of 4
            break;
        case 4:
            newPos1.setX(qMin(qMax(newPos1.x(), graph->node3->pos().x()), graph->node5->pos().x()));  // 4 can't go left of 3, right of 5
            break;
        case 5:
            newPos1.setX(qMax(newPos1.x(), graph->node4->pos().x()));  // 5 can't go left of 4
            break;
        }

        // keep in bounds
        QRectF sceneRect = scene()->sceneRect();
        newPos1.setX(qMin(qMax(newPos1.x(), sceneRect.left() + 25), sceneRect.right() - 25));
        newPos1.setY(qMin(qMax(newPos1.y(), sceneRect.top() + 25), sceneRect.bottom() - 25));

        edge->adjust();
        graph->itemMoved();

        return newPos1;  // return the restricted position
    }

    return QGraphicsItem::itemChange(change, value);
}

void Node::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    update();
    QGraphicsItem::mousePressEvent(event);
}

void Node::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    update();
    QGraphicsItem::mouseReleaseEvent(event);
}

qreal Node::getFreq() {
    return graph->x2freq(pos().x());
}

qreal Node::getdB() {
    return graph->y2dB(pos().y());
}
