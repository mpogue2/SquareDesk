/****************************************************************************
**
** Copyright (C) 2016-2021 Mike Pogue, Dan Lyke
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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utility.h"
//#include "renderarea.h"
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
#include <QCoreApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QClipboard>
#include <QtSvg/QSvgGenerator>
#include "common.h"
#include "danceprograms.h"
#include "../sdlib/database.h"
#include "sdformationutils.h"
#include <algorithm>  // for random_shuffle

//#define NO_TIMING_INFO 1

static const int kColCurrentSequenceCall = 0;
static const int kColCurrentSequenceFormation = 1;
#define NO_TIMING_INFO 1
#ifndef NO_TIMING_INFO
static const int kColCurrentSequenceTiming = 2;
#include "sdsequencecalllabel.h"
#endif


static const double dancerGridSize = 16;
static const double backgroundSquareCount = 8;
static const double gridSize = 32;
static const double halfBackgroundSize = gridSize * backgroundSquareCount / 2;
static const double currentSequenceIconSize = 64;
static int initialTableWidgetCurrentSequenceDefaultSectionSize = -1;
static const double sdListIconSize = 128;
static QBrush coupleColorBrushes[4] = { QBrush(COUPLE1COLOR),
                             QBrush(COUPLE2COLOR),
                             QBrush(COUPLE3COLOR),
                             QBrush(COUPLE4COLOR)
};
static QStringList initialDancerLocations;
static QHash<dance_level, QString> sdLevelEnumsToStrings;

static const char *str_exit_from_the_program = "exit from the program";
static const char *str_abort_this_sequence = "abort this sequence";
static const char *str_square_your_sets = "square your sets";
static const char *strBracketsSubsidiaryCall = "[SUBSIDIARY CALL]";
static const char *strLTAnythingGT = "<ANYTHING>";
static const char *str_undo_last_call = "undo last call";

static QFont dancerLabelFont;
static QString stringClickableCall("clickable call!");


static QGraphicsItemGroup *generateDancer(QGraphicsScene &sdscene, SDDancer &dancer, int number, bool boy)
{
    static QPen pen(Qt::black, 1.5, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);

    static double rectSize = 16;
    static QRectF rect(-rectSize/2, -rectSize/2, rectSize, rectSize);
    static QRectF directionRect(-2,-rectSize / 2 - 4,4,4);

//    QGraphicsItem *mainItem = boy ?
//                dynamic_cast<QGraphicsItem*>(sdscene.addRect(rect, pen, coupleColorBrushes[number]))
//        :   dynamic_cast<QGraphicsItem*>(sdscene.addEllipse(rect, pen, coupleColorBrushes[number]));

    // Now we have 2 items, and we only show one of them, depending on gender.  This allows for dynamic changing of gender via menu.
    QGraphicsItem *boyItem = dynamic_cast<QGraphicsItem*>(sdscene.addRect(rect, pen, coupleColorBrushes[number]));
    QGraphicsItem *girlItem = dynamic_cast<QGraphicsItem*>(sdscene.addEllipse(rect, pen, coupleColorBrushes[number]));

// HEXAGON style:
    QGraphicsPolygonItem *hex = new QGraphicsPolygonItem();
    QPolygonF hexagon;
    qreal side = rectSize/2;
    qreal dx = qSqrt(3)/2 * side;
    hexagon
        << QPointF(dx, -side/2)
        << QPointF(0, -side)
        << QPointF(-dx, -side/2)
        << QPointF(-dx, side/2)
        << QPointF(0, side)
        << QPointF(dx, side/2);
    hex->setPolygon(hexagon);
    hex->setPen(pen);
    QGraphicsItem *hexItem = dynamic_cast<QGraphicsItem*>(sdscene.addPolygon(hexagon, pen, coupleColorBrushes[number]));

    boyItem->setVisible(boy);
    girlItem->setVisible(!boy);
    hexItem->setVisible(false);  // always invisible to start

    QGraphicsRectItem *directionRectItem = sdscene.addRect(directionRect, pen, coupleColorBrushes[number]);

    QGraphicsTextItem *label = sdscene.addText(QString("%1").arg(number + 1),
                                               dancerLabelFont);

    QRectF labelBounds(label->boundingRect());
    QTransform labelTransform;
    dancer.labelTranslateX =-(labelBounds.left() + labelBounds.width() / 2);
    dancer.labelTranslateY = -(labelBounds.top() + labelBounds.height() / 2);
    labelTransform.translate(dancer.labelTranslateX,
                             dancer.labelTranslateY);
    label->setTransform(labelTransform);

    QList<QGraphicsItem*> items;
//    items.append(mainItem);
    items.append(boyItem);
    items.append(girlItem);
    items.append(hexItem);

    items.append(directionRectItem);
    items.append(label);
    QGraphicsItemGroup *group = sdscene.createItemGroup(items);
    dancer.graphics = group;
//    dancer.mainItem = mainItem;
    dancer.boyItem = boyItem;
    dancer.girlItem = girlItem;
    dancer.hexItem = hexItem;

    dancer.directionRectItem = directionRectItem;
    dancer.label = label;

    return group;
}

void move_dancers(QList<SDDancer> &sdpeople, double t)
{
    for (int dancerNum = 0; dancerNum < sdpeople.length(); dancerNum++)
    {
        QTransform transform;
        double dancer_x = sdpeople[dancerNum].getX(t);
        double dancer_y = sdpeople[dancerNum].getY(t);
        transform.translate( dancer_x * dancerGridSize,
                             dancer_y * dancerGridSize);
        transform.rotate(sdpeople[dancerNum].getDirection(t));
        sdpeople[dancerNum].graphics->setTransform(transform);

        // Gyrations to make sure that the text labels are always upright
        QTransform textTransform;
        textTransform.translate( -sdpeople[dancerNum].labelTranslateX,
                                 -sdpeople[dancerNum].labelTranslateY );
        textTransform.rotate( -sdpeople[dancerNum].getDirection(t));
        textTransform.translate( sdpeople[dancerNum].labelTranslateX,
                                 sdpeople[dancerNum].labelTranslateY );
        sdpeople[dancerNum].label->setTransform(textTransform); // Rotation(-sdpeople[dancerNum].direction);
    }
}

// remember for groupness and sequence check later
//
static struct dancer dancers[8];


// SEQUENCE ---------
//    returns InOrder, if boys/girls are in order
//            OutOfOrder, otherwise
//            Unknown, if there is no defined ordering for this gender (e.g. dancers are colinear)
void MainWindow::inOrder(struct dancer dancers[])
{
    getDancerOrder(dancers, &boyOrder, &girlOrder);

}

// GROUPNESS --------
int MainWindow::groupNum(struct dancer dancers[], bool top, int coupleNum, int gender) {
//    qDebug() << "searching: " << top << coupleNum << gender;
    for (int i = 0; i < 8; i++) {
        if (dancers[i].coupleNum == coupleNum && dancers[i].gender == gender) {
            if (top) {
//                qDebug() << "found: " << dancers[i].topside;
                return(dancers[i].topside);
            } else {
//                qDebug() << "found: " << dancers[i].leftside;
                return(dancers[i].leftside);
            }
        }
    }
    return(-1);  // get rid of warning
}

bool MainWindow::sameGroup(int group1, int group2) {
//    qDebug() << "checking: " << group1 << group2;
    return (group1 == group2 && group1 != -1 && group2 != -1);
}

int MainWindow::whichGroup (struct dancer dancers[], bool top) {
    double minx, maxx, miny, maxy;
    minx = miny = 99;
    maxx = maxy = -99;
    for (int i = 0; i < 8; i++) {
        minx = fmin(minx, dancers[i].x);
        miny = fmin(miny, dancers[i].y);
        maxx = fmax(maxx, dancers[i].x);
        maxy = fmax(maxy, dancers[i].y);
    }
//    qDebug() << "min x/y = " << minx << miny << ", max x/y = " << maxx << maxy;
    double dividerx = (minx + maxx)/2.0;  // find midpoint dividers of formation
    double dividery = (miny + maxy)/2.0;
    for (int i = 0; i < 8; i++) {
        dancers[i].leftside = (dancers[i].x < dividerx ? 1 : (dancers[i].x > dividerx ? 0 : -1)); // 1 = left, 0 = right, -1 = i dunno
        dancers[i].topside = (dancers[i].y < dividery ? 1 : (dancers[i].y > dividery ? 0 : -1)); // 1 = top, 0 = bottom, -1 = i dunno
//        qDebug() << dancers[i].coupleNum + 1 << (dancers[i].gender == 1 ? "G" : "B") << dancers[i].x << dancers[i].y << dancers[i].topside << dancers[i].leftside;
    }

    int group = -1;
    if ( sameGroup(groupNum(dancers, top, 0, 0), groupNum(dancers, top, 0, 1)) &&
         sameGroup(groupNum(dancers, top, 1, 0), groupNum(dancers, top, 1, 1)) )
    {
        // 1B and 1G in same top group, and 2B and 2G in same top group, so
//        qDebug() << "PARTNER GROUP";
        group = 0;

    } else if ( sameGroup(groupNum(dancers, top, 0, 0), groupNum(dancers, top, 1, 1)) &&
                sameGroup(groupNum(dancers, top, 1, 0), groupNum(dancers, top, 2, 1)) )
    {
        // 1B and 2G in same top group, 2B and 3G in same top group, so
//        qDebug() << "RIGHT HAND LADY GROUP";
        group = 1;
    } else if ( sameGroup(groupNum(dancers, top, 0, 0), groupNum(dancers, top, 2, 1)) &&
                sameGroup(groupNum(dancers, top, 1, 0), groupNum(dancers, top, 3, 1)) )
    {
        // 1B and 3G in same top group, 2B and 4G in same top group, so
//        qDebug() << "OPPOSITE GROUP";
        group = 2;
    } else if ( sameGroup(groupNum(dancers, top, 0, 0), groupNum(dancers, top, 3, 1)) &&
                sameGroup(groupNum(dancers, top, 1, 0), groupNum(dancers, top, 0, 1)) )
    {
        // 1B and 4G in same top group, 2B and 1G in same top group, so
//        qDebug() << "CORNER GROUP";
        group = 3;
    } else {
//        qDebug() << "NO GROUP";
    }

//    qDebug() << "TOP GROUP = " << group;
    return(group);
}

// also sets leftGroup, topGroup, boyOrder, girlOrder in MainWindow object
void MainWindow::decode_formation_into_dancer_destinations(
        const QStringList &sdformation,
        QList<SDDancer> &sdpeople)
{
    int coupleNumber = -1;
    int girl = 0;

    double max_y = static_cast<double>(sdformation.length());
    if (sdformation.first() != "") {
        max_y += 1;  // When double clicking on sequence, there will be "waves" or "8 chain" as an extra entry at beginning
                     //   by compensating for this, the whole diagram will no longer shift downward.
                     //   Don't ask me why it's +1 instead of -1.
    }

    // Assign each dancer a location based on the picture
    for (int y = 0; y < sdformation.length(); ++y)
    {
        int dancer_start_x = -1;
        bool draw = false;
        int direction = -1.0;

        for (int x = 0; x < sdformation[y].length(); ++x)
        {
            int ch = sdformation[y].at(x).unicode();
            switch (ch)
            {
            case '1' :
            case '2' :
            case '3' :
            case '4' :
                dancer_start_x = x;
                coupleNumber = ch - '1';
                break;
            case 'B':
                girl = 0;
                break;
            case 'G':
                girl = 1;
                break;
            case '<':
                draw = true;
                direction = 270;
                break;
            case '>':
                draw = true;
                direction = 90;
                break;
            case '^':
                draw = true;
                direction = 0;
                break;
            case 'V':
                draw = true;
                direction = 180;
                break;
            case ' ':
            case '.':
            case '\n':
                break;
            default:
                qDebug() << "Unknown character " << ch;
            }
            if (draw)
            {
                if (direction == -1 || coupleNumber == -1)
                {
                    qDebug() << "Drawing state error";
                }
                else
                {
                    int dancerNum = coupleNumber * 2 + girl;
                    if (dancerNum > sdpeople.length())
                    {
                        qDebug() << "Drawing state error dancer count";
                    }
                    else
                    {
                        sdpeople[dancerNum].setDestination(dancer_start_x, y, static_cast<int>(direction) );
                        QString gend = (girl == 1 ? "G" : "B");
//                        qDebug() << "Couple: " << coupleNumber + 1 << gend << ", x/y: " << dancer_start_x << "," << y;
                        dancers[dancerNum].coupleNum = coupleNumber;
                        dancers[dancerNum].gender = girl;
                        dancers[dancerNum].x = dancer_start_x;
                        dancers[dancerNum].y = y;
                        dancers[dancerNum].leftside = 0;
                        dancers[dancerNum].topside = 0;
                    }
                }
                draw = false;
                direction = -1;
                coupleNumber = -1;

            }
        } /* end of for x */
    } /* end of for y */

//    qDebug() << "END";

    // group-ness check ------------------------
//    qDebug() << "CHECKING FOR LEFT GROUP ------";
    leftGroup = whichGroup(dancers, false);

//    qDebug() << "CHECKING FOR TOP GROUP ------";
    topGroup = whichGroup(dancers, true);

//    qDebug() << "------------------------------";

//    qDebug() << "Left Group: " << *lGroup;  // save for later display
//    qDebug() << "Top Group: " << *tGroup;

    // FIX: this is inefficient because we calculate them both each time.
//    *bOrder = inOrder(dancers, Boys);
//    *gOrder = inOrder(dancers, Girls);

    // determine order/sequence of boys and girls, sets boyOrder, girlOrder
    inOrder(dancers);

//    qDebug() << "Boy order: " << *bOrder << ", Girl order: " << *gOrder;

    // -----------------------------------------

    // Now calculate the lowest common divisor for the X position and the scene bounds
    
    bool factors[4];
    double left_x = -1;

    // left_x = min(dancers.x)
    for (int dancerNum = 0; dancerNum < sdpeople.length(); dancerNum++)
    {
        int dancer_start_x = static_cast<int>(sdpeople[dancerNum].getX(1.0));
        if (left_x < 0 || dancer_start_x < left_x)
            left_x = dancer_start_x;
    }

    // calculate a lowest common divisor for X scaling
    for (size_t i = 0; i < sizeof(factors) / sizeof(*factors); ++i)
        factors[i] = true;
    int max_x = -1;

    for (int dancerNum = 0; dancerNum < sdpeople.length(); dancerNum++)
    {
        int dancer_start_x = static_cast<int>(sdpeople[dancerNum].getX(1) - left_x);
        if (dancer_start_x > max_x)
            max_x = dancer_start_x;

        for (size_t i = 0; i < sizeof(factors) / sizeof(*factors); ++i)
        {
            if (0 != (static_cast<unsigned long>(dancer_start_x) % (i + 3)))
            {
                factors[i] = false;
            }
        }
    }
    
    double lowest_factor = 2; // just default to something that won't suck too badly
    for (size_t i = 0; i < sizeof(factors) / sizeof(*factors); ++i)
    {
        if (factors[i])
        {
            lowest_factor = i + 3;
            break;
        }
    }

    max_x /= lowest_factor;
    
    for (int dancerNum = 0; dancerNum < sdpeople.length(); dancerNum++)
    {
        sdpeople[dancerNum].setDestinationScalingFactors(left_x, max_x, max_y, lowest_factor);
    }

}

void MainWindow::update_sd_animations()
{
    sd_animation_t_value += sd_animation_delta_t;
    if (sd_animation_t_value > 1.0)
        sd_animation_t_value = 1.0;
    move_dancers(sd_animation_people, sd_animation_t_value);
    if (sd_animation_t_value < 1.0)
    {
        QTimer::singleShot(static_cast<int>(sd_animation_msecs_per_frame), this, SLOT(update_sd_animations()));
        sd_animation_running = true;
    }
    else
    {
        sd_animation_running = false;
    }
}


static void initialize_scene(QGraphicsScene &sdscene, QList<SDDancer> &sdpeople,
                             QGraphicsTextItem *&graphicsTextItemSDStatusBarText,
                             QGraphicsTextItem *&graphicsTextItemSDLeftGroupText,
                             QGraphicsTextItem *&graphicsTextItemSDTopGroupText)
{
#if defined(Q_OS_MAC)
    dancerLabelFont = QFont("Arial",11);
#elif defined(Q_OS_WIN)
    dancerLabelFont = QFont("Arial",8);
#else
    dancerLabelFont = QFont("Arial",8);
#endif
    
    QPen backgroundPen(Qt::black);
    QPen gridPen(Qt::black,0.25,Qt::DotLine);
    QPen axisPen(Qt::darkGray,0.5);
    QBrush backgroundBrush(QColor(240,240,240));
    QRectF backgroundRect(-halfBackgroundSize,
                          -halfBackgroundSize,
                          halfBackgroundSize * 2,
                          halfBackgroundSize * 2);
    sdscene.addRect(backgroundRect, backgroundPen, backgroundBrush);

    graphicsTextItemSDStatusBarText = sdscene.addText("", dancerLabelFont);
    QTransform statusBarTransform;
    statusBarTransform.translate(-halfBackgroundSize, -halfBackgroundSize);
    statusBarTransform.scale(2,2);
    graphicsTextItemSDStatusBarText->setTransform(statusBarTransform);

    // groupness labels
    graphicsTextItemSDLeftGroupText = sdscene.addText("", dancerLabelFont);
    QTransform statusBarTransform2;
    statusBarTransform2.translate(-halfBackgroundSize/8, halfBackgroundSize*7/8);
//    statusBarTransform2.scale(2,2);
    graphicsTextItemSDLeftGroupText->setTransform(statusBarTransform2);

    graphicsTextItemSDTopGroupText = sdscene.addText("", dancerLabelFont);
    QTransform statusBarTransform3;
    statusBarTransform3.translate(-halfBackgroundSize, -halfBackgroundSize/8);
//    statusBarTransform3.scale(2,2);
    graphicsTextItemSDTopGroupText->setTransform(statusBarTransform3);

    for (double x = -halfBackgroundSize + gridSize;
         x < halfBackgroundSize; x += gridSize)
    {
        QPen &pen(x == 0.0 ? axisPen : gridPen);
        sdscene.addLine(x, -halfBackgroundSize, x, halfBackgroundSize, pen);
        sdscene.addLine(-halfBackgroundSize, x, halfBackgroundSize, x, pen);
    }

    for (int i = 0; i < 4; ++i)
    {
        QTransform boyTransform;
        boyTransform.rotate(-90*i);
        boyTransform.translate(-20, 50);

        QTransform girlTransform;
        girlTransform.rotate(-90*i);
        girlTransform.translate(20, 50);

        SDDancer boy, girl;

        QGraphicsItemGroup *boyGroup = generateDancer(sdscene, boy, i, true);
        QGraphicsItemGroup *girlGroup = generateDancer(sdscene, girl, i, false);

        boyGroup->setTransform(boyTransform);
        girlGroup->setTransform(girlTransform);

        sdpeople.append(boy);
        sdpeople.append(girl);
    }
}

void MainWindow::reset_sd_dancer_locations() {
    decode_formation_into_dancer_destinations(initialDancerLocations, sd_animation_people); // also sets L/Tgroup and B/Gorder
    decode_formation_into_dancer_destinations(initialDancerLocations, sd_fixed_people);     // also sets L/Tgroup and B/Gorder
    // Easiest way to make sure dest and source are the same
    // FIX: Why is this done twice??
    decode_formation_into_dancer_destinations(initialDancerLocations, sd_animation_people); // also sets L/Tgroup and B/Gorder
    decode_formation_into_dancer_destinations(initialDancerLocations, sd_fixed_people);     // also sets L/Tgroup and B/Gorder

    set_sd_last_groupness(); // update groupness strings

    move_dancers(sd_animation_people, 1.0);
    move_dancers(sd_fixed_people, 1.0);
}


void MainWindow::initialize_internal_sd_tab()
{
    sd_redo_stack->initialize();
    
    if (nullptr != shortcutSDTabUndo)
        delete shortcutSDTabUndo;
    shortcutSDTabUndo = new QShortcut(ui->tabSDIntegration);
    connect(shortcutSDTabUndo, SIGNAL(activated()), this, SLOT(undo_last_sd_action()));
    shortcutSDTabUndo->setKey(QKeySequence::Undo);

    if (nullptr != shortcutSDTabRedo)
        delete shortcutSDTabRedo;
    shortcutSDTabRedo = new QShortcut(ui->tabSDIntegration);
    connect(shortcutSDTabRedo, SIGNAL(activated()), this, SLOT(redo_last_sd_action()));
    shortcutSDTabRedo->setKey(QKeySequence::Redo);

   if (nullptr != shortcutSDCurrentSequenceSelectAll)
        delete shortcutSDCurrentSequenceSelectAll;
    shortcutSDCurrentSequenceSelectAll = new QShortcut(ui->tableWidgetCurrentSequence);
    connect(shortcutSDCurrentSequenceSelectAll, SIGNAL(activated()), this, SLOT(select_all_sd_current_sequence()));
    shortcutSDCurrentSequenceSelectAll->setKey(QKeySequence::SelectAll);

   if (nullptr != shortcutSDCurrentSequenceCopy)
        delete shortcutSDCurrentSequenceCopy;
    shortcutSDCurrentSequenceCopy = new QShortcut(ui->tableWidgetCurrentSequence);
    connect(shortcutSDCurrentSequenceCopy, SIGNAL(activated()), this, SLOT(copy_selection_from_tableWidgetCurrentSequence()));
    shortcutSDCurrentSequenceCopy->setKey(QKeySequence::Copy);
    

    ui->lineEditSDInput->setMainWindow(this);

    sdLastLineWasResolve = false;
    static QAction *danceProgramActionsStatic[] = {
        ui->actionSDDanceProgramMainstream,
        ui->actionSDDanceProgramPlus,
        ui->actionSDDanceProgramA1,
        ui->actionSDDanceProgramA2,
        ui->actionSDDanceProgramC1,
        ui->actionSDDanceProgramC2,
        ui->actionSDDanceProgramC3A,
        ui->actionSDDanceProgramC3,
        ui->actionSDDanceProgramC3x,
        ui->actionSDDanceProgramC4,
        ui->actionSDDanceProgramC4x,
        nullptr
    };
 
    danceProgramActions = new QAction *[sizeof(danceProgramActionsStatic) / sizeof(*danceProgramActionsStatic)];

    sdActionGroupDanceProgram = new QActionGroup(this);  // checker styles
    sdActionGroupDanceProgram->setExclusive(true);

    for (int i = 0; danceProgramActionsStatic[i]; ++i)
    {
        danceProgramActions[i] = danceProgramActionsStatic[i];
        QAction *action = danceProgramActions[i];
        sdActionGroupDanceProgram->addAction(action);
    }


    sdLevelEnumsToStrings[l_mainstream] = "Mainstream";
    sdLevelEnumsToStrings[l_plus] = "Plus";
    sdLevelEnumsToStrings[l_a1] = "A1";
    sdLevelEnumsToStrings[l_a2] = "A2";
    sdLevelEnumsToStrings[l_c1] = "C1";
    sdLevelEnumsToStrings[l_c2] = "C2";
    sdLevelEnumsToStrings[l_c3a] = "C3a";
    sdLevelEnumsToStrings[l_c3] = "C3";
    sdLevelEnumsToStrings[l_c3x] = "C3x";
    sdLevelEnumsToStrings[l_c4a] = "C4a";
    sdLevelEnumsToStrings[l_c4] = "C4";
    sdLevelEnumsToStrings[l_c4x] = "C4x";
    sdLevelEnumsToStrings[l_dontshow] = "Dontshow";
    sdLevelEnumsToStrings[l_nonexistent_concept] = "Nonexistent_concept";


    
    ui->listWidgetSDOutput->setIconSize(QSize(128,128));

    initialize_scene(sd_animation_scene, sd_animation_people, graphicsTextItemSDStatusBarText_animated,
                     graphicsTextItemSDLeftGroupText_animated, graphicsTextItemSDTopGroupText_animated);
    initialize_scene(sd_fixed_scene, sd_fixed_people, graphicsTextItemSDStatusBarText_fixed,
                     graphicsTextItemSDLeftGroupText_fixed, graphicsTextItemSDTopGroupText_fixed);
    ui->graphicsViewSDFormation->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    ui->graphicsViewSDFormation->setScene(&sd_animation_scene);

    initialDancerLocations.append("");
    initialDancerLocations.append("  .    3GV   3BV    .");
    initialDancerLocations.append("");
    initialDancerLocations.append(" 4B>    .     .    2G<");
    initialDancerLocations.append("");
    initialDancerLocations.append(" 4G>    .     .    2B<");
    initialDancerLocations.append("");
    initialDancerLocations.append("  .    1B^   1G^    .");
    reset_sd_dancer_locations();
    sd_animation_t_value = 1.0;

    QStringList tableHeader;
    tableHeader << "call" << "result";
    ui->tableWidgetCurrentSequence->setHorizontalHeaderLabels(tableHeader);
    ui->tableWidgetCurrentSequence->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->tableWidgetCurrentSequence->horizontalHeader()->setVisible(false);
    ui->tableWidgetCurrentSequence->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableWidgetCurrentSequence->setColumnWidth(1, static_cast<int>(currentSequenceIconSize) + 8);
    QHeaderView *verticalHeader = ui->tableWidgetCurrentSequence->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    initialTableWidgetCurrentSequenceDefaultSectionSize =
        verticalHeader->defaultSectionSize();
    verticalHeader->setDefaultSectionSize(static_cast<int>(currentSequenceIconSize));
}


void MainWindow::on_threadSD_errorString(QString /* str */)
{
}

void MainWindow::on_sd_set_window_title(QString /* str */)
{
}

void MainWindow::set_sd_last_formation_name(const QString &str)
{
    sdLastFormationName = str;

    // get rid of "resolve: N out of M" --> "Resolve" (so text field doesn't grow without bound)
    QRegularExpression re("resolve: \\d+ out of \\d+", QRegularExpression::CaseInsensitiveOption);
    sdLastFormationName.replace(re, "Resolve");

    QString boyOrderString, girlOrderString;
    switch (boyOrder) {
        case InOrder:
            boyOrderString = "in";
            break;
        case OutOfOrder:
            boyOrderString = "out";
            break;
        default:
            boyOrderString = "?";
            break;
    }
    switch (girlOrder) {
        case InOrder:
            girlOrderString = "in";
            break;
        case OutOfOrder:
            girlOrderString = "out";
            break;
        default:
            girlOrderString = "?";
            break;
    }
    QString orderString = "[B:" + boyOrderString + " G:" + girlOrderString + "]";

    bool showOrderSequence = prefsManager.Getenableordersequence();

    if ( sdLastFormationName == "<startup>" ||
         sdLastFormationName == "(any setup)" ) {
        if (showOrderSequence) {
            graphicsTextItemSDStatusBarText_fixed->setPlainText(orderString);
            graphicsTextItemSDStatusBarText_animated->setPlainText(orderString);
        } else {
            graphicsTextItemSDStatusBarText_fixed->setPlainText("");
            graphicsTextItemSDStatusBarText_animated->setPlainText("");
        }
    } else {
        if (showOrderSequence) {
            graphicsTextItemSDStatusBarText_fixed->setPlainText(sdLastFormationName + " " + orderString);
            graphicsTextItemSDStatusBarText_animated->setPlainText(sdLastFormationName + " " + orderString);
        } else {
            graphicsTextItemSDStatusBarText_fixed->setPlainText(sdLastFormationName);
            graphicsTextItemSDStatusBarText_animated->setPlainText(sdLastFormationName);
        }
    }
}

void MainWindow::set_sd_last_groupness() {
//    qDebug() << "updating SD groupness...";

    QString leftGroupStr, topGroupStr;
    switch (leftGroup) {
        case 0: leftGroupStr = "P"; break;
        case 1: leftGroupStr = "R"; break;
        case 2: leftGroupStr = "O"; break;
        case 3: leftGroupStr = "C"; break;
        default: leftGroupStr = "?"; break;
    }
    switch (topGroup) {
        case 0: topGroupStr = "P"; break;
        case 1: topGroupStr = "R"; break;
        case 2: topGroupStr = "O"; break;
        case 3: topGroupStr = "C"; break;
        default: topGroupStr = "?"; break;
    }

//    qDebug() << "Groupness strings updated to L/T: " << leftGroupStr << topGroupStr;

    if (graphicsTextItemSDLeftGroupText_fixed != nullptr) {
        if (ui->actionShow_group_station->isChecked()) {
            graphicsTextItemSDLeftGroupText_fixed->setPlainText(leftGroupStr);
        } else {
            graphicsTextItemSDLeftGroupText_fixed->setPlainText("");
        }
    }

    if (graphicsTextItemSDLeftGroupText_animated != nullptr) {
        if (ui->actionShow_group_station->isChecked()) {
            graphicsTextItemSDLeftGroupText_animated->setPlainText(leftGroupStr);
        } else {
            graphicsTextItemSDLeftGroupText_animated->setPlainText("");
        }
    }

    if (graphicsTextItemSDTopGroupText_fixed != nullptr) {
        if (ui->actionShow_group_station->isChecked()) {
            graphicsTextItemSDTopGroupText_fixed->setPlainText(topGroupStr);
        } else {
            graphicsTextItemSDTopGroupText_fixed->setPlainText("");
        }
    }

    if (graphicsTextItemSDTopGroupText_animated != nullptr) {
        if (ui->actionShow_group_station->isChecked()) {
            graphicsTextItemSDTopGroupText_animated->setPlainText(topGroupStr);
        } else {
            graphicsTextItemSDTopGroupText_animated->setPlainText("");
        }
    }
}

//void MainWindow::set_sd_last_order(Order bOrder, Order gOrder) {
////    qDebug() << "updating SD order..." << bOrder << "," << gOrder;
////    boyOrder = bOrder;
////    girlOrder = gOrder;
//}

void MainWindow::SetAnimationSpeed(AnimationSpeed speed)
{
    switch (speed)
    {
    case AnimationSpeedSlow :
        sd_animation_delta_t = .05;
        sd_animation_msecs_per_frame = 50;
        break;
    case AnimationSpeedMedium:
        sd_animation_delta_t = .1;
        sd_animation_msecs_per_frame = 50;
        break;
    case AnimationSpeedFast:
        sd_animation_delta_t = .2;
        sd_animation_msecs_per_frame = 50;
        break;
        
    case AnimationSpeedOff:
//    default:  // all enums are explicit, no need for default
        sd_animation_delta_t = 1;
        sd_animation_msecs_per_frame = 0;
    }

    if (sd_animation_msecs_per_frame <= 0)
        sd_animation_msecs_per_frame = 1;
    if (sd_animation_msecs_per_frame >= 1000)
        sd_animation_msecs_per_frame = 1;
    if (sd_animation_delta_t < .01)
        sd_animation_delta_t = .01;
    if (sd_animation_delta_t > 1)
        sd_animation_delta_t = 1;
            
//    qDebug() << "Animation speed: " << speed << " :" << sd_animation_delta_t << "/" << sd_animation_msecs_per_frame;
}

void MainWindow::on_sd_update_status_bar(QString str)
{
//    qDebug() << "updating SD status bar...";
    if (!str.compare("<startup>"))
    {
        sdformation = initialDancerLocations;
    }
    set_sd_last_formation_name(str);
    QString formation(sdLastFormationName +
                      "\n" + sdformation.join("\n"));
    if (!sdformation.empty())
    {
        sd_redo_stack->checkpoint_formation(sdLastLine, formation);

        decode_formation_into_dancer_destinations(sdformation, sd_animation_people);     // also sets L/Tgroup and B/Gorder
        decode_formation_into_dancer_destinations(sdformation, sd_fixed_people);         // also sets L/Tgroup and B/Gorder

        set_sd_last_groupness(); // update groupness strings
//        set_sd_last_order(boyOrder, girlOrder);     // update order strings

        set_sd_last_formation_name(str);  // this must be last to pull in the order strings (hack)

        sd_animation_t_value = sd_animation_delta_t;

        move_dancers(sd_fixed_people, 1);
        QPixmap image(static_cast<int>(sdListIconSize), static_cast<int>(sdListIconSize));
        image.fill();
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        sd_fixed_scene.render(&painter);

        QListWidgetItem *item = new QListWidgetItem();
        item->setData(Qt::UserRole, formation);
        item->setIcon(QIcon(image));
        ui->listWidgetSDOutput->addItem(item); 
        
        update_sd_animations();
   }
    if (!sdformation.empty() && sdLastLine >= 1)
    {
        int row = sdLastLine >= 2 ? (sdLastLine - 2) : 0;

        move_dancers(sd_fixed_people, 1);
        render_current_sd_scene_to_tableWidgetCurrentSequence(row, formation);
#ifdef NO_TIMING_INFO
        QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row, kColCurrentSequenceCall);
        item->setData(Qt::UserRole, QVariant(formation));
#endif /* ifdef NO_TIMING_INFO */
        /* ui->listWidgetSDOutput->addItem(sdformation.join("\n")); */
    }
//    sdformation.clear(); // not needed.  If here, it prevents updates when group menu item is toggled.
}


void MainWindow::sd_begin_available_call_list_output()
{
    sdOutputtingAvailableCalls = true;
    sdAvailableCalls.clear();
}
void MainWindow::sd_end_available_call_list_output()
{
    dance_level current_dance_program = get_current_sd_dance_program();
    
    sdOutputtingAvailableCalls = false;
    QString available("Available calls:");
    QListWidgetItem *availableItem = new QListWidgetItem(available);
    QFont availableItemFont(availableItem->font());
    availableItemFont.setBold(true);
    availableItem->setFont(availableItemFont);
    ui->listWidgetSDOutput->addItem(availableItem);
    ui->listWidgetSDQuestionMarkComplete->clear();
    
    for (auto available_call : sdAvailableCalls)
    {
        QString call_text(available_call.call_name);
        if (sdLevelEnumsToStrings.end() != sdLevelEnumsToStrings.find(available_call.dance_program))
        {
            if (available_call.dance_program != l_nonexistent_concept)
            {
                call_text += " - " + sdLevelEnumsToStrings[available_call.dance_program];
            }
        }
        else
        {
            call_text += QString(" - (%0)").arg(available_call.dance_program);
        }
        if (((available_call.dance_program <= current_dance_program)
             || (available_call.dance_program == l_nonexistent_concept))
            && !call_text.startsWith("_"))
        {
            ui->listWidgetSDQuestionMarkComplete->addItem(call_text);
            QListWidgetItem *callItem = new QListWidgetItem(call_text);
            callItem->setData(Qt::UserRole, stringClickableCall);
            ui->listWidgetSDOutput->addItem(callItem);
        }
    }
    ui->tabWidgetSDMenuOptions->setCurrentIndex(1);
}




void MainWindow::highlight_sd_replaceables()
{
    QString call = ui->lineEditSDInput->text();

    if (!(call.contains("<") && call.contains(">")))
    {
//        qDebug() << "No <>, return pressed";
        submit_lineEditSDInput_contents_to_sd();
        ui->lineEditSDInput->setFocus();
    }
    else
    {
        int lessThan = call.indexOf("<");
        int greaterThan = call.indexOf(">");
        ui->lineEditSDInput->setSelection(lessThan, greaterThan - lessThan + 1);
        ui->lineEditSDInput->setFocus();
        QString selectedText = ui->lineEditSDInput->selectedText();

        ui->listWidgetSDAdditionalOptions->clear();
        if (selectedText == "<N>" /* /8 */)
        {
            for (int i = 1; i <= 8; ++i)
            {
                ui->listWidgetSDAdditionalOptions->addItem(QString("%1").arg(i));
            }
            ui->tabWidgetSDMenuOptions->setCurrentIndex(2);
        }
        if (selectedText == "<Nth>")
        {
            ui->listWidgetSDAdditionalOptions->addItem("1st");
            ui->listWidgetSDAdditionalOptions->addItem("2nd");
            ui->listWidgetSDAdditionalOptions->addItem("3rd");
            ui->listWidgetSDAdditionalOptions->addItem("4th");
            ui->tabWidgetSDMenuOptions->setCurrentIndex(2);
        }
        if (selectedText == "<N/4>")
        {
            for (int i = 1; i <= 4; ++i)
            {
                ui->listWidgetSDAdditionalOptions->addItem(QString("%1/4").arg(i));
            }
            ui->tabWidgetSDMenuOptions->setCurrentIndex(2);
        }
        if (selectedText == "<DIRECTION>")
        {
            sdthread->add_directions_to_list_widget(ui->listWidgetSDAdditionalOptions);
            ui->tabWidgetSDMenuOptions->setCurrentIndex(2);
        }
        if (selectedText == "<ATC>")
        {
            submit_lineEditSDInput_contents_to_sd();
        }
        if (selectedText == "<ANYCIRC>")
        {
            submit_lineEditSDInput_contents_to_sd();
        }
        if (selectedText == "<ANYTHING>")
        {
            ui->lineEditSDInput->insert("");
            submit_lineEditSDInput_contents_to_sd();
        }
        if (selectedText == "<ANYONE>")
        {
            sdthread->add_selectors_to_list_widget(ui->listWidgetSDAdditionalOptions);
            ui->tabWidgetSDMenuOptions->setCurrentIndex(2);
        }
    }
}

void MainWindow::on_listWidgetSDAdditionalOptions_itemDoubleClicked(QListWidgetItem *item)
{
    QString call = ui->lineEditSDInput->text();
    if (call.contains("<") && call.contains(">"))
    {
        int lessThan = call.indexOf("<");
        int greaterThan = call.indexOf(">");
        call.replace(lessThan, greaterThan - lessThan + 1,
                     item->text());
        ui->lineEditSDInput->setText(call);
    }
    else
    {
        ui->lineEditSDInput->insert(item->text());
    }
    ui->tabWidgetSDMenuOptions->setCurrentIndex(0);
    highlight_sd_replaceables();
}

void MainWindow::on_listWidgetSDQuestionMarkComplete_itemDoubleClicked(QListWidgetItem *item)
{
    do_sd_double_click_call_completion(item);
}

QString toCamelCase(const QString& s)
{
    QStringList parts = s.split(' ', Qt::SkipEmptyParts);
    for (int i = 0; i < parts.size(); ++i)
        parts[i].replace(0, 1, parts[i][0].toUpper());

    return parts.join(" ");
}

QString MainWindow::prettify(QString call) {
    QString call1 = toCamelCase(call);  // HEADS square thru 4 --> Heads Square Thru 4

    // from Mike's process_V1.5.R
    // ALL CAPS the who of each line (the following explicitly defined terms, at the beginning of a line)
    // This matches up with what Mike is doing for Ceder cards.
    QRegularExpression who("^(Center Box|Center Lady|That Boy|That Girl|Those Girls|Those Boys|Points|On Each Side|Center Line Of 4|New Outsides|Each Wave|Outside Boys|Lines Of 3 Boys|Side Boys|Side Ladies|Out-Facing Girls|Line Of 8|Line Of 3|Lead Boy|Lead Girl|Lead Boys|Just The Boys|Heads In Your Box|Sides In Your Box|All Four Ladies|Four Ladies|Four Girls|Four Boys|Center Girls|Center Four|All 8|Both|Side Position|Same Girl|Couples 1 and 2|Head Men and Corner|Outer 4|Outer 6|Couples|New Couples 1 and 3|New Couples 1 and 4|Couples 1 & 2|Ladies|Head Man|Head Men|Lead Couple|Men|Those Who Can|New Ends|End Ladies|Other Ladies|Lines Of 3|Waves Of 3|All 4 Girls|All 4 Ladies|Each Side Boys|Those Boys|Each Side Centers|Center 4|Center 6|Center Boys|Center Couples|Center Diamond|Center Boy|Center Girl|Center Wave|Center Line|All Eight|Other Boy|Other Girl|Centers Only|Ends Only|Outside Boy|All The Boys|All The Girls|Centers|Ends|All 8|Boys Only|Girls Only|Boys|Girls|Heads|Sides|All|New Centers|Wave Of 6|Others|Outsides|Leaders|Side Boy|4 Girls|4 Ladies|Very Center Boy|Very Center Boys|Very End Boys|Very Centers|Very Center Girls|Those Facing|Those Boys|Head Position|Head Boys|Head Ladies|End Boy|End Girl|Everybody|Each Side|4 Boys|4 Girls|Same 4)");
    QRegularExpressionMatch match = who.match(call1);
    if (match.hasMatch()) {
        QString matched = match.captured(0);
        call1.replace(match.capturedStart(), match.capturedLength(), matched.toUpper());  // replace the match with the toUpper of the match
    }

    return(call1);
}

void MainWindow::on_sd_add_new_line(QString str, int drawing_picture)
{
    if (sdOutputtingAvailableCalls)
    {
        SDAvailableCall available_call;
        available_call.call_name = str.simplified();
        available_call.dance_program = sdthread->find_dance_program(str);
        sdAvailableCalls.append(available_call);
        return;
    }
    
    if (!drawing_picture)
    {
        str = str.trimmed();
        if (str.isEmpty())
            return;
        if (str == "(no matches)"
            || str == "SUBSIDIARY CALL")
        {
            ui->label_SD_Resolve->setText(str);
        }
        if (sdLastLineWasResolve)
        {
            ui->label_SD_Resolve->setText(str);
            sdLastLineWasResolve = false;
        }
        if (str.startsWith("resolve is:"))
        {
            sdLastLineWasResolve = true;
        }
    }
    else
    {
        ui->label_SD_Resolve->setText("");
    }

    if (str.startsWith("Output file is \""))
    {
        sdLastLine = 0;
        sd_redo_stack->initialize();
    }

    while (str.length() > 1 && str[str.length() - 1] == '\n')
        str = str.left(str.length() - 1);

    if (drawing_picture)
    {
        if (sdWasNotDrawingPicture)
        {
            sdformation.clear();
        }
        sdWasNotDrawingPicture = false;
        sdformation.append(str);
    }
    else
    {
        sdWasNotDrawingPicture = true;
        static QRegularExpression regexMove("^\\s*(\\d+)\\:\\s*(.*)$");
        QRegularExpressionMatch match = regexMove.match(str);
        if (match.hasMatch())
        {
            QString move = match.captured(2).trimmed();
            sdHasSubsidiaryCallContinuation = false;
            if (move.contains("[") && !move.contains("]")) {
                sdHasSubsidiaryCallContinuation = true;
            }
            sdLastLine = match.captured(1).toInt();
            if (sdLastLine > 0 && !move.isEmpty())
            {
                if (ui->tableWidgetCurrentSequence->rowCount() < sdLastLine)
                {
                    ui->tableWidgetCurrentSequence->setRowCount(sdLastLine);
                }

#ifdef NO_TIMING_INFO
                QString theCall = match.captured(2);
                QString thePrettifiedCall = prettify(theCall);

                QTableWidgetItem *moveItem(new QTableWidgetItem(thePrettifiedCall));
                moveItem->setFlags(moveItem->flags() & ~Qt::ItemIsEditable);
                ui->tableWidgetCurrentSequence->setItem(sdLastLine - 1, kColCurrentSequenceCall, moveItem);
#else
                QString lastCall(match.captured(2).toHtmlEscaped());
                int longestMatchLength = 0;
                QString callTiming;
                
                for (int i = 0 ; danceprogram_callinfo[i].name; ++i)
                {n
                    if (danceprogram_callinfo[i].timing)
                    {
                        QString thisCallName(danceprogram_callinfo[i].name);
                        if (lastCall.contains(thisCallName, Qt::CaseInsensitive)
                            && thisCallName.length() > longestMatchLength)
                        {
                            longestMatchLength = thisCallName.length();
                            callTiming = danceprogram_callinfo[i].timing;
                        }
                    }
                }
                if (!callTiming.isEmpty())
                {
//                    lastCall += "\n<br><small>&nbsp;&nbsp;" + callTiming + "</small>";
                }
                QLabel *moveLabel(new SDSequenceCallLabel(this));
//                moveLabel->setTextFormat(Qt::RichText);
//                qDebug() << "Setting move label to " << lastCall;
                moveLabel->setText(lastCall);
                ui->tableWidgetCurrentSequence->setCellWidget(sdLastLine - 1, kColCurrentSequenceCall, moveLabel);
#endif
            }
            else if (str[0] == '[')
            {
                QTableWidgetItem *moveItem(ui->tableWidgetCurrentSequence->item(sdLastLine - 1, kColCurrentSequenceCall));
                QString lastCall(moveItem->text());
                lastCall += " " + str;
//                qDebug() << "Appending additional call info " << lastCall;
                moveItem->setText(lastCall);
            }
        } else if (sdHasSubsidiaryCallContinuation) {
                QTableWidgetItem *moveItem(ui->tableWidgetCurrentSequence->item(sdLastLine - 1, kColCurrentSequenceCall));
                QString lastCall(moveItem->text());
                lastCall += " " + str.trimmed();
//                qDebug() << "Appending additional call info " << lastCall;
                moveItem->setText(lastCall);
                sdHasSubsidiaryCallContinuation = false;
        }

        // Drawing the people happens over in on_sd_update_status_bar
        
        ui->listWidgetSDOutput->addItem(str);
    }
}


void MainWindow::render_sd_item_data(QTableWidgetItem *item)
{
        QVariant v = item->data(Qt::UserRole);
        if (!v.isNull())
        {
            QString formation(v.toString());
            QStringList formationList = formation.split("\n");
            if (formationList.size() > 0)
            {
                decode_formation_into_dancer_destinations(formationList, sd_animation_people);  // also sets L/Tgroup and B/Gorder
                set_sd_last_groupness(); // update groupness strings
//                set_sd_last_order(boyOrder, girlOrder);     // update order strings

                set_sd_last_formation_name(formationList[0]); // must be last
                formationList.removeFirst();

                move_dancers(sd_animation_people, 1);
            }
        }
}

void MainWindow::on_sd_awaiting_input()
{
    ui->listWidgetSDOutput->scrollToBottom();
    int rowCount = sdLastLine > 1 ? sdLastLine - 1 : sdLastLine;
//    qDebug() << "on_sd_awaiting_input setting row count to " << rowCount << " vs " << ui->tableWidgetCurrentSequence->rowCount();

    // In failed subsidiary call cases we don't get a formation output, so we need to manually reset the formation display.
    if (ui->tableWidgetCurrentSequence->rowCount() > rowCount
        && rowCount > 0 && !sd_animation_running)
    {
        QTableWidgetItem *item(ui->tableWidgetCurrentSequence->item(rowCount - 1, kColCurrentSequenceFormation));
        render_sd_item_data(item);
    }
    ui->tableWidgetCurrentSequence->setRowCount(rowCount);
    ui->tableWidgetCurrentSequence->scrollToBottom();
    on_lineEditSDInput_textChanged();
}

void MainWindow::on_sd_set_pick_string(QString str)
{
    Q_UNUSED(str)
//    qDebug() << "on_sd_set_pick_string: " <<  str;
}

void MainWindow::on_sd_dispose_of_abbreviation(QString str)
{
    Q_UNUSED(str)
//    qDebug() << "on_sd_dispose_of_abbreviation: " <<  str;
}


void MainWindow::on_sd_set_matcher_options(QStringList options, QStringList levels)
{
    ui->listWidgetSDOptions->clear();
    
    for (int i = 0; i < options.length(); ++i)
    {
        // if it's not "exit the program"
        if (options[i].compare(str_exit_from_the_program, Qt::CaseInsensitive))
        {
            QListWidgetItem *item = new QListWidgetItem(options[i]);
            QVariant v(levels[i].toInt());
            item->setData(Qt::UserRole, v);
            ui->listWidgetSDOptions->addItem(item);
        }
    }
    ui->tabWidgetSDMenuOptions->setCurrentIndex(0);

}


void MainWindow::on_tableWidgetCurrentSequence_itemDoubleClicked(QTableWidgetItem *item)
{
    render_sd_item_data(item);
}

void MainWindow::render_current_sd_scene_to_tableWidgetCurrentSequence(int row, const QString &formation)
{
    QPixmap image(static_cast<int>(currentSequenceIconSize), static_cast<int>(currentSequenceIconSize));
    image.fill();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    sd_fixed_scene.render(&painter);

    QTableWidgetItem *item = new QTableWidgetItem();
    item->setData(Qt::UserRole, formation);
    item->setData(Qt::DecorationRole,image);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);

    ui->tableWidgetCurrentSequence->setItem (row, kColCurrentSequenceFormation, item);
#ifdef NO_TIMING_INFO
    QTableWidgetItem *textItem = ui->tableWidgetCurrentSequence->item(row,kColCurrentSequenceCall);
    textItem->setData(Qt::UserRole, QVariant(formation));
#endif
}

void MainWindow::set_current_sequence_icons_visible(bool visible)
{
    if (visible)
    {
        QHeaderView *verticalHeader = ui->tableWidgetCurrentSequence->verticalHeader();
        verticalHeader->setDefaultSectionSize(static_cast<int>(currentSequenceIconSize));
    }
    else
    {
        QHeaderView *verticalHeader = ui->tableWidgetCurrentSequence->verticalHeader();
        verticalHeader->setDefaultSectionSize(initialTableWidgetCurrentSequenceDefaultSectionSize);
    }
    ui->tableWidgetCurrentSequence->setColumnHidden(kColCurrentSequenceFormation,!visible);
}

void MainWindow::do_sd_double_click_call_completion(QListWidgetItem *item)
{
    QString call(item->text());
    QStringList calls = call.split(" - ");
    if (calls.size() > 1)
    {
        call = calls[0];
    }
//    qDebug() << "Double click " << call;
    ui->lineEditSDInput->setText(call);
//    qDebug() << "Highlighting replaceables";
    highlight_sd_replaceables();
}

void MainWindow::sdSequenceCallLabelDoubleClicked(QMouseEvent * /* event */)
{
    QItemSelectionModel *selectionModel = ui->listWidgetSDOutput->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
        QTableWidgetItem *textItem(ui->tableWidgetCurrentSequence->item(row,kColCurrentSequenceFormation));
        on_tableWidgetCurrentSequence_itemDoubleClicked(textItem);
    }
    else {
        // more than 1 row or no rows at all selected (BAD)
    }
    
}


void MainWindow::on_listWidgetSDOutput_itemDoubleClicked(QListWidgetItem *item)
{
    QVariant v = item->data(Qt::UserRole);
    if (!v.isNull())
    {
        QString formation(v.toString());
        if (!formation.compare(stringClickableCall))
        {
            do_sd_double_click_call_completion(item);
        }
        else
        {
            QStringList formationList = formation.split("\n");
            if (formationList.size() > 0)
            {
                decode_formation_into_dancer_destinations(formationList, sd_animation_people);   // also sets L/Tgroup and B/Gorder
                set_sd_last_groupness(); // update groupness strings
//                set_sd_last_order(boyOrder, girlOrder); // update groupness strings

                set_sd_last_formation_name(formationList[0]); // must be last
                formationList.removeFirst();

                move_dancers(sd_animation_people, 1);
            }
        }

    }
}

void MainWindow::on_listWidgetSDOptions_itemDoubleClicked(QListWidgetItem *item)
{
    do_sd_double_click_call_completion(item);
}

// WARNING: making these statically allocated makes this not thread safe, because we use the matchedLength() property!
// NOTE: now dynamically allocated, now that we're using QRegularExpression.

static QRegularExpression regexEnteredNth("(\\d+(st|nd|rd|th)?)");
static QRegularExpression regexCallNth("<Nth>");
static QRegularExpression regexEnteredN("(\\d+)");
static QRegularExpression regexCallN("<N>");

static int compareEnteredCallToCall(const QString &enteredCall, const QString &call, QString *maxCall = nullptr)
{
    
    int enteredCallPos = 0;
    int callPos = 0;

    QRegularExpressionMatch enteredCallMatch;
    QRegularExpressionMatch callMatch;

    while (enteredCallPos < enteredCall.length() && callPos < call.length())
    {
        if (enteredCall.at(enteredCallPos).toUpper() == call.at(callPos).toUpper())
        {
            enteredCallPos ++;
            callPos ++;
        }
//        else if (enteredCallPos == enteredCall.indexOf(regexEnteredNth, enteredCallPos)
//                 && callPos == call.indexOf(regexCallNth, callPos))
//        {
//            enteredCallPos += regexEnteredNth.matchedLength();
//            callPos += regexCallNth.matchedLength();
//        }
//        else if (enteredCallPos == enteredCall.indexOf(regexEnteredN, enteredCallPos)
//                 && callPos == call.indexOf(regexCallN, callPos))
//        {
//            enteredCallPos += regexEnteredN.matchedLength();
//            callPos += regexCallN.matchedLength();
//        }
        else if (enteredCallPos == enteredCall.indexOf(regexEnteredNth, enteredCallPos, &enteredCallMatch)
                 && callPos == call.indexOf(regexCallNth, callPos, &callMatch))
        {
            enteredCallPos += enteredCallMatch.capturedLength();
            callPos += callMatch.capturedLength();
        }
        else if (enteredCallPos == enteredCall.indexOf(regexEnteredN, enteredCallPos, &enteredCallMatch)
                 && callPos == call.indexOf(regexCallN, callPos, &callMatch))
        {
            enteredCallPos += enteredCallMatch.capturedLength();
            callPos += callMatch.capturedLength();
        }
        else
        {
            break;
        }
    }
    
    if (nullptr != maxCall)
    {
        *maxCall = enteredCall.left(enteredCallPos) + call.right(call.length() - callPos);
    }
    return enteredCallPos;
}

static QString get_longest_match_from_list_widget(QListWidget *listWidget,
                                                  QString originalText,
                                                  QString callSearch,
                                                  QString longestMatch = QString())
{
    QString substitutedLongestMatchCallSearch;
    QString substitutedLongestMatchOriginalText;
    
    for (int i = 0; i < listWidget->count(); ++i)
    {
        int callSearchMatchLength = compareEnteredCallToCall(callSearch, listWidget->item(i)->text(), &substitutedLongestMatchCallSearch);
        int enteredCallMatchLength = compareEnteredCallToCall(originalText, listWidget->item(i)->text(), &substitutedLongestMatchOriginalText);
                                                         
        if ((!listWidget->isRowHidden(i))
            &&
            (callSearch.length() == callSearchMatchLength
             || originalText.length() == enteredCallMatchLength)
            )
        {
            if (longestMatch.isEmpty())
            {
                longestMatch = (callSearch.length() == callSearchMatchLength) ? substitutedLongestMatchCallSearch : substitutedLongestMatchOriginalText;
            }
            else
            {
                QString thisItem = (callSearch.length() == callSearchMatchLength) ? substitutedLongestMatchCallSearch : substitutedLongestMatchOriginalText;

                int substringMatch = 0;
                while (substringMatch < longestMatch.length() && substringMatch < thisItem.length() &&
                       thisItem[substringMatch].toLower() == longestMatch[substringMatch].toLower())
                {
                    ++substringMatch;
                }
                longestMatch = longestMatch.left(substringMatch);
            }
        }
    }
    return longestMatch;
}
    

void MainWindow::do_sd_tab_completion()
{
    QString originalText(ui->lineEditSDInput->text().simplified());
    QString callSearch = sdthread->sd_strip_leading_selectors(originalText);
    QString prefix = originalText.left(originalText.length() - callSearch.length());

    
    QString longestMatch(get_longest_match_from_list_widget(ui->listWidgetSDOptions,
                                                            originalText,
                                                            callSearch));
    longestMatch = get_longest_match_from_list_widget(ui->listWidgetSDQuestionMarkComplete,
                                                      originalText,
                                                      originalText,
                                                      longestMatch);
//    qDebug() << "longest match is " << longestMatch;
    
    if (longestMatch.length() > callSearch.length())
    {
        if (prefix.length() != 0 && !prefix.endsWith(" "))
        {
            prefix = prefix + " ";
        }
        QString new_line((longestMatch.startsWith(prefix)) ? longestMatch : (prefix + longestMatch));

        bool forceAnEnter(false);
        
        if (new_line.contains('<'))
        {
            int indexOfLessThan = new_line.indexOf('<');
            int indexOfAnything = new_line.indexOf(strLTAnythingGT);
            forceAnEnter = new_line.endsWith(strLTAnythingGT)
                && indexOfLessThan == indexOfAnything;
            
            new_line = new_line.left(indexOfLessThan);
            if (forceAnEnter && !prefsManager.GetAutomaticEnterOnAnythingTabCompletion())
                new_line += strBracketsSubsidiaryCall;
        }
        ui->lineEditSDInput->setText(new_line);
        if (forceAnEnter)
        {
            if (prefsManager.GetAutomaticEnterOnAnythingTabCompletion())
                on_lineEditSDInput_returnPressed();
            else if (new_line.contains(strBracketsSubsidiaryCall))
            {
                int indexStart = new_line.indexOf(strBracketsSubsidiaryCall) + 1;
//                qDebug() << "Setting subsidiary call at " << indexStart;
                ui->lineEditSDInput->setSelection(indexStart, static_cast<int>(strlen(strBracketsSubsidiaryCall) - 2));
                ui->lineEditSDInput->setFocus();
            }
        }
    }
}


void MainWindow::SDDebug(const QString &str, bool bold) {
    QListWidgetItem *item = new QListWidgetItem(str);
    QFont font(item->font());
    font.setBold(bold);
    font.setFixedPitch(true);
    item->setFont(font);
    ui->listWidgetSDOutput->addItem(item);
    ui->listWidgetSDOutput->scrollToBottom();
}

void MainWindow::on_lineEditSDInput_returnPressed()
{
//    int redoRow = ui->tableWidgetCurrentSequence->rowCount() - 1;
//    while (redoRow >= 0 && redoRow < sd_redo_stack->length())
//        sd_redo_stack->erase(sd_redo_stack->begin() + redoRow);
    QString cmd(ui->lineEditSDInput->text().simplified());  // both trims whitespace and consolidates whitespace
    if (cmd.startsWith("debug "))
    {
        if (cmd == "debug undo")
        {
            SDDebug("Undo Command Stack", true);
            for (QString s : sd_redo_stack->command_stack)
            {
                SDDebug(s);
            }
            SDDebug("Redo Command Stack", true);
            for (int i = 0; i < sd_redo_stack->redo_stack.count(); ++i)
            {
                SDDebug(QString("%1").arg(i));
                for (QString s : sd_redo_stack->redo_stack[i])
                {
                    SDDebug(QString("    %1").arg(s));
                }
            }
        }
        ui->lineEditSDInput->clear();
    }

    
    sd_redo_stack->set_doing_user_input();
    submit_lineEditSDInput_contents_to_sd();
    sd_redo_stack->clear_doing_user_input();
}

void MainWindow::submit_lineEditSDInput_contents_to_sd()
{
    QString cmd(ui->lineEditSDInput->text().simplified());  // both trims whitespace and consolidates whitespace
    cmd = cmd.toLower(); // on input, convert everything to lower case, to make it easier for the user
    ui->lineEditSDInput->clear();

    if (!cmd.compare("quit", Qt::CaseInsensitive))
    {
        cmd = str_abort_this_sequence;
    }

    // handle quarter, a quarter, one quarter, half, one half, two quarters, three quarters
    // must be in this order, and must be before number substitution.
    cmd = cmd.replace("once and a half","1-1/2");
    cmd = cmd.replace("one and a half","1-1/2");
    cmd = cmd.replace("two and a half","2-1/2");
    cmd = cmd.replace("three and a half","3-1/2");
    cmd = cmd.replace("four and a half","4-1/2");
    cmd = cmd.replace("five and a half","5-1/2");
    cmd = cmd.replace("six and a half","6-1/2");
    cmd = cmd.replace("seven and a half","7-1/2");
    cmd = cmd.replace("eight and a half","8-1/2");

    cmd = cmd.replace("four quarters","4/4");
    cmd = cmd.replace("three quarters","3/4").replace("three quarter","3/4");  // always replace the longer thing first!
    cmd = cmd.replace("two quarters","2/4").replace("one half","1/2").replace("half","1/2");
    cmd = cmd.replace("and a quarter more", "AND A QUARTER MORE");  // protect this.  Separate call, must NOT be "1/4"
    cmd = cmd.replace("one quarter", "1/4").replace("a quarter", "1/4").replace("quarter","1/4");
    cmd = cmd.replace("AND A QUARTER MORE", "and a quarter more");  // protect this.  Separate call, must NOT be "1/4"

    // handle 1P2P
    cmd = cmd.replace("one pee two pee","1P2P");

    // handle "third" --> "3rd", for dixie grand
    cmd = cmd.replace("third", "3rd");

    // handle numbers: one -> 1, etc.
    cmd = cmd.replace("eight chain","EIGHT CHAIN"); // sd wants "eight chain 4", not "8 chain 4", so protect it
    cmd = cmd.replace("one","1").replace("two","2").replace("three","3").replace("four","4");
    cmd = cmd.replace("five","5").replace("six","6").replace("seven","7").replace("eight","8");
    cmd = cmd.replace("EIGHT CHAIN","eight chain"); // sd wants "eight chain 4", not "8 chain 4", so protect it

    // handle optional words at the beginning

    cmd = cmd.replace("do the centers", "DOTHECENTERS");  // special case "do the centers part of load the boat"
    cmd = cmd.replace(QRegularExpression("^go "),"").replace(QRegularExpression("^do a "),"").replace(QRegularExpression("^do "),"");
    cmd = cmd.replace("DOTHECENTERS", "do the centers");  // special case "do the centers part of load the boat"

    // handle specialized sd spelling of flutter wheel, and specialized wording of reverse flutter wheel
    cmd = cmd.replace("flutterwheel","flutter wheel");
    cmd = cmd.replace("reverse the flutter","reverse flutter wheel");

    // handle specialized sd wording of first go *, next go *
    cmd = cmd.replace(QRegularExpression("first[a-z ]* go left[a-z ]* next[a-z ]* go right"),"first couple go left, next go right");
    cmd = cmd.replace(QRegularExpression("first[a-z ]* go right[a-z ]* next[a-z ]* go left"),"first couple go right, next go left");
    cmd = cmd.replace(QRegularExpression("first[a-z ]* go left[a-z ]* next[a-z ]* go left"),"first couple go left, next go left");
    cmd = cmd.replace(QRegularExpression("first[a-z ]* go right[a-z ]* next[a-z ]* go right"),"first couple go right, next go right");

    // handle "single circle to an ocean wave" -> "single circle to a wave"
    cmd = cmd.replace("single circle to an ocean wave","single circle to a wave");

    // handle manually-inserted brackets
    cmd = cmd.replace(QRegularExpression("left bracket\\s+"), "[").replace(QRegularExpression("\\s+right bracket"),"]");

    // handle "single hinge" --> "hinge", "single file circulate" --> "circulate", "all 8 circulate" --> "circulate" (quirk of sd)
    cmd = cmd.replace("single hinge", "hinge").replace("single file circulate", "circulate").replace("all 8 circulate", "circulate");

    // handle "men <anything>" and "ladies <anything>", EXCEPT for ladies chain
    if (!cmd.contains("ladies chain") && !cmd.contains("men chain")) {
        cmd = cmd.replace("men", "boys").replace("ladies", "girls");  // wacky sd!
    }

    // handle "allemande left alamo style" --> "allemande left in the alamo style"
    cmd = cmd.replace("allemande left alamo style", "allemande left in the alamo style");

    // handle "right a left thru" --> "right and left thru"
    cmd = cmd.replace("right a left thru", "right and left thru");

    // handle "separate [go] around <n> [to a line]" --> delete "go"
    cmd = cmd.replace("separate go around", "separate around");

    // handle "dixie style [to a wave|to an ocean wave]" --> "dixie style to a wave"
    cmd = cmd.replace(QRegularExpression("dixie style.*"), "dixie style to a wave");

    // handle the <anything> and roll case
    //   NOTE: don't do anything, if we added manual brackets.  The user is in control in that case.
    if (!cmd.contains("[")) {

        // TODO: might want to make this A-level specific?
        if (!cmd.contains("pass and roll")) {  // at A-level, "Pass and Roll" must not be turned into "[Pass] and Roll"
            QRegularExpression andRollCall("(.*) and roll.*");
//            if (cmd.indexOf(andRollCall) != -1) {
//                cmd = "[" + andRollCall.cap(1) + "] and roll";
//            }

            cmd.replace(andRollCall, "[ \\1 ] and roll");
        }

        // explode must be handled *after* roll, because explode binds tightly with the call
        // e.g. EXPLODE AND RIGHT AND LEFT THRU AND ROLL must be translated to:
        //      [explode and [right and left thru]] and roll

        // first, handle both: "explode and <anything> and roll"
        //  by the time we're here, it's already "[explode and <anything>] and roll", because
        //  we've already done the roll processing.
        QRegularExpression explodeAndRollCall("\\[explode and (.*)\\] and roll");
        QRegularExpression explodeAndNotRollCall("^explode and (.*)");

//        if (cmd.indexOf(explodeAndRollCall) != -1) {
//            cmd = "[explode and [" + explodeAndRollCall.cap(1).trimmed() + "]] and roll";
//        } else if (cmd.indexOf(explodeAndNotRollCall) != -1) {
//            // not a roll, for sure.  Must be a naked "explode and <anything>"
//            cmd = "explode and [" + explodeAndNotRollCall.cap(1).trimmed() + "]";
//        } else {
//        }

        cmd.replace(explodeAndRollCall, "[ explode and \\1 ] and roll");
        cmd.replace(explodeAndNotRollCall, "explode and [ \\1 ]");

    }

    // handle <ANYTHING> and spread
    if (!cmd.contains("[")) {
        QRegularExpression andSpreadCall("(.*) and spread");
//        if (cmd.indexOf(andSpreadCall) != -1) {
//            cmd = "[" + andSpreadCall.cap(1) + "] and spread";
//        }
        cmd.replace(andSpreadCall, "[ \\1 ] and spread");
    }

    // handle "undo [that]" --> "undo last call"
    cmd = cmd.replace("undo that", str_undo_last_call);
    if (cmd == "undo") {
        cmd = str_undo_last_call;
    }

    // handle "peel your top" --> "peel the top"
    cmd = cmd.replace("peel your top", "peel the top");

    // handle "veer (to the) left/right" --> "veer left/right"
    cmd = cmd.replace("veer to the", "veer");

    // handle the <anything> and sweep case
    // FIX: this needs to add center boys, etc, but that messes up the QRegularExpression

    // <ANYTHING> AND SWEEP (A | ONE) QUARTER [MORE]
    QRegularExpression andSweepPart(" and sweep.*");
    int found = cmd.indexOf(andSweepPart);
    if (found != -1) {
        if (cmd.contains("[")) {
            // if manual brackets added, don't add more of them.
            cmd = cmd.replace(andSweepPart,"") + " and sweep 1/4";
        } else {
            cmd = "[" + cmd.replace(andSweepPart,"") + "] and sweep 1/4";
        }
    }

    // handle "square thru" -> "square thru 4"
    //  and "heads square thru" --> "heads square thru 4"
    //  and "sides..."
    if (cmd.endsWith("square thru")) {
        cmd.replace("square thru", "square thru 4");
    }

    // SD COMMANDS -------
    // square your|the set -> square thru 4
    if (cmd == "square the set" || cmd == "square your set"
        || cmd == str_square_your_sets) {
        sdthread->do_user_input(str_abort_this_sequence);
        sdthread->do_user_input("y");
    }
    else
    {
        if (ui->tableWidgetCurrentSequence->rowCount() == 0 && !cmd.contains("1p2p")) {
            if (cmd.startsWith("heads ")) {
                sdthread->do_user_input("heads start");
                cmd.replace("heads ", "");
            } else if (cmd.startsWith("sides")) {
                sdthread->do_user_input("sides start");
                cmd.replace("sides ", "");
            }
        }
        if (sdthread->do_user_input(cmd))
        {
            int row = ui->tableWidgetCurrentSequence->rowCount() - 1;
            if (row < 0) row = 0;
            if (!cmd.compare(str_undo_last_call, Qt::CaseInsensitive))
            {
                sdthread->do_user_input("refresh");
                sd_redo_stack->did_an_undo();
            }
            else
            {
                sd_redo_stack->add_command(row, cmd);
            }
        }
    }
}

dance_level MainWindow::get_current_sd_dance_program()
{
    dance_level current_dance_program = static_cast<dance_level>(INT_MAX);

    if (ui->actionSDDanceProgramMainstream->isChecked())
    {
        current_dance_program = l_mainstream;
    }
    if (ui->actionSDDanceProgramPlus->isChecked())
    {
        current_dance_program = l_plus;
    }
    if (ui->actionSDDanceProgramA1->isChecked())
    {
        current_dance_program = l_a1;
    }
    if (ui->actionSDDanceProgramA2->isChecked())
    {
        current_dance_program = l_a2;
    }
    if (ui->actionSDDanceProgramC1->isChecked())
    {
        current_dance_program = l_c1;
    }
    if (ui->actionSDDanceProgramC2->isChecked())
    {
        current_dance_program = l_c2;
    }
    if (ui->actionSDDanceProgramC3A->isChecked())
    {
        current_dance_program = l_c3a;
    }
    if (ui->actionSDDanceProgramC3->isChecked())
    {
        current_dance_program = l_c3;
    }
    if (ui->actionSDDanceProgramC3x->isChecked())
    {
        current_dance_program = l_c3x;
    }
    if (ui->actionSDDanceProgramC4->isChecked())
    {
        current_dance_program = l_c4;
    }
    if (ui->actionSDDanceProgramC4x->isChecked())
    {
        current_dance_program = l_c4x;
    }
    return current_dance_program;
}

void MainWindow::on_lineEditSDInput_textChanged()
{
    bool showCommands = ui->actionShow_Commands->isChecked();
    bool showConcepts = ui->actionShow_Concepts->isChecked();
    
    QString currentText = ui->lineEditSDInput->text().simplified();
    int currentTextLastChar = currentText.length() - 1;
    
    if (currentTextLastChar >= 0 &&
        (currentText[currentTextLastChar] == '!' ||
         currentText[currentTextLastChar] == '?'))
    {
        submit_lineEditSDInput_contents_to_sd();
        ui->lineEditSDInput->setText(currentText.left(currentTextLastChar));
        sdLineEditSDInputLengthWhenAvailableCallsWasBuilt = currentTextLastChar;
        return;
    }
    else
    {
        if (currentTextLastChar < sdLineEditSDInputLengthWhenAvailableCallsWasBuilt)
            sdAvailableCalls.clear();
    }
    
    QString strippedText = sdthread->sd_strip_leading_selectors(currentText);

    dance_level current_dance_program = get_current_sd_dance_program();
    
    for (int i = 0; i < ui->listWidgetSDOptions->count(); ++i)
    {
        bool show = (currentText.length() == compareEnteredCallToCall(currentText, ui->listWidgetSDOptions->item(i)->text())
                     || strippedText.length() == compareEnteredCallToCall(strippedText, ui->listWidgetSDOptions->item(i)->text()));

        if (show)
        {
            QVariant v = ui->listWidgetSDOptions->item(i)->data(Qt::UserRole);
            int dance_program = v.toInt();
            if (dance_program & kSDCallTypeConcepts)
            {
                if (!showConcepts)
                    show = false;
            }
            else if (dance_program & kSDCallTypeCommands)
            {
                bool inAvailableCalls = false;
                if (sdAvailableCalls.length() > 0)
                {
                    static QRegularExpression regexpReplaceableParts("<.*?>");
                    QString callName = ui->listWidgetSDOptions->item(i)->text();
                    QStringList callParts(callName.split(regexpReplaceableParts, Qt::SkipEmptyParts));
                    
                    for (auto call : sdAvailableCalls)
                    {
                        bool containsAllParts = true;
                        for (auto callPart : callParts)
                        {
                            if (!call.call_name.contains(callPart, Qt::CaseInsensitive))
                            {
                                containsAllParts = false;
                            }
                        }
                        if (containsAllParts)
                            inAvailableCalls = true;
                    }
                }
                if (!inAvailableCalls)
                    show = false;
                if (!showCommands)
                    show = false;
            }
            else if (dance_program > current_dance_program)
            {
                show = false;
            }
        }
        ui->listWidgetSDOptions->setRowHidden(i, !show);
    }

}

void MainWindow::startSDThread(dance_level dance_program) {
    // Initializers for these should probably be up in the constructor
    sdthread = new SDThread(this, dance_program, sdLevelEnumsToStrings[dance_program]);
    sdthread->start();
    sdthread->unlock();
}

void MainWindow::restartSDThread(dance_level dance_program)
{
    if (sdthread)
    {
        sdthread->finishAndShutdownSD();
        sdthread->wait(250);
        
        sdthread = nullptr; // NULL;
    }
    startSDThread(dance_program);
    reset_sd_dancer_locations();
    
}

void MainWindow::setCurrentSDDanceProgram(dance_level dance_program)
{
    restartSDThread(dance_program);
//    for (int i = 0; actions[i]; ++i)
//    {
//        bool checked = (i == (int)(dance_program));
//        actions[i]->setChecked(checked);
//    }

    // set string for keyboard level in UI
    currentSDKeyboardLevel = "UNK";
    switch (dance_program) {
        case l_mainstream: currentSDKeyboardLevel = "Mainstream"; break;
        case l_plus: currentSDKeyboardLevel = "Plus"; break;
        case l_a1:   currentSDKeyboardLevel = "A1"; break;
        case l_a2:   currentSDKeyboardLevel = "A2"; break;
        case l_c1:   currentSDKeyboardLevel = "C1"; break;
        case l_c2:   currentSDKeyboardLevel = "C2"; break;
        case l_c3a:  currentSDKeyboardLevel = "C3a"; break;
        case l_c3:   currentSDKeyboardLevel = "C3"; break;
        case l_c3x:  currentSDKeyboardLevel = "C3x"; break;
        case l_c4a:  currentSDKeyboardLevel = "C4a"; break;
        case l_c4:   currentSDKeyboardLevel = "C4"; break;
        case l_c4x:  currentSDKeyboardLevel = "C4x"; break;
        default: break;
    }
    microphoneStatusUpdate(); // update the level designator

    on_lineEditSDInput_textChanged();
}



void MainWindow::on_actionSDDanceProgramMainstream_triggered()
{
    setCurrentSDDanceProgram(l_mainstream);
}
void MainWindow::on_actionSDDanceProgramPlus_triggered()
{
    setCurrentSDDanceProgram(l_plus);
}
void MainWindow::on_actionSDDanceProgramA1_triggered()
{
    setCurrentSDDanceProgram(l_a1);
}
void MainWindow::on_actionSDDanceProgramA2_triggered()
{
    setCurrentSDDanceProgram(l_a2);
}
void MainWindow::on_actionSDDanceProgramC1_triggered()
{
    setCurrentSDDanceProgram(l_c1);
}
void MainWindow::on_actionSDDanceProgramC2_triggered()
{
    setCurrentSDDanceProgram(l_c2);
}
void MainWindow::on_actionSDDanceProgramC3A_triggered()
{
    setCurrentSDDanceProgram(l_c3a);
}
void MainWindow::on_actionSDDanceProgramC3_triggered()
{
    setCurrentSDDanceProgram(l_c3);
}
void MainWindow::on_actionSDDanceProgramC3x_triggered()
{
    setCurrentSDDanceProgram(l_c3x);
}
void MainWindow::on_actionSDDanceProgramC4_triggered()
{
    setCurrentSDDanceProgram(l_c4);
}
void MainWindow::on_actionSDDanceProgramC4x_triggered()
{
    setCurrentSDDanceProgram(l_c4x);
}

void MainWindow::on_actionShow_Concepts_triggered()
{
    on_lineEditSDInput_textChanged();
}

void MainWindow::on_actionShow_Commands_triggered()
{
    on_lineEditSDInput_textChanged();
}

void MainWindow::on_actionFormation_Thumbnails_triggered()
{
    set_current_sequence_icons_visible(ui->actionFormation_Thumbnails->isChecked());
}

void SDDancer::setColor(const QColor &color)
{
//    QGraphicsRectItem* rectItem = dynamic_cast<QGraphicsRectItem*>(mainItem);
//    QGraphicsEllipseItem* ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(mainItem);
////    QGraphicsPolygonItem* polygonItem = dynamic_cast<QGraphicsPolygonItem*>(mainItem);  // HEXAGON
//    if (rectItem) rectItem->setBrush(QBrush(color));
//    if (ellipseItem) ellipseItem->setBrush(QBrush(color));
////    if (polygonItem) polygonItem->setBrush(QBrush(color));   // HEXAGON

    QGraphicsRectItem* rectItem = dynamic_cast<QGraphicsRectItem*>(boyItem);
    rectItem->setBrush(QBrush(color));

    QGraphicsEllipseItem* ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(girlItem);
    ellipseItem->setBrush(QBrush(color));

    QGraphicsPolygonItem* polygonItem = dynamic_cast<QGraphicsPolygonItem*>(hexItem);
    polygonItem->setBrush(QBrush(color));

    directionRectItem->setBrush(QBrush(color));
}

static void setPeopleColoringScheme(QList<SDDancer> &sdpeople, const QString &colorScheme)
{
//    bool showDancerLabels = (colorScheme == "Normal") || (colorScheme == "Random");
//    for (int dancerNum = 0; dancerNum < sdpeople.length(); ++dancerNum)
//    {
//        sdpeople[dancerNum].label->setVisible(showDancerLabels);
//    }

    // New colors are: "Normal", "Mental Image", "Sight", "Randomize"
    if (colorScheme == "Normal" || colorScheme == "Color only") {
        sdpeople[COUPLE1 * 2 + 0].setColor(COUPLE1COLOR);
        sdpeople[COUPLE1 * 2 + 1].setColor(COUPLE1COLOR);
        sdpeople[COUPLE2 * 2 + 0].setColor(COUPLE2COLOR);
        sdpeople[COUPLE2 * 2 + 1].setColor(COUPLE2COLOR);
        sdpeople[COUPLE3 * 2 + 0].setColor(COUPLE3COLOR);
        sdpeople[COUPLE3 * 2 + 1].setColor(COUPLE3COLOR);
        sdpeople[COUPLE4 * 2 + 0].setColor(COUPLE4COLOR);
        sdpeople[COUPLE4 * 2 + 1].setColor(COUPLE4COLOR);
    } else if (colorScheme == "Mental Image") {
        sdpeople[COUPLE1 * 2 + 0].setColor(COUPLE1COLOR);
        sdpeople[COUPLE1 * 2 + 1].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE2 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE2 * 2 + 1].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE3 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE3 * 2 + 1].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE4 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE4 * 2 + 1].setColor(GREYCOUPLECOLOR);
    } else if (colorScheme == "Sight") {
        // Sight
        sdpeople[COUPLE1 * 2 + 0].setColor(COUPLE1COLOR);
        sdpeople[COUPLE1 * 2 + 1].setColor(COUPLE1COLOR);
        sdpeople[COUPLE2 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE2 * 2 + 1].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE3 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE3 * 2 + 1].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE4 * 2 + 0].setColor(COUPLE4COLOR);
        sdpeople[COUPLE4 * 2 + 1].setColor(COUPLE4COLOR);
    } else {
        // Randomize
        // Random, or Random Color only (every time we get here, we get new colors!
        QColor randomColors[8] = {RANDOMCOLOR1, RANDOMCOLOR2, RANDOMCOLOR3, RANDOMCOLOR4,
                   RANDOMCOLOR5, RANDOMCOLOR6, RANDOMCOLOR7, RANDOMCOLOR8};
        int indexes[8] ={0,1,2,3,4,5,6,7};

        std::random_shuffle(std::begin(indexes), std::end(indexes)); // randomize colors

        sdpeople[COUPLE1 * 2 + 0].setColor(randomColors[indexes[0]]);
        sdpeople[COUPLE1 * 2 + 1].setColor(randomColors[indexes[1]]);
        sdpeople[COUPLE2 * 2 + 0].setColor(randomColors[indexes[2]]);
        sdpeople[COUPLE2 * 2 + 1].setColor(randomColors[indexes[3]]);
        sdpeople[COUPLE3 * 2 + 0].setColor(randomColors[indexes[4]]);
        sdpeople[COUPLE3 * 2 + 1].setColor(randomColors[indexes[5]]);
        sdpeople[COUPLE4 * 2 + 0].setColor(randomColors[indexes[6]]);
        sdpeople[COUPLE4 * 2 + 1].setColor(randomColors[indexes[7]]);
    }
}

static void setPeopleNumberingScheme(QList<SDDancer> &sdpeople, const QString &numberScheme)
{
    // New numbers are: "Normal", "Invisible"
    bool showDancerLabels = (numberScheme == "Normal");
    for (int dancerNum = 0; dancerNum < sdpeople.length(); ++dancerNum)
    {
        sdpeople[dancerNum].label->setVisible(showDancerLabels);
    }
}

static void setPeopleGenderingScheme(QList<SDDancer> &sdpeople, const QString &genderScheme)
{
    // New genders are: "Normal", "Arky (reversed)", "Randomize", "None (hex)"
    for (int dancerNum = 0; dancerNum < sdpeople.length(); ++dancerNum)
    {
        if (genderScheme == "Normal") {
            sdpeople[dancerNum].boyItem->setVisible(dancerNum % 2 == 0);   // even dancer nums are boys
            sdpeople[dancerNum].girlItem->setVisible(dancerNum % 2 == 1);  // odd dancer nums are girls
            sdpeople[dancerNum].hexItem->setVisible(false);
        } else if (genderScheme == "Arky (reversed)") {
            sdpeople[dancerNum].boyItem->setVisible(dancerNum % 2 == 1);   // even dancer nums are girls
            sdpeople[dancerNum].girlItem->setVisible(dancerNum % 2 == 0);  // odd dancer nums are girls
            sdpeople[dancerNum].hexItem->setVisible(false);
        } else if (genderScheme == "Randomize") {
            bool randBoyGirl = rand() > (RAND_MAX / 2); // 50% boy, 50% girl; persistent until next Randomize
            sdpeople[dancerNum].boyItem->setVisible(randBoyGirl);
            sdpeople[dancerNum].girlItem->setVisible(!randBoyGirl);
            sdpeople[dancerNum].hexItem->setVisible(false); // not doing genderless dancers yet
        } else if (genderScheme == "None (hex)") {
            sdpeople[dancerNum].boyItem->setVisible(false);   // even dancer nums are girls
            sdpeople[dancerNum].girlItem->setVisible(false);  // odd dancer nums are girls
            sdpeople[dancerNum].hexItem->setVisible(true);    // show genderless checkers
        }
    }
}

void MainWindow::setSDCoupleColoringScheme(const QString &colorScheme)
{
    setPeopleColoringScheme(sd_animation_people, colorScheme);
    setPeopleColoringScheme(sd_fixed_people, colorScheme);
}

void MainWindow::setSDCoupleNumberingScheme(const QString &numberScheme)
{
//    qDebug() << "setSDCoupleNumberingScheme: " << numberScheme;
    setPeopleNumberingScheme(sd_animation_people, numberScheme);
    setPeopleNumberingScheme(sd_fixed_people, numberScheme);
}

void MainWindow::setSDCoupleGenderingScheme(const QString &genderScheme)
{
//    qDebug() << "setSDCoupleGenderingScheme: " << genderScheme;
    setPeopleGenderingScheme(sd_animation_people, genderScheme);
    setPeopleGenderingScheme(sd_fixed_people, genderScheme);
}

void MainWindow::select_all_sd_current_sequence()
{
    for (int row = 0; row < ui->tableWidgetCurrentSequence->rowCount(); ++row)
    {
        for (int col = 0; col < ui->tableWidgetCurrentSequence->columnCount(); ++col)
        {
            QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,col);
            item->setSelected(true);
        }
    }    
}

void MainWindow::undo_last_sd_action()
{
    sdthread->do_user_input(str_undo_last_call);
    sdthread->do_user_input("refresh");
    sd_redo_stack->did_an_undo();
    
    ui->lineEditSDInput->setFocus();
}

void MainWindow::redo_last_sd_action()
{
    int redoRow = ui->tableWidgetCurrentSequence->rowCount() - 1;
    if (redoRow < 0) redoRow = 0;
    QStringList redoCommands(sd_redo_stack->get_redo_commands());
    sd_redo_stack->set_doing_user_input();
    for (auto redoCommand : redoCommands)
    {
        if (!redoCommand.startsWith("<")) {
            ui->lineEditSDInput->setText(redoCommand);
            submit_lineEditSDInput_contents_to_sd();
//            sdthread->do_user_input(redoCommand);
        }
    }
    sd_redo_stack->clear_doing_user_input();
    sd_redo_stack->discard_redo_commands();

    ui->lineEditSDInput->setFocus();
}

void MainWindow::undo_sd_to_row()
{
    while (--sdUndoToLine)
    {
        undo_last_sd_action();
    }
}


void MainWindow::copy_selection_from_tableWidgetCurrentSequence()
{
    bool copyAllRows(prefsManager.GetSDCallListCopyOptions() == 1);
    bool copyOnlySelectedCells(prefsManager.GetSDCallListCopyOptions() == 3);
    QString selection;
    for (int row = 0; row < ui->tableWidgetCurrentSequence->rowCount(); ++row)
    {
        bool selected(copyAllRows);
        for (int col = 0; col < ui->tableWidgetCurrentSequence->columnCount(); ++col)
        {
            QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,col);
            if (item->isSelected())
            {
                if (copyOnlySelectedCells)
                {
                    if (col == kColCurrentSequenceCall)
                    {
                        selection += item->text() + "\n";
                    }
                    else if (row == kColCurrentSequenceFormation)
                    {
                        QVariant v = item->data(Qt::UserRole);
                        QString formation(v.toString());
                        QStringList formationList = formation.split("\n");
                        for (auto s : formationList)
                        {
                            selection += "    " + s + "\n";
                        }
                    }
                }
                else
                {
                    selected = true;
                }
            }
        }
        if (selected)
        {
            QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,0);
            selection += item->text() + "\n";
        }
    }
     
    if (prefsManager.GetSDCallListCopyDeepUndoBuffer())
    {
        selection += "\nCommand Stack:\n";
        for (QString s : sd_redo_stack->command_stack)
        {
            selection += s + "\n";
        }
    }
    QApplication::clipboard()->setText(selection);
}

void MainWindow::copy_selection_from_listWidgetSDOutput()
{
    QString selection;
    
    for (int row = 0; row < ui->listWidgetSDOutput->count(); ++row)
    {
        QListWidgetItem *item = ui->listWidgetSDOutput->item(row);
        if (item->isSelected())
        {
            if (!item->text().isEmpty())
                selection += item->text()  + "\n";
        }
    }

    QApplication::clipboard()->setText(selection);
}

QString MainWindow::render_image_item_as_html(QTableWidgetItem *imageItem, QGraphicsScene &scene, QList<SDDancer> &people, bool graphics_as_text)
{
    QString selection;
    QVariant v = imageItem->data(Qt::UserRole);
    
    if (!v.isNull())
    {
        QString formation(v.toString());
        
        QStringList formationList = formation.split("\n");
        if (formationList.size() > 0)
        {
            QString formationName = formationList[0];
            formationList.removeFirst();
            
            if (graphics_as_text)
            {
                selection += ": " + formationName.toHtmlEscaped();
                formationList.removeFirst();
                selection += "<pre>" + formationList.join("\n").toHtmlEscaped() + "</pre>";
            }
            else
            {
                decode_formation_into_dancer_destinations(formationList, people);   // also sets L/Tgroup and B/Gorder
//                set_sd_last_groupness(lGroup, tGroup); // update groupness strings
                move_dancers(people, 1);
                QBuffer svgText;
                svgText.open(QBuffer::ReadWrite);
                QSvgGenerator svgGen;
                svgGen.setOutputDevice(&svgText);
                svgGen.setSize(QSize(static_cast<int>(halfBackgroundSize) * 2,
                                     static_cast<int>(halfBackgroundSize) * 2));
                svgGen.setTitle(formationName);
                svgGen.setDescription(formation.toHtmlEscaped());
                {
                    QPainter painter( &svgGen );
                    scene.render( &painter );
                    painter.end();
                }
                svgText.seek(0);
                QString s(svgText.readAll());
                selection += "<br>" + s;
            }
        }
    }
    return selection;
}

QString MainWindow::get_current_sd_sequence_as_html(bool all_fields, bool graphics_as_text)
{
    QGraphicsScene scene;
    QList<SDDancer> people;
    QGraphicsTextItem *graphicsTextItemStatusBar;
    bool copyOnlySelectedCells(prefsManager.GetSDCallListCopyOptions() == 3);

    QGraphicsTextItem *graphicsTextItemLeftGroup;
    QGraphicsTextItem *graphicsTextItemTopGroup;
    initialize_scene(scene, people, graphicsTextItemStatusBar, graphicsTextItemLeftGroup, graphicsTextItemTopGroup);
    
    QString selection("<!DOCTYPE html>\n"
                      "<html lang=\"en\">\n"
                      "  <head>\n"
                      "    <meta charset=\"utf-8\">\n"
                      "    <title>Square Dance Sequence</title>\n"
                      "  </head>\n"
                      "  <body>\n"
                      "    <h1>Square Dance Sequence</h1>\n");
    if (!prefsManager.GetSDCallListCopyHTMLIncludeHeaders())
        selection.clear();

    selection += "    <ol>\n";
    for (int row = 0; row < ui->tableWidgetCurrentSequence->rowCount(); ++row)
    {
        bool selected(all_fields);
        for (int col = 0; col < ui->tableWidgetCurrentSequence->columnCount(); ++col)
        {
            QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,col);
            if (item->isSelected())
            {
                if (copyOnlySelectedCells)
                {
                    selected += "<li>";
                    if (col == 0)
                    {
                        QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,kColCurrentSequenceCall);
                        selection += "<b>" + item->text().toHtmlEscaped() + "</b>";
                    }
                    else if (col == 1)
                    {
                        QTableWidgetItem *imageItem = ui->tableWidgetCurrentSequence->item(row,kColCurrentSequenceFormation);
                        selection += render_image_item_as_html(imageItem, scene, people, graphics_as_text);
                    }
                    selection += "</li>";
                }
                else
                {
                    selected = true;
                }
            }
        }
        if (selected)
        {
            QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,kColCurrentSequenceCall);
            selection += "<li><b>" + item->text().toHtmlEscaped() + "</b>";
            QTableWidgetItem *imageItem = ui->tableWidgetCurrentSequence->item(row,kColCurrentSequenceFormation);
            if ( (1) || imageItem->isSelected() )
            {
                selection += render_image_item_as_html(imageItem, scene, people, graphics_as_text);
            }

            selection +=  + "</li>\n";
        }
    }

    if (prefsManager.GetSDCallListCopyHTMLIncludeHeaders())
    {
        selection += "    </ol>\n"
            "  </body>\n"
            "</html>\n";
    }
    return selection;
}

void MainWindow::copy_selection_from_tableWidgetCurrentSequence_html()
{
    QString selection(get_current_sd_sequence_as_html(prefsManager.GetSDCallListCopyOptions() == 1,
                                                      !prefsManager.GetSDCallListCopyHTMLFormationsAsSVG()));
    QApplication::clipboard()->setText(selection);
    
}

void MainWindow::set_sd_copy_options_entire_sequence()
{
    prefsManager.SetSDCallListCopyOptions(1);
}

void MainWindow::set_sd_copy_options_selected_rows()
{
    prefsManager.SetSDCallListCopyOptions(2);
}

void MainWindow::set_sd_copy_options_selected_cells()
{
    prefsManager.SetSDCallListCopyOptions(3);
}

void MainWindow::toggle_sd_copy_html_includes_headers()
{
    prefsManager.SetSDCallListCopyHTMLIncludeHeaders(!prefsManager.GetSDCallListCopyHTMLIncludeHeaders());
}

void MainWindow::toggle_sd_copy_html_formations_as_svg()
{
    prefsManager.SetSDCallListCopyHTMLFormationsAsSVG(!prefsManager.GetSDCallListCopyHTMLFormationsAsSVG());
}

void MainWindow::toggle_sd_copy_include_deep()
{
    prefsManager.SetSDCallListCopyDeepUndoBuffer(!prefsManager.GetSDCallListCopyDeepUndoBuffer());
}


void MainWindow::on_tableWidgetCurrentSequence_customContextMenuRequested(const QPoint &pos)
{
    QMenu contextMenu(tr("Sequence"), this);

    QAction action1("Copy", this);
    connect(&action1, SIGNAL(triggered()), this, SLOT(copy_selection_from_tableWidgetCurrentSequence()));
    action1.setShortcut(QKeySequence::Copy);
    contextMenu.addAction(&action1);
   
    QAction action2("Undo", this);
    connect(&action2, SIGNAL(triggered()), this, SLOT(undo_last_sd_action()));
    action2.setShortcut(QKeySequence::Undo);
    contextMenu.addAction(&action2);

    QAction action25("Redo", this);
    connect(&action25, SIGNAL(triggered()), this, SLOT(redo_last_sd_action()));
    action25.setShortcut(QKeySequence::Redo);
    action25.setEnabled(sd_redo_stack->can_redo());
    contextMenu.addAction(&action25);

    
    QAction action3("Select All", this);
    connect(&action3, SIGNAL(triggered()), this, SLOT(select_all_sd_current_sequence()));
    action3.setShortcut(QKeySequence::SelectAll);
    contextMenu.addAction(&action3);

    contextMenu.addSeparator(); // ---------------

    sdUndoToLine = ui->tableWidgetCurrentSequence->rowCount() - ui->tableWidgetCurrentSequence->rowAt(pos.y());
    QAction action4("Go Back To Here", this);
    connect(&action4, SIGNAL(triggered()), this, SLOT(undo_sd_to_row()));
    contextMenu.addAction(&action4);

    QAction action4a("Square Your Sets", this);
    connect(&action4a, SIGNAL(triggered()), this, SLOT(on_actionSDSquareYourSets_triggered()));
    contextMenu.addAction(&action4a);

    contextMenu.addSeparator(); // ---------------

    QAction action5("Copy as HTML", this);
    connect(&action5, SIGNAL(triggered()), this, SLOT(copy_selection_from_tableWidgetCurrentSequence_html()));
    contextMenu.addAction(&action5);

    QMenu menuCopySettings("Copy Options");
    QActionGroup actionGroupCopyOptions(this);
    actionGroupCopyOptions.setExclusive(true);

    QAction action1_1("Entire Sequence", this);
    action1_1.setCheckable(true);
    connect(&action1_1, SIGNAL(triggered()), this, SLOT(set_sd_copy_options_entire_sequence()));
    actionGroupCopyOptions.addAction(&action1_1);
    menuCopySettings.addAction(&action1_1);

    QAction action1_2("Selected Rows", this);
    action1_2.setCheckable(true);
    connect(&action1_2, SIGNAL(triggered()), this, SLOT(set_sd_copy_options_selected_rows()));
    actionGroupCopyOptions.addAction(&action1_2);
    menuCopySettings.addAction(&action1_2);

    QAction action1_3("Selected Cells", this);
    action1_3.setCheckable(true);
    connect(&action1_3, SIGNAL(triggered()), this, SLOT(set_sd_copy_options_selected_cells()));
    actionGroupCopyOptions.addAction(&action1_3);
    menuCopySettings.addAction(&action1_3);

    switch (prefsManager.GetSDCallListCopyOptions())
    {
    default:
    case 1:
        action1_1.setChecked(true);
        break;
    case 2:
        action1_2.setChecked(true);
        break;
    case 3:
        action1_3.setChecked(true);
        break;
    }
    
    menuCopySettings.addSeparator();
    QAction action1_4("HTML Includes Headers", this);
    action1_4.setCheckable(true);
    action1_4.setChecked(prefsManager.GetSDCallListCopyHTMLIncludeHeaders());
    connect(&action1_4, SIGNAL(triggered()), this, SLOT(toggle_sd_copy_html_includes_headers()));
    menuCopySettings.addAction(&action1_4);
    contextMenu.addMenu(&menuCopySettings);

    QAction action1_5("HTML Formations As SVG", this);
    action1_5.setCheckable(true);
    action1_5.setChecked(prefsManager.GetSDCallListCopyHTMLFormationsAsSVG());
    connect(&action1_5, SIGNAL(triggered()), this, SLOT(toggle_sd_copy_html_formations_as_svg()));
    menuCopySettings.addAction(&action1_5);
    contextMenu.addMenu(&menuCopySettings);

    QAction action1_6("Include Undo Buffer", this);
    action1_6.setCheckable(true);
    action1_6.setChecked(prefsManager.GetSDCallListCopyDeepUndoBuffer());
    connect(&action1_6, SIGNAL(triggered()), this, SLOT(toggle_sd_copy_include_deep()));
    menuCopySettings.addAction(&action1_6);
    contextMenu.addMenu(&menuCopySettings);

    contextMenu.exec(ui->tableWidgetCurrentSequence->mapToGlobal(pos));
}


void MainWindow::on_listWidgetSDOutput_customContextMenuRequested(const QPoint &pos)
{
    QMenu contextMenu(tr("Sequence"), this);

    QAction action1("Copy", this);
    connect(&action1, SIGNAL(triggered()), this, SLOT(copy_selection_from_listWidgetSDOutput()));
    contextMenu.addAction(&action1);
    QAction action2("Undo", this);
    connect(&action2, SIGNAL(triggered()), this, SLOT(undo_last_sd_action()));
    action2.setShortcut(QKeySequence::Undo);
    contextMenu.addAction(&action2);

    QAction action3("Redo", this);
    connect(&action3, SIGNAL(triggered()), this, SLOT(redo_last_sd_action()));
    action3.setShortcut(QKeySequence::Redo);
    action3.setEnabled(sd_redo_stack->can_redo());
    contextMenu.addAction(&action3);

    contextMenu.exec(ui->listWidgetSDOutput->mapToGlobal(pos));
}

void MainWindow::on_actionSDSquareYourSets_triggered() {
    ui->lineEditSDInput->clear();
    restartSDThread(get_current_sd_dance_program());

    if (nullptr == sdthread) // NULL == sdthread)
    {
        qDebug() << "Something has gone wrong, sdthread is null!";
        restartSDThread(get_current_sd_dance_program());
    }
//    qDebug() << "Square your sets";
    ui->lineEditSDInput->setFocus();  // set focus to the input line
}

void MainWindow::on_actionSDHeadsStart_triggered() {
//    qDebug() << "(square your sets and) Heads start";
    on_actionSDSquareYourSets_triggered();
    QStringList list(QString("heads start"));
    sdthread->resetAndExecute(list);

    ui->lineEditSDInput->setFocus();  // set focus to the input line
}

void MainWindow::on_actionSDHeadsSquareThru_triggered() {
    on_actionSDSquareYourSets_triggered();
    QStringList list(QString("heads start"));
    list.append(QString("square thru 4"));  // don't need to repeat "heads" here
    sdthread->resetAndExecute(list);
}

void MainWindow::on_actionSDHeads1p2p_triggered() {
//    qDebug() << "(square your sets and ) Heads 1p2p";
    on_actionSDSquareYourSets_triggered();
    QStringList list(QString("heads 1p2p"));
    sdthread->resetAndExecute(list);

    ui->lineEditSDInput->setFocus();  // set focus to the input line
}
