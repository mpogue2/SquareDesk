/****************************************************************************
**
** Copyright (C) 2016, 2017 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk/SquareDeskPlayer application.
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

#ifndef MYSLIDER_H
#define MYSLIDER_H

#include <QSlider>
#include <QPainter>
#include <QPen>

// ---------------------------------------------
class MySlider : public QSlider
{

public:
    MySlider(QWidget *parent = 0);
    void SetLoop(bool b);           // turn on loop points
    void SetSingingCall(bool b);    // turn on singing call coloring
    void SetOrigin(int newOrigin);  // use an origin other than zero, when double-clicked
    void SetIntro(float intro);
    void SetOutro(float outro);
    float GetIntro() const;
    float GetOutro() const;
    void SetDefaultIntroOutroPositions();

    bool eventFilter(QObject *obj, QEvent *event);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);

private:
    bool drawLoopPoints;
    bool singingCall;
    float introPosition;
    float outroPosition;
    int origin;  // reset to this point when double-clicked

};

#endif // MYSLIDER_H

