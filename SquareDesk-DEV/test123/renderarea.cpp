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

#include "renderarea.h"

#include <QPainter>
#include <QDebug>

RenderArea::RenderArea(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    bad = 0;

    // TODO: two sets of colors needed
    setCoupleColoringScheme("Normal");
    mentalImageMode = false;
}

QSize RenderArea::minimumSizeHint() const
{
    return QSize(100, 100);
}

QSize RenderArea::sizeHint() const
{
    return QSize(360, 360);
}

void RenderArea::setPen(const QPen &pen)
{
//    qDebug() << "setPen" << bad++;
    this->pen = pen;
//    update();
}

void RenderArea::setBrush(const QBrush &brush)
{
//    qDebug() << "setBrush" << bad++;
    this->brush = brush;
//    update();
}

void RenderArea::setLayout1(QString s)
{
//    qDebug() << "setLayout1" << s;
    this->layout1 = s;
    update();
}

void RenderArea::setLayout2(QStringList sl)
{
//    qDebug() << "setLayout2" << sl;
    this->layout2 = sl;
    update();
}

void RenderArea::setFormation(QString s)
{
//    qDebug() << "setFormation" << bad++;
    this->formation = s;
    update();
}

void RenderArea::setCoupleColoringScheme(QString colorScheme) {
    Q_UNUSED(colorScheme)
//    coupleColoringScheme = colorScheme;  // save as QString
//    if (colorScheme == "Normal" || colorScheme == "Color only") {
//        coupleColor[COUPLE1] = COUPLE1COLOR;
//        coupleColor[COUPLE2] = COUPLE2COLOR;
//        coupleColor[COUPLE3] = COUPLE3COLOR;
//        coupleColor[COUPLE4] = COUPLE4COLOR;
//        mentalImageMode = false;
//    } else if (colorScheme == "Mental image") {
//        coupleColor[COUPLE1] = COUPLE1COLOR;
//        coupleColor[COUPLE2] = GREYCOUPLECOLOR;
//        coupleColor[COUPLE3] = GREYCOUPLECOLOR;
//        coupleColor[COUPLE4] = GREYCOUPLECOLOR;
//        mentalImageMode = true;
//    } else {
//        // Sight
//        coupleColor[COUPLE1] = COUPLE1COLOR;
//        coupleColor[COUPLE2] = GREYCOUPLECOLOR;
//        coupleColor[COUPLE3] = GREYCOUPLECOLOR;
//        coupleColor[COUPLE4] = COUPLE4COLOR;
//        mentalImageMode = false;
//    }
//    update();  // re-render the area
}


void RenderArea::paintEvent(QPaintEvent * /* event */)
{
//    qDebug() << "paintEvent" << bad++;

//    QRect rect(10, 20, 80, 60);

    QPainter painter(this);

    painter.setPen(QPen(Qt::black));
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (layout1 == "" && layout2.length() == 0) {
        setLayout1("      n  o  @@k              b@@j              c@@      g  f  ");
        QStringList sl;
        sl.append("        3GV   3BV      ");
        sl.append("");
        sl.append(" 4B>               2G<");
        sl.append("");
        sl.append(" 4G>               2B<");
        sl.append("");
        sl.append("       1B^   1G^      ");
        setLayout2(sl);
    }

    if (layout1 != "" && layout2.length() != 0) {
//        qDebug() << "************";
//        qDebug() << "layouts:" << layout1 << layout2;
//        qDebug() << "w/h:" << this->width() << this->height();

//        QPoint center(180,180);
//        qDebug() << "center:" << center;
        QPoint center(this->width()/2, this->height()/2);

        // make a list of each person
        QString peopleStr;
        foreach (const QString &line, layout2) {
            peopleStr += line;
        }
//        peopleStr.replace("    "," ").replace("   "," ").replace("  "," ").replace(QRegExp("^ "),"");
        peopleStr.replace(QRegExp("[ ]+")," ").replace(QRegExp("^ "),"");
//        qDebug() << "peopleStr:" << peopleStr;
        QStringList people = peopleStr.split(" ");
//        qDebug() << "people:" << people;

        if (people.length() < 8) {
            return; // defensive programming (with phantoms, get more than 8)
        }

        // scan for X and Y size -------------
        float x = 0.0;
        float y = 0.0;

        // THESE DEFINE THE SIZES OF PEOPLE AND THE ENTIRE FIELD
        int oneUnit = 5;                 // pixels: field is 4 * 12 of these wide and tall
        int oneSpace = 6 * oneUnit;      // pixels
        // note that one person is only 4 units wide

        x = y = 0.0;
        float longestx = 0.0;

        for (int i=0; i<layout1.length(); i++) {
            QChar c = layout1[i];
            QChar c2 = layout1[i+1];  // FIX: this goes off the end!
            if (c == '@' && c2 == '7') {
                if (x > longestx) {
                    longestx = x;
                }
                x = 0;
                y += 0.5*oneSpace;
                i++;  // consume the '7'
            } else if (c == '@' && c2 != '@') {
                if (x > longestx) {
                    longestx = x;
                }
                x = 0;
                y += oneSpace;
            } else if (c == '@' && c2 == '@') {
                if (x > longestx) {
                    longestx = x;
                }
                x = 0;
                y += oneSpace;
                i++;  // consume the second '@' for graphical plot
            } else if (c >= 'a' && c <= 'z') {
                x += 4.0*oneUnit;  // TEST
            } else if (c == ' ') {
                x += 1.0*oneUnit;
            } else if (c == '6') {
                x += 4.0*oneUnit;
            } else if (c == '8' || c == '5') {
                // for graphics, 5 and 8 are the same (1/2 person size)
                x += 2.0*oneUnit;
            } else if (c == '9') {
                x += 3.0*oneUnit;
            }
        }
        // tidal waves or 2FL will hit this because there are no @'s
        if (x > longestx) {
            longestx = x;
        }

        float xSize = longestx;        // total width is always accurate
        float ySize = y + 4.0*oneUnit; // total height is always one person short
//        qDebug() << "final x/y sizes (pixels):" << xSize << ySize;

//        int xoffset = 180 - xSize/2.0;
//        int yoffset = 180 - ySize/2.0;
        int xoffset = this->width()/2 - xSize/2.0;
        int yoffset = this->height()/2 - ySize/2.0;
//        qDebug() << "final x/y offsets (pixels):" << xoffset << yoffset;

        // draw the renderArea ---------------------
        QRect r2(0, 0, this->width(), this->height());
        painter.setPen(QPen(Qt::black));
        painter.setBrush(QBrush(QColor(240,240,240)));
        painter.drawRect(r2);

        // draw the grid ---------------------
//        for (int i=0; i<360; i+=oneSpace) {
//            if (i == 180) {
//                continue;  // skip the center lines
//            }
//            painter.setPen(QPen(Qt::black,0.25,Qt::DotLine));
//            painter.drawLine(0,i,360,i);
//            painter.drawLine(i,0,i,360);
//        }

        for (int i=1; i<10; i++) {
            painter.setPen(QPen(Qt::black,0.25,Qt::DotLine));
            painter.drawLine(0,i*oneSpace + center.y(),this->width(),i*oneSpace + center.y());
            painter.drawLine(0,-i*oneSpace + center.y(),this->width(),-i*oneSpace + center.y());
            painter.drawLine(i*oneSpace + center.x(),0,i*oneSpace + center.x(),this->height());
            painter.drawLine(-i*oneSpace + center.x(),0,-i*oneSpace + center.x(),this->height());
        }

        // draw the center lines explictly
        //        painter.setPen(QPen(Qt::black,0.75,Qt::SolidLine));
        //        painter.drawLine(0,180,360,180);
        //        painter.drawLine(180,0,180,360);
        painter.setPen(QPen(Qt::black,0.75,Qt::SolidLine));
        painter.drawLine(0,center.y(),this->width(),center.y());
        painter.drawLine(center.x(),0,center.x(),this->height());

        // draw the people ----------------------
        int person = 0;
        x = y = 0.0;

        for (int i=0; i<layout1.length(); i++) {
            QChar c = layout1[i];
            QChar c2 = layout1[i+1];
            if (c == '@' && c2 == '7') {
                x = 0;
                y += 0.5*oneSpace;
                i++;  // consume the '7'
            } else if (c == '@' && c2 != '@') {
                x = 0;
                y += oneSpace;
            } else if (c == '@' && c2 == '@') {
                x = 0;
                y += oneSpace;
                i++;  // consume the second '@' for graphical plot
            } else if (c >= 'a' && c <= 'z') {
                QString personString = people[person];
//                qDebug() << "personString:" << personString;

                if (personString.length() < 3) {
                    if (personString == ".") {
                        // phantoms
                        QRect phantom(x + xoffset + 1.5*oneUnit, y + yoffset + 1.5*oneUnit,
                                oneUnit, oneUnit);  // EACH PHANTOM TAKES UP SPACE MUCH SMALLER THAN REGULAR PERSON
                        painter.setBrush(QBrush(Qt::yellow));
                        painter.drawEllipse(phantom);
                        person++;
                        x += 4.0*oneUnit;  // TEST
                        continue;  //
                    } else {
                        break;  // defensive programming
                    }
                }
                QString personNumber = personString.at(0);
                QString personGender = personString.at(1);
                QString personDirection = personString.at(2);

                QRect r(x + xoffset, y + yoffset,
                        4*oneUnit, 4*oneUnit);  // EACH PERSON TAKES UP SPACE SMALLER THAN ONESPACE

                if (personNumber=="1") {
                    if (personGender != "B" && mentalImageMode) {
                            // head girl is grey in mental image mode
                            painter.setBrush(QBrush(GREYCOUPLECOLOR));
                    } else {
                        painter.setBrush(QBrush(coupleColor[COUPLE1]));
                    }
                } else if (personNumber=="2") {
                    painter.setBrush(QBrush(coupleColor[COUPLE2]));
                } else if (personNumber=="3") {
                    painter.setBrush(QBrush(coupleColor[COUPLE3]));
                } else {
                    painter.setBrush(QBrush(coupleColor[COUPLE4]));
                }

                // draw the shape
                if (personGender == "B") {
                    painter.setPen(QPen(Qt::black,1.5,
                                        Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));  // outline pen style

                    painter.drawRect(r);
                } else {
                    painter.setPen(QPen(Qt::black,1.75,
                                        Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));  // outline pen style

                    painter.drawEllipse(r);
                }

                // draw a number
//                painter.setFont(QFont("Arial",formationFontSize,QFont::Bold));
#if defined(Q_OS_MAC)
                painter.setFont(QFont("Arial",11));
                int hOffset = 3;
                int vOffset = -4;
#elif defined(Q_OS_WIN)
                painter.setFont(QFont("Arial",8));
                int hOffset = 4;
                int vOffset = -5;
#else
                painter.setFont(QFont("Arial",8));
                int hOffset = 2;
                int vOffset = -3;
#endif
                QPointF textLoc = QPoint(x + xoffset + 2*oneUnit - hOffset,
                                         y + yoffset + 2*oneUnit - vOffset);
                painter.setPen(Qt::black);

                if (coupleColoringScheme == "Normal") {
                    // This is the only mode where we draw numbers
                    painter.drawText(textLoc, personNumber);
                }

                // draw the direction indicator
                painter.setPen(QPen(Qt::black,2.0,
                                    Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));  // outline pen style

                QPoint personCenter(x + 2*oneUnit + xoffset,
                                    y + 2*oneUnit + yoffset);
                if (personDirection == "<") {
                    painter.drawLine(personCenter + QPoint(-1.2*oneUnit,0), personCenter + QPoint(-2*oneUnit,0));
                } else if (personDirection == ">") {
                    painter.drawLine(personCenter + QPoint(1.2*oneUnit,0), personCenter + QPoint(2*oneUnit,0));
                } else if (personDirection == "V") {
                    painter.drawLine(personCenter+ QPoint(0,1.2*oneUnit), personCenter + QPoint(0,2*oneUnit));
                } else {
                    painter.drawLine(personCenter+ QPoint(0,-1.2*oneUnit), personCenter + QPoint(0,-2*oneUnit));
                }

                person++;
                x += 4.0*oneUnit;  // TEST
            } else if (c == ' ') {
                x += 1.0*oneUnit;
            } else if (c == '6') {
                x += 4.0*oneUnit;
            } else if (c == '8' || c == '5') {
                // for graphics, 5 and 8 are the same (1/2 person size)
                x += 2.0*oneUnit;
            } else if (c == '9') {
                x += 3.0*oneUnit;
            }
        }

        // draw the formation (if there is one)
#if defined(Q_OS_MAC)
        unsigned int formationFontSize = (24*this->width())/360;
#else
        unsigned int formationFontSize = (12*this->width())/360;
#endif
        if (formation != "") {
            painter.setFont(QFont("Arial",formationFontSize,QFont::Bold));
//            painter.drawText(190,30,formation);
            painter.drawText(0.52*this->width(),0.1*this->height(),formation);
        }
    }
}

// TODO: search messes us up, because it shows the final formation, which gets drawn. :-(
// TODO: Show a static square to start, before heads start.  (just initialize the layout strings properly!)

