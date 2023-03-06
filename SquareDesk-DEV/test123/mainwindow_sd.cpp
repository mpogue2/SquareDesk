/****************************************************************************
**
** Copyright (C) 2016-2023 Mike Pogue, Dan Lyke
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
// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"
#include "utility.h"
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

static bool gTwoCouplesOnly = false;

static QGraphicsItemGroup *generateDancer(QGraphicsScene &sdscene, SDDancer &dancer, int number, bool boy)
{
    static QPen pen(Qt::black, 1.5, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);

    static double rectSize = 16;
    static QRectF rect(-rectSize/2, -rectSize/2, rectSize, rectSize);
    static QRectF directionRect(-2,-rectSize / 2 - 4,4,4);

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

    QGraphicsTextItem *label = sdscene.addText(QString("  %1  ").arg(number + 1), // exactly 3 characters wide when numbers used (sets the hjust)
                                               dancerLabelFont);

    QRectF labelBounds(label->boundingRect());
    QTransform labelTransform;
    dancer.labelTranslateX =-(labelBounds.left() + labelBounds.width() / 2);
    dancer.labelTranslateY = -(labelBounds.top() + labelBounds.height() / 2);
    labelTransform.translate(dancer.labelTranslateX,
                             dancer.labelTranslateY);
    label->setTransform(labelTransform);

    QList<QGraphicsItem*> items;
    items.append(boyItem);
    items.append(girlItem);
    items.append(hexItem);

    items.append(directionRectItem);
    items.append(label);
    QGraphicsItemGroup *group = sdscene.createItemGroup(items);
    dancer.graphics = group;
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

        // in two couple dancing, only couples 1 and 3 exist, others are invisible
        if (gTwoCouplesOnly && (dancerNum / 2) % 2 == 1) {
            sdpeople[dancerNum].graphics->setVisible(false);
        }

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
                //qDebug() << "Unknown character in SD output" << ch;
                break;
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
//                        QString gend = (girl == 1 ? "G" : "B");
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
    dancerLabelFont = QFont("Arial", 8);
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
    graphicsTextItemSDLeftGroupText->setTransform(statusBarTransform2);

    graphicsTextItemSDTopGroupText = sdscene.addText("", dancerLabelFont);
    QTransform statusBarTransform3;
    statusBarTransform3.translate(-halfBackgroundSize, -halfBackgroundSize/8);
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

    verticalHeader->setVisible(false); // turn off row numbers in the current sequence pane
    const QString gridStyle = "QTableWidget { gridline-color: #000000; }";
    ui->tableWidgetCurrentSequence->setStyleSheet(gridStyle);
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
        if (item != nullptr) {
            item->setData(Qt::UserRole, QVariant(formation));
        }
#endif /* ifdef NO_TIMING_INFO */
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
    QString s2 = s;
    s2.replace("[", "[ ");
    QStringList parts = s2.split(' ', Qt::SkipEmptyParts);
    for (int i = 0; i < parts.size(); ++i)
        parts[i].replace(0, 1, parts[i][0].toUpper());
    QString s3 = parts.join(" ");
    s3.replace("[ ", "[");  // handle the "explode and [right and left thru]" case
    return s3;
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
            // Cleanup/shorten resolve line, so it doesn't expand the whole UI section
            // TODO: This can be generalized.
            // TODO: The current algorithm only captures the first of multiple resolve lines.  This could be fixed.

            if (str == "promenade  (1/8 promenade, or couples 1/2 circulate, bend") {
                str = "promenade or COUPLES 1/2 circ, BTL, HOME";
            } else if (str == "reverse promenade  (7/8 promenade, or couples circulate") {
                str = "rev promenade or COUPLES circ 3-1/2, BTL, HOME";
            } else if (str == "reverse promenade  (1/8 promenade, or couples 1/2") {
                str = "rev promenade or COUPLES 1/2 circ, BTL, HOME";
            } else if (str == "promenade  (7/8 promenade, or couples circulate 3-1/2, bend") {
                str = "promenade or COUPLES circ 3-1/2, BTL, HOME";
            }

            str.replace("right and left grand", "RLG");
            str.replace("left allemande", "AL");
            str.replace("promenade", "prom");
            str.replace("swing and promenade", "SwProm");
            str.replace("couples", "COUPLES");
            str.replace("circulate", "circ");
            str.replace("bend the line", "BTL");
            str.replace("you're home", "HOME");
            str.replace("at home", "HOME");

            QRegularExpression parens("\\((.*)\\)");
            str.replace(parens, ", \\1"); // Promenade (1/2 promenade) --> Prom, 1/2 prom
            str = str.simplified(); // consolidate whitespace
            str.replace(" ,", ","); // get rid of extra space before comma

//            qDebug() << "str: " << str;
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

    if (str.startsWith("SD -- square dance") ||                 // startup time
            str.contains("current sequence will be aborted")) { // square your sets time
        // start of copyright section
        ui->listWidgetSDOutput->clear();
    }

    if (str.startsWith("Output file is"))
    {
        // end of copyright section, so reinit the engine
        sdLastLine = 0;
        sd_redo_stack->initialize();
        ui->label_SD_Resolve->clear(); // get rid of extra "(no matches)" or "left allemande" in resolve area when changing levels
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

                QString level = translateCallToLevel(thePrettifiedCall);

                if (level == "Mainstream") {
                    moveItem->setBackground(QBrush("#E0E0FF"));
                } else if (level == "Plus") {
                    moveItem->setBackground(QBrush("#BFFFC0"));
                } else if (level == "A1" || level == "A2") {
                    moveItem->setBackground(QBrush("#FFF0C0"));
                } else if (level == "C1") {
                    moveItem->setBackground(QBrush("#FEE0E0"));
                } else {
//                    qDebug() << "ERROR: unknown level for setting BG color of SD item: " << level;
                }

                ui->tableWidgetCurrentSequence->setItem(sdLastLine - 1, kColCurrentSequenceCall, moveItem);

//                qDebug() << "on_sd_add_new_line: adding " << thePrettifiedCall;

                // This is a terrible kludge, but I don't know a better way to do this.
                // The tableWidgetCurrentSequence is updated asynchronously, when sd gets around to sending output
                //   back to us.  There is NO indication that SD is done processing, near as I can tell.  So, there
                //   is no way for us to select the first row after SD is done.  If we try to do it before, BOOM.
                // This kludge gives SD 100ms to do all its processing, and get at least one valid row and item in there.
                QTimer::singleShot(100, [this]{
                    if (selectFirstItemOnLoad) {
                        selectFirstItemOnLoad = false;
                        ui->tableWidgetCurrentSequence->setFocus();
                        ui->tableWidgetCurrentSequence->clearSelection();
                        if (ui->tableWidgetCurrentSequence->item(0,0) != nullptr) { // make darn sure that there's actually an item there
                            // Surprisingly, both of the following are necessary to get the first row selected.
                            ui->tableWidgetCurrentSequence->setCurrentItem(ui->tableWidgetCurrentSequence->item(0,0));
                            ui->tableWidgetCurrentSequence->item(0,0)->setSelected(true);
                        }

                        checkSDforErrors(); // DEBUG DEBUG DEBUG
                    }
                    });

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
//                    lastCall += "\n<BR/><small>&nbsp;&nbsp;" + callTiming + "</small>";
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
//        qDebug() << "adding line to listWIdget: " << str;
    }
}


void MainWindow::render_sd_item_data(QTableWidgetItem *item)
{
//        qDebug() << "render_sd_item_data: " << item;
        if (item == nullptr) {
//            qDebug() << "render_sd_item_data RETURNING BECAUSE NULL POINTER";
            return; // for safety
        }
        QVariant v = item->data(Qt::UserRole);
        if (!v.isNull())
        {
//            qDebug() << "v: " << v;
            QString formation(v.toString());
//            qDebug() << "v string: " << formation;
            QStringList formationList = formation.split("\n");
            if (formationList.size() > 0)
            {
                decode_formation_into_dancer_destinations(formationList, sd_animation_people);  // also sets L/Tgroup and B/Gorder
                set_sd_last_groupness(); // update groupness strings

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
    Q_UNUSED(item)
//    qDebug() << "on_tableWidgetCurrentSequence_itemDoubleClicked: " << item;
    // regardless of clicking on col 0 or 1, use the variant formation squirreled away in column 1 (even if not visible)
//    render_sd_item_data(ui->tableWidgetCurrentSequence->item(item->row(),1));
}

void MainWindow::on_tableWidgetCurrentSequence_itemSelectionChanged()
{
//    qDebug() << "on_tableWidgetCurrentSequence_itemSelectionChanged()";
    // regardless of clicking on col 0 or 1, use the variant formation squirreled away in column 1 (even if not visible)
    QList<QTableWidgetItem *> selected = ui->tableWidgetCurrentSequence->selectedItems();
    if (selected.count() >= 1) {
        render_sd_item_data(ui->tableWidgetCurrentSequence->item(selected[0]->row(),1)); // selected row and col 1
    }
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
    if (textItem != nullptr) {
        textItem->setData(Qt::UserRole, QVariant(formation));
    }
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

void MainWindow::submit_lineEditSDInput_contents_to_sd(QString s, bool firstCall) // s defaults to ""
{
//    qDebug() << "original call: " << s;
    QString cmd;
    if (s.isEmpty()) {
        cmd = ui->lineEditSDInput->text().simplified(); // both trims whitespace and consolidates whitespace
    } else {
        cmd = s;
    }
//    QString cmd(ui->lineEditSDInput->text().simplified());  // both trims whitespace and consolidates whitespace
    cmd = cmd.toLower(); // on input, convert everything to lower case, to make it easier for the user
//    qDebug() << "cmd: " << cmd;
    ui->lineEditSDInput->clear();

    if (!cmd.compare("quit", Qt::CaseInsensitive))
    {
        cmd = str_abort_this_sequence;
    }

    // handle quarter, a quarter, one quarter, half, one half, two quarters, three quarters
    // must be in this order, and must be before number substitution.
    cmd = cmd.replace("1 & 1/2","1-1/2");
    cmd = cmd.replace("once and a half","1-1/2");
    cmd = cmd.replace("one and a half","1-1/2");
    cmd = cmd.replace("two and a half","2-1/2");
    cmd = cmd.replace("three and a half","3-1/2");
    cmd = cmd.replace("four and a half","4-1/2");
    cmd = cmd.replace("five and a half","5-1/2");
    cmd = cmd.replace("six and a half","6-1/2");
    cmd = cmd.replace("seven and a half","7-1/2");
    cmd = cmd.replace("eight and a half","8-1/2");

    // new from CSDS PLUS ===================================
    cmd = cmd.replace(" & "," and "); // down here to not disturb 1-1/2 processing
    cmd = cmd.replace(QRegularExpression("\\(.*\\)"),"").simplified(); // remove parens, trims whitespace

    // -------------------
    cmd = cmd.replace("four quarters","4/4");
    cmd = cmd.replace("three quarters","3/4").replace("three quarter","3/4");  // always replace the longer thing first!
    cmd = cmd.replace("halfway", "xyzzy123");
    cmd = cmd.replace("two quarters","2/4").replace("one half","1/2").replace("half","1/2");
    cmd = cmd.replace("xyzzy123","halfway");  // protect "halfway" from being changed to "1/2way"
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
    if (!cmd.contains("ladies chain") && !cmd.contains("men chain") && !cmd.contains("promenade")) {
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

            cmd.replace(andRollCall, "[\\1] and roll");
        }

        // explode must be handled *after* roll, because explode binds tightly with the call
        // e.g. EXPLODE AND RIGHT AND LEFT THRU AND ROLL must be translated to:
        //      [explode and [right and left thru]] and roll

        // first, handle both: "explode and <anything> and roll"
        //  by the time we're here, it's already "[explode and <anything>] and roll", because
        //  we've already done the roll processing.
        QRegularExpression explodeAndRollCall("\\[explode and (.*)\\] and roll");
        QRegularExpression explodeAndNotRollCall("^explode and (.*)");

        cmd.replace(explodeAndRollCall, "[explode and \\1] and roll");
        cmd.replace(explodeAndNotRollCall, "explode and [\\1]");

        // similar for Transfer and Anything, Clover and Anything, and Cross Clover and Anything
        QRegularExpression transferAndRollCall("\\[transfer and (.*)\\] and roll");
        QRegularExpression transferAndNotRollCall("^transfer and (.*)");

        cmd.replace(transferAndRollCall, "[transfer and \\1] and roll");
        cmd.replace(transferAndNotRollCall, "transfer and [\\1]");

        QRegularExpression cloverAndRollCall("\\[clover and (.*)\\] and roll");
        QRegularExpression cloverAndNotRollCall("^clover and (.*)");

        cmd.replace(cloverAndRollCall, "[clover and \\1] and roll");
        cmd.replace(cloverAndNotRollCall, "clover and [\\1]");

        QRegularExpression crossCloverAndRollCall("\\[cross clover and (.*)\\] and roll");
        QRegularExpression crossCloverAndNotRollCall("^cross clover and (.*)");

        cmd.replace(crossCloverAndRollCall, "[cross clover and \\1] and roll");
        cmd.replace(crossCloverAndNotRollCall, "cross clover and [\\1]");
    }

    // handle <ANYTHING> and spread
    if (!cmd.contains("[")) {
        QRegularExpression andSpreadCall("(.*) and spread");
//        if (cmd.indexOf(andSpreadCall) != -1) {
//            cmd = "[" + andSpreadCall.cap(1) + "] and spread";
//        }
        cmd.replace(andSpreadCall, "[\\1] and spread");
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

    cmd.replace("1/2 square thru", "square thru 2"); // must be first for "1/2 square thru"

    // handle "square thru" -> "square thru 4"
    //  and "heads square thru" --> "heads square thru 4"
    //  and "sides..."
    if (cmd.endsWith("square thru")) {
        cmd.replace("square thru", "square thru 4");
    }

    // SD COMMANDS -------

//    qDebug() << "CMD: " << cmd;

    // square your|the set -> square thru 4
    if (cmd == "square the set" || cmd == "square your set"
        || cmd == str_square_your_sets) {
        sdthread->do_user_input(str_abort_this_sequence);
        sdthread->do_user_input("y");
        ui->tableWidgetCurrentSequence->clear(); // clear the current sequence pane
        // sd_redo_stack->initialize(); // init the redo stack
    }
    else if (cmd == "2 couples only") {
        sdthread->do_user_input("two couples only");
        gTwoCouplesOnly = true;
    } else {
//        qDebug() << "ui->tableWidgetCurrentSequence->rowCount(): " << ui->tableWidgetCurrentSequence->rowCount() << "firstCall: " << firstCall;
        if ((firstCall || ui->tableWidgetCurrentSequence->rowCount() == 0) && !cmd.contains("1p2p")) {
            if (cmd.startsWith("heads ")) {
//                qDebug() << "ready to call do_user_input in submit_lineEditSDInput_contents_to_sd with: heads start";
                sdthread->do_user_input("heads start");
                cmd.replace("heads ", "");
            } else if (cmd.startsWith("sides")) {
//                qDebug() << "ready to call do_user_input in submit_lineEditSDInput_contents_to_sd with: sides start";
                sdthread->do_user_input("sides start");
                cmd.replace("sides ", "");
            }
        }
//        qDebug() << "ready to call do_user_input in submit_lineEditSDInput_contents_to_sd with: " << cmd;
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
    ui->listWidgetSDOutput->clear();  // we're restarting the engine, so clear the SDOutput window
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

//    qDebug() << "SETTING SD LEVEL TO: " << currentSDKeyboardLevel;
    prefsManager.SetSDLevel(currentSDKeyboardLevel); // persist the selected dance level

//    ui->tabWidget_2->setTabText(1, QString("Current Sequence: ") + currentSDKeyboardLevel); // Current {Level} Sequence

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

void MainWindow::on_actionSD_Output_triggered()
{
//    ui->tabWidget_2->setTabVisible(0, ui->actionSD_Output->isChecked()); // 0 = SD Output tab
    ui->listWidgetSDOutput->setVisible(ui->actionSD_Output->isChecked());
}

// ------------------------------
void SDDancer::setColor(const QColor &color)
{
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

        std::shuffle(std::begin(indexes), std::end(indexes), std::mt19937(std::random_device()())); // >= C++17

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

// static
void MainWindow::setPeopleNumberingScheme(QList<SDDancer> &sdpeople, const QString &numberScheme)
{
    // New numbers are: "None", "Numbers", "Names"
    bool showDancerLabels = (numberScheme != "None");

    if (numberScheme == "None") {
        // None/Invisible
    } else if (numberScheme == "Numbers"){
        // Numbers
        for (int dancerNum = 0; dancerNum < sdpeople.length(); ++dancerNum)
        {
            sdpeople[dancerNum].label->setPlainText("  " + QString::number(1 + dancerNum/2) + "  ");
        }
    } else {
        // Names
        for (int dancerNum = 0; dancerNum < sdpeople.length(); ++dancerNum)
        {
            QString dancerName;
            switch (dancerNum) {
                case 0: dancerName = ui->boy1->text(); break;
                case 1: dancerName = ui->girl1->text(); break;
                case 2: dancerName = ui->boy2->text(); break;
                case 3: dancerName = ui->girl2->text(); break;
                case 4: dancerName = ui->boy3->text(); break;
                case 5: dancerName = ui->girl3->text(); break;
                case 6: dancerName = ui->boy4->text(); break;
                case 7: dancerName = ui->girl4->text(); break;
                default: break;
            }
            sdpeople[dancerNum].label->setPlainText((dancerName + "   ").left(3));
        }
    }

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
        qDebug() << "UNDO -----";
        undo_last_sd_action();
    }
}

void MainWindow::paste_to_tableWidgetCurrentSequence() {

    // Clear out existing sequence, if there is one...
    on_actionSDSquareYourSets_triggered();
    ui->tableWidgetCurrentSequence->clear(); // clear the current sequence pane

    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasHtml()) {
       qDebug() << "ERROR: HTML FOUND -- " << mimeData->html();
    } else if (mimeData->hasText()) {
//        qDebug() << "PLAIN TEXT: " << mimeData->text();
        QString theText = mimeData->text(); // get plain text from clipboard
        theText.replace(QRegularExpression("\\(.*\\)", QRegularExpression::InvertedGreedinessOption), ""); // delete comments
        QStringList theCalls = theText.split(QRegularExpression("[\r\n]"),Qt::SkipEmptyParts); // listify, and remove blank lines
//        qDebug() << "theCalls: " << theCalls;

        selectFirstItemOnLoad = true;  // one-shot, when we get the first item actually loaded into tableWidgetCurrentSequence, select the first row
        sdthread->resetAndExecute(theCalls);  // OK LET'S TRY THIS

    } else {
        qDebug() << "ERROR: CANNOT UNDERSTAND CLIPBOARD DATA"; // error? warning?
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
                selection += "<BR/>" + s;
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

    QAction action1("Copy Sequence", this);
    connect(&action1, SIGNAL(triggered()), this, SLOT(copy_selection_from_tableWidgetCurrentSequence()));
    action1.setShortcut(QKeySequence::Copy);
    action1.setShortcutVisibleInContextMenu(true);
    contextMenu.addAction(&action1);

    QAction action5("Copy Sequence as HTML", this);
    connect(&action5, SIGNAL(triggered()), this, SLOT(copy_selection_from_tableWidgetCurrentSequence_html()));
    action5.setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C)); // MAC-SPECIFIC
    action5.setShortcutVisibleInContextMenu(true);
    contextMenu.addAction(&action5);

    QAction action1a("Paste Sequence", this);
    connect(&action1a, SIGNAL(triggered()), this, SLOT(paste_to_tableWidgetCurrentSequence()));
    action1a.setShortcut(QKeySequence::Paste);
    action1a.setShortcutVisibleInContextMenu(true);
    contextMenu.addAction(&action1a);

    contextMenu.addSeparator(); // ---------------

    QAction action2("Undo", this);
    connect(&action2, SIGNAL(triggered()), this, SLOT(undo_last_sd_action()));
    action2.setShortcut(QKeySequence::Undo);
    action2.setShortcutVisibleInContextMenu(true);
    contextMenu.addAction(&action2);

    QAction action25("Redo", this);
    connect(&action25, SIGNAL(triggered()), this, SLOT(redo_last_sd_action()));
    action25.setShortcut(QKeySequence::Redo);
    action25.setShortcutVisibleInContextMenu(true);
    action25.setEnabled(sd_redo_stack->can_redo());
    contextMenu.addAction(&action25);

    
    QAction action3("Select All", this);
    connect(&action3, SIGNAL(triggered()), this, SLOT(select_all_sd_current_sequence()));
    action3.setShortcut(QKeySequence::SelectAll);
    action3.setShortcutVisibleInContextMenu(true);
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
    restartSDThread(get_current_sd_dance_program());

    if (nullptr == sdthread) // NULL == sdthread)
    {
        qDebug() << "Something has gone wrong, sdthread is null!";
        restartSDThread(get_current_sd_dance_program());
    }
//    qDebug() << "Square your sets";

    ui->listWidgetSDOutput->clear();  // squaring up, so clear the SDOutput window, too.
    //  this allows us to scan for "(no matches)"
    ui->lineEditSDInput->clear();
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

void MainWindow::dancerNameChanged() {  // when text in any dancerName field changes
    // If any key is pressed AND we are in Names mode, update the names in the dancer icons
    if (ui->actionNames->isChecked()) {
        setSDCoupleNumberingScheme("Names");  // update dancer names in diagram
        on_sd_update_status_bar(sdLastFormationName); // update everything in the diagram
    }
}


void MainWindow::on_actionLoad_Sequence_triggered()
{
//    qDebug() << "on_actionLoad_Sequence_triggered";
    RecursionGuard dialog_guard(inPreferencesDialog);

    QString sequenceFilename = QFileDialog::getOpenFileName(this,
                                                    tr("Load SD Sequence"),
                                                    musicRootPath + "/sd",
                                                    tr("TXT (*.txt)"));
    if (!sequenceFilename.isNull())
    {
        QFile file(sequenceFilename);
        if ( file.open(QIODevice::ReadOnly) )
        {
            QTextStream in(&file);
            QString entireSequence = in.readAll();
//            qDebug() << file.size() << entireSequence; // print entire file!

            QStringList theCalls = entireSequence.toLower().split(QRegularExpression("[\r\n]"),Qt::SkipEmptyParts); // listify, and remove blank lines
//            qDebug() << "theCalls: " << theCalls;

//            callsToLoad = theCalls; // load these AFTER we restart the SD engine

            file.close();

//            qDebug() << "About to call: resetAndExecute() with these calls: " << theCalls;
            on_actionSDSquareYourSets_triggered();
            ui->tableWidgetCurrentSequence->clear(); // clear the current sequence pane
            selectFirstItemOnLoad = true; // when the resetAndExecute is done, do a one-time set of the selection to the first item
            sdthread->resetAndExecute(theCalls);  // OK LET'S TRY THIS
        }
    }
}


void MainWindow::on_actionSave_Sequence_As_triggered()
{
//    qDebug() << "on_actionSave_Sequence_As_triggered";
    saveSequenceAs(); // intentionally Save Sequence AS
}

void MainWindow::saveSequenceAs()
{
    // Ask me where to save it...
    RecursionGuard dialog_guard(inPreferencesDialog);

    QString sequenceFilename = QFileDialog::getSaveFileName(this,
                                                    tr("Save SD Sequence"),
                                                    musicRootPath + "/sd/sequence.txt",
                                                    tr("TXT (*.txt);;HTML (*.html *.htm)"));
    if (!sequenceFilename.isNull())
    {
        QFile file(sequenceFilename);
        if ( file.open(QIODevice::WriteOnly) )
        {
            QTextStream stream( &file );
            if (sequenceFilename.endsWith(".html", Qt::CaseInsensitive)
                || sequenceFilename.endsWith(".htm", Qt::CaseInsensitive))
            {
                QTextStream stream( &file );
                stream << get_current_sd_sequence_as_html(true, false);
            }
            else
            {
                for (int row = 0; row < ui->tableWidgetCurrentSequence->rowCount();
                     ++row)
                {
                    QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,0);
                    stream << item->text() + "\n";
                }
            }
            stream.flush();
            file.close();
        }
    }
}

void MainWindow::on_actionShow_Frames_triggered()
{
    bool vis = ui->actionShow_Frames->isChecked();
    ui->labelEasy->setVisible(vis);
    ui->labelMedium->setVisible(vis);
    ui->labelHard->setVisible(vis);
    ui->listEasy->setVisible(vis);
    ui->listMedium->setVisible(vis);
    ui->listHard->setVisible(vis);
}

void MainWindow::refreshSDframes() {
    // refreshSDframes ---------
    int whichSidebar = 0;
    QString frameTitleString("<html><head/><body><p><span style=\"font-weight:700; color:#0433ff;\">F%1</span><span style=\"font-weight:700;\"> %2%5 [%06%3/%4]</span></p></body></html>"); // %06 is intentional, do not use just %6
    QString editingInProgressIndicator = (newSequenceInProgress || editSequenceInProgress ? "*" : "");

    for (int i = 0; i < frameFiles.length(); i++) {
//        qDebug() << "frameFile: " << frameFiles[i] << ", frameVisible: " << frameVisible[i];
        if (frameVisible[i] == "sidebar") {
            whichSidebar += 1;
            switch (whichSidebar) {
                case 1:
                    ui->labelEasy->setText(frameTitleString.arg(i+1).arg(frameFiles[i]).arg(frameCurSeq[i]).arg(frameMaxSeq[i]).arg("").arg(""));
                    loadFrame(i, frameFiles[i], fmin(frameCurSeq[i], frameMaxSeq[i]), ui->listEasy);
                    break;
                case 2:
                    ui->labelMedium->setText(frameTitleString.arg(i+1).arg(frameFiles[i]).arg(frameCurSeq[i]).arg(frameMaxSeq[i]).arg("").arg(""));
                    loadFrame(i, frameFiles[i], fmin(frameCurSeq[i], frameMaxSeq[i]), ui->listMedium);
                    break;
                case 3:
                    ui->labelHard->setText(frameTitleString.arg(i+1).arg(frameFiles[i]).arg(frameCurSeq[i]).arg(frameMaxSeq[i]).arg("").arg(""));
                    loadFrame(i, frameFiles[i], fmin(frameCurSeq[i], frameMaxSeq[i]), ui->listHard);
                    break;
                default: break; // by design, only the first 3 sidebar frames found are loaded (FIX)
            }

        } else if (frameVisible[i] == "central") {
            loadFrame(i, frameFiles[i], frameCurSeq[i], NULL); // NULL means "use the Central widget, which is a table"; also sets the currentSequenceRecordNumber

//            qDebug() << "***** sequenceStatus: " << sequenceStatus;

            int thisSequenceNumber = currentSequenceRecordNumberi;
            int thisSequenceStatus = (sequenceStatus.contains(thisSequenceNumber) ? sequenceStatus[thisSequenceNumber] : 0); // 0 = not rated, 1 = good, else bad and # = reason code

//            qDebug() << "thisSequenceNumber/Status: " << thisSequenceNumber << thisSequenceStatus;

            QString statusString = "";
            switch (thisSequenceStatus) {
                case 0:  statusString = "";                                                              break;  // no string (not evaluated yet)
                case 1:  statusString = "<span style=\"font-weight:700; color:#008000;\">GOOD: </span>"; break;  // dark green
                default: statusString = "<span style=\"font-weight:700; color:#C00000;\">BAD: </span>";  break;  // red
            }
            QString html1 = frameTitleString.arg(i+1).arg(frameFiles[i]).arg(frameCurSeq[i]).arg(frameMaxSeq[i]).arg(editingInProgressIndicator).arg(statusString);
            currentFrameTextName = frameFiles[i]; // save just the name of the frame
            currentFrameHTMLName = html1;         // save fancy string

            if ( ui->pushButtonSDSave->menu() != nullptr ) {
                ui->pushButtonSDSave->menu()->actions()[0]->setText(QString("Save Current Sequence to ") + currentFrameTextName);       // first one in the list is Save to Current (item 0)
                ui->pushButtonSDSave->menu()->actions()[1]->setText(QString("Delete Current Sequence from ") + currentFrameTextName);   // second one in the list is Delete from Current (item 1)
            }

//            ui->labelWorkshop->setText(html1);          // use fancy string
            ui->label_CurrentSequence->setText(html1);  // use fancy string

//            loadFrame(i, frameFiles[i], frameCurSeq[i], NULL); // NULL means "use the Central widget, which is a table"
//            ui->tableWidgetCurrentSequence->selectRow(1);  // When we use F11 or F12 and refresh the frames, affecting the Current Sequence frame, select the first row (TODO: always, or just Dance Arranger mode?)
        }
    }
}

// TODO: If we ask to load a frame, and that frame is already loaded, just return...fixes the F12 F12 F12 visual noise problem.

// loads a specified frame from a file in <musicDir>/sd/ into a listWidget (if not NULL), else into the tableSequence widget
void MainWindow::loadFrame(int i, QString filename, int seqNum, QListWidget *list) {
//    qDebug() << "----- loadFrame: " << filename << seqNum;
    QStringList callList;

    if (list != nullptr) {
        list->clear();  // clear out the current contents
    }

    // NOTE: THIS IS WHERE THE FILENAME STRUCTURE IS DEFINED:
    QFile theFile((musicRootPath + "/sd/frames/" + frameName + "/" + filename + ".txt"));
//    qDebug() << "loadFrame: " << theFile.fileName();

    if(!theFile.open(QIODevice::ReadOnly)) {
        QMessageBox::information(0, "ERROR", theFile.errorString()); // if file does not exist...
    } else {
        // now read in the lines of the file, looking for the sequence we want
        QTextStream in(&theFile);

        int seq = 0;
        bool wantThisSequence = false;
        while(!in.atEnd()) {
            QString line = in.readLine();
            // typical first two lines in a sequence:
            // #REC=1658300002#
            // #AUTHOR=16583#
            if (line.startsWith("#REC=")) {
                seq++;
                wantThisSequence = (seq == seqNum); // want it, iff the seqNum matches the one we're looking at

                if (frameVisible[i] == "central") {
                    // if this is the central frame, then remember the REC
                    line = line.replace("#REC=", "").replace("#", "");  // #REC=<record number># --> <record number>
                    currentSequenceRecordNumber = line ;
//                    qDebug() << "currentSequenceRecordNumber: " << currentSequenceRecordNumber;
                }
                continue;
            } else if (line.startsWith("#AUTHOR=")) {
                if (frameVisible[i] == "central") {
                    // if this is the central frame, then remember the AUTHOR
                    line = line.replace("#AUTHOR=", "").replace("#", "");  // #AUTHOR=<string># --> <string>
                    currentSequenceAuthor = line;
                }
                continue;
            } else if (line.startsWith("@")) {
                if (wantThisSequence) {
//                    qDebug() << "ACTUAL currentSequenceRecordNumber: " << currentSequenceRecordNumber;
                    currentSequenceRecordNumberi = currentSequenceRecordNumber.toInt();  // convert to int and squirrel it away for use by sequenceStatus

                    break;  // break out of the loop, if we just read all the lines for seqNum
                }
            } else if (line.startsWith("#")) {
                continue; // skip #PROOFREAD#, #EASY#, #SEQTYPE#, #AUTHOR=...#, etc.
            } else if (wantThisSequence){
                // this is the place!
//                qDebug() << "FOUND SEQ: " << seq << ": " << line;

                // TODO: this string processing needs to be made into a function that returns a QStringList (because
                //   some strings like "Heads Lead Right & Veer Left" will return TWO calls to be sent to SD)
                line = line.replace(QRegularExpression(",$"), "");
                line = line.replace(QRegularExpression("^heads", QRegularExpression::CaseInsensitiveOption),    "HEADS");
                line = line.replace(QRegularExpression("^sides", QRegularExpression::CaseInsensitiveOption),    "SIDES");
                line = line.replace(QRegularExpression("^all ", QRegularExpression::CaseInsensitiveOption),    "ALL "); // NOTE: spaces
                line = line.replace(QRegularExpression("^centers", QRegularExpression::CaseInsensitiveOption),  "CENTERS");
                line = line.replace(QRegularExpression("^ends", QRegularExpression::CaseInsensitiveOption),     "ENDS");
                if (list != nullptr) {
                    list->addItem(line);    // add to a list widget
                } else {
                    callList.append(line);  // add to the tableSequence widget (below)
                }
            }
        }

        if (!wantThisSequence) {
            // sequence not found!, wantThisSequence will be true if one was found and we broke out of the while loop, false otherwise
            if (list != nullptr) {
                list->addItem(QString("Sequence #%1 not found in file.").arg(seqNum));
            } else {
//                qDebug() << "TODO: Sequence not found for Central widget, DO SOMETHING HERE";
            }
        }

        if (callList.length() != 0) {
            // we are loading the central widget now
//            qDebug() << "Loading central widget with level: " << level << ", callList: " << callList;

            // in the new scheme, we do NOT change the level away from what the user selected.
//            setCurrentSDDanceProgram(dlevel);        // first, we need to set the SD engine to the level for these calls

//            on_actionSDSquareYourSets_triggered();   // second, init the SD engine (NOT NEEDED?)
            ui->tableWidgetCurrentSequence->clear(); // third, clear the current sequence pane
            selectFirstItemOnLoad = true;  // one-shot, when we get the first item actually loaded into tableWidgetCurrentSequence, select the first row

//            qDebug() << "**********************************";
//            qDebug() << "loadFrame() SEQUENCE TO BE SENT TO SD: " << callList;

            sdthread->resetAndExecute(callList);     // finally, send the calls in the list to SD.
        }

        theFile.close();
    }
//    qDebug() << "----- DONE";
}

// take care of F1 - F12 and UP/DOWN/LEFT/RIGHT and combinations when SD tab is showing ------------------------
bool MainWindow::handleSDFunctionKey(QKeyCombination keyCombo, QString text) {
    Q_UNUSED(text)

    int  key = keyCombo.key();
    int  modifiers = keyCombo.keyboardModifiers();
    bool shiftDown   = modifiers & Qt::ShiftModifier;       // true iff SHIFT was down when key was pressed
    bool commandDown = modifiers & Qt::ControlModifier;     // true iff CMD (MacOS) or CTRL (others) was down when key was pressed

//    qDebug() << "handleSDFunctionKey(" << key << ")" << shiftDown;
    if (inPreferencesDialog || !trapKeypresses || (prefDialog != nullptr)) {
        return false;
    }
//    qDebug() << "YES: handleSDFunctionKey(" << key << ")";

    int centralIndex = frameVisible.indexOf("central");
    int newIndex;

    QString pathToSequenceUsedFile = (musicRootPath + "/sd/frames/" + frameName + "/sequenceUsed.csv");
    QFile currentFile(pathToSequenceUsedFile);
    QFileInfo info(pathToSequenceUsedFile);

    if (shiftDown) {
        // SHIFT-something
        switch (key) {
        case Qt::Key_Left:  break;  // TODO: go to previous frame
        case Qt::Key_Right: break;  // TODO: go to next frame
        case Qt::Key_Up:    break;
        case Qt::Key_Down:  break;
        case Qt::Key_F1:
        case Qt::Key_F2:
        case Qt::Key_F3:
        case Qt::Key_F4:    break;
        case Qt::Key_F10:   break;
        case Qt::Key_F11:
//            qDebug() << "SHIFT-F11";
            newIndex = fmax(centralIndex - 1, 0);
            qDebug() << "    central/new index: " << centralIndex << newIndex;
            frameVisible[centralIndex] = "sidebar";
            frameVisible[newIndex]     = "central";
            refreshSDframes(); // load 3 sidebar and 1 central frame, update labels, update context menu for Save button
            break;
        case Qt::Key_F12:
//            qDebug() << "SHIFT-F12";
            newIndex = fmin(centralIndex + 1, 3);
            qDebug() << "    central/new index: " << centralIndex << newIndex;
            frameVisible[centralIndex] = "sidebar";
            frameVisible[newIndex]     = "central";
            refreshSDframes(); // load 3 sidebar and 1 central frame, update labels, update context menu for Save button
            break;
        default: break;
        }
    } else if (commandDown) {
        // CMD-something (MacOS) or CTRL-something (others)
        switch (key) {
        case Qt::Key_Left:  break;
        case Qt::Key_Right: break;
        case Qt::Key_Up:    break;
        case Qt::Key_Down:  break;
        case Qt::Key_F1:
        case Qt::Key_F2:
        case Qt::Key_F3:
        case Qt::Key_F4:    break;
        case Qt::Key_F10:   break;
        case Qt::Key_F11:
            // CMD-F11 = go to sequence 1 in current frame
            qDebug() << "CMD-F11";
            frameCurSeq[centralIndex] = 1;
            SDSetCurrentSeqs(1); // persist the new Current Sequence numbers
            refreshSDframes();  // load 3 sidebar and 1 central frame, update labels, update context menu for Save button
            break;
        case Qt::Key_F12:
            // go to last sequence in current frame
//            qDebug() << "CMD-F12";
            frameCurSeq[centralIndex] = frameMaxSeq[centralIndex];
            SDSetCurrentSeqs(2); // persist the new Current Sequence numbers
            refreshSDframes();  // load 3 sidebar and 1 central frame, update labels, update context menu for Save button
            break;
        default: break;
        }
    } else {
        // key with no relevant modifiers
        switch (key) {
        case Qt::Key_Left:  break;
        case Qt::Key_Right: break;
        case Qt::Key_Up:    break;
        case Qt::Key_Down:  break;
        case Qt::Key_F1:
        case Qt::Key_F2:
        case Qt::Key_F3:
        case Qt::Key_F4:
            // F1, F2, F3, F4 means go to a specific frame
//            qDebug() << "F1/2/3/4";
            frameVisible[centralIndex] = "sidebar";
            frameVisible[key - Qt::Key_F1] = "central";
            refreshSDframes(); // load 3 sidebar and 1 central frame, update labels, update context menu for Save button
            break;

        case Qt::Key_F10:   break;

        case Qt::Key_F11:
            // Decrement sequence number in current frame
            // only do something if we are NOT looking at the first sequence in the current frame
//            qDebug() << "F11";
            if (frameCurSeq[centralIndex] > 1) {
                frameCurSeq[centralIndex] = (int)(fmax(1, frameCurSeq[centralIndex] - 1)); // decrease, but not below ONE
            }
            SDSetCurrentSeqs(3); // persist the new Current Sequence numbers
            refreshSDframes(); // load 3 sidebar and 1 central frame, update labels, update context menu for Save button
            break;

        case Qt::Key_B:
        case Qt::Key_G:
            // Mark current sequence as "used", either "BAD" or "GOOD"

            if (!info.exists()) {
                if (currentFile.open(QFile::WriteOnly | QFile::Append)) {
                    // file is created, and CSV header is written
                    QTextStream out(&currentFile);
                    out << "datetime,userID,sequenceID,status,reason\n";
                    currentFile.close();
                } else {
                    qDebug() << "ERROR: could not make a new sequenceUsed file: " << pathToSequenceUsedFile;
                }
            }

            if (currentFile.open(QFile::WriteOnly | QFile::Append | QFile::ExistingOnly)) {
                QTextStream out(&currentFile);

                int thisSequenceNumber = currentSequenceRecordNumberi; // the sequence number (REC) of the one in the current sequence pane

                QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
                if (key == Qt::Key_B) {
                    // MARK SEQUENCE AS BAD/USED, THEN MOVE TO NEXT SEQUENCE (equiv to CSDS Ctrl-P)
                    //  Note: marking a sequence as BAD will override the GOOD flag that might have been set earlier.
                    //        The LAST indicator in the sequenceUsed file will be the latest one.
                    out << now << "," << currentSequenceAuthor << "," << currentSequenceRecordNumber << ",BAD,NA\n"; // BAD: reason is always NA right now (TODO: add a reason code here!)

                    // TODO XYZZY: Should B clear B (and NOT advance to the next sequence)? That would be simplest! ************

                    // UPDATE LOCAL CACHE: 0 = not rated, 1 = good, else bad and # = reason code
                    sequenceStatus[thisSequenceNumber] = 2;  // 2 = BAD and Reason Code == NA; later: >2 == reason code

                } else {
                    // MARK SEQUENCE AS GOOD/USED, THEN MOVE TO NEXT SEQUENCE  (equiv to CSDS U)
                    //  Note: marking a sequence as GOOD will override the BAD flag that might have been set earlier.
                    //        The LAST indicator in the sequenceUsed file will be the latest one.
                    out << now << "," << currentSequenceAuthor << "," << currentSequenceRecordNumber << ",GOOD,NA\n"; // GOOD: reason is always NA

                    // TODO XYZZY: Should G clear G (and NOT advance to the next sequence)?  That would be simplest! ************

                    // UPDATE LOCAL CACHE: 0 = not rated, 1 = good, else bad and # = reason code
                    sequenceStatus[thisSequenceNumber] = 1;  // 1 = GOOD and Reason Code == NA
                }

                currentFile.close();
            } else {
                qDebug() << "ERROR: could not open sequenceUsed file for appending: " << pathToSequenceUsedFile;
            }

            [[fallthrough]];

        case Qt::Key_F12:
            // increment sequence number in current frame
            // only do something if we are NOT looking at the last sequence in the current frame
//            qDebug() << "F12";
            if (frameCurSeq[centralIndex] < frameMaxSeq[centralIndex]) {
                frameCurSeq[centralIndex] = (int)(fmin(frameMaxSeq[centralIndex], frameCurSeq[centralIndex] + 1)); // increase, but not above how many we have
            }
            SDSetCurrentSeqs(4); // persist the new Current Sequence numbers
            refreshSDframes(); // load 3 sidebar and 1 central frame, update labels, update context menu for Save button
            // TODO: should set selection to first item in list, if in Dance Arranger
            break;

        default: break;
        }
    }

    return(true);
}

// SD editing buttons ------
void MainWindow::on_pushButtonSDUnlock_clicked()
{
//    qDebug() << "SDUnlock triggered";
    ui->pushButtonSDSave->setVisible(true);
    ui->pushButtonSDUnlock->setVisible(false);
    ui->pushButtonSDNew->setVisible(false);

    ui->lineEditSDInput->setVisible(true);
    ui->lineEditSDInput->setFocus();

    editSequenceInProgress = true;
    refreshSDframes();  // clear the * editing indicator
}

void MainWindow::SDExitEditMode() {

    ui->pushButtonSDSave->setVisible(false);
    ui->pushButtonSDUnlock->setVisible(true);
    ui->pushButtonSDNew->setVisible(true);

    ui->lineEditSDInput->setVisible(false);
    ui->tableWidgetCurrentSequence->setFocus();

    // we're exiting edit, so highlight the first line
    ui->tableWidgetCurrentSequence->clearSelection();
    if (ui->tableWidgetCurrentSequence->item(0,0) != nullptr) { // make darn sure that there's actually an item there
        // Surprisingly, both of the following are necessary to get the first row selected.
        ui->tableWidgetCurrentSequence->setCurrentItem(ui->tableWidgetCurrentSequence->item(0,0));
        ui->tableWidgetCurrentSequence->item(0,0)->setSelected(true);
    }

    newSequenceInProgress = false;
    editSequenceInProgress = false;
    refreshSDframes();  // clear the * editing indicator
}

void MainWindow::SDSetCurrentSeqs(int i) {
    Q_UNUSED(i)
//    qDebug() << "SDSet: " << i;
    // persist the frameCurSeq values into /sd/<frameName>/.current.csv (don't worry, it won't take very long!)
    QString pathToCurrentSeqFile = (musicRootPath + "/sd/frames/" + frameName + "/.current.csv");
    QFile currentFile(pathToCurrentSeqFile);
    if (currentFile.open(QFile::WriteOnly)) {
        QTextStream out(&currentFile);
        out << "filename,currentSeqNum,maxSeqNum\n";  // HEADER
        for (int i = 0; i < frameFiles.length(); i++) {
            out << frameFiles[i] << "," << frameCurSeq[i] << "," << frameMaxSeq[i] << "\n"; // persist all the values from this session
        }
        currentFile.close();
    } else {
        qDebug() << "ERROR: could not open for writing: " << pathToCurrentSeqFile;
    }
}

void MainWindow::SDGetCurrentSeqs() {
//    qDebug() << "SDGetCurrentSeqs triggered";
    // currently viewed sequence numbers are stored persistently in /sd/.current.csv, so that
    //  they are shared automatically and are kept consistent across machines (e.g. synced with Dropbox or Box.net).
    // They are intentionally not single-laptop-specific states, they are entire-SD-sequence-data states, so they move with the musicDir.
    // This is a standard CSV file, with filename and sequence numbers.
    // If the current numbers are greater than the scanned-for max numbers, the current numbers are adjusted to the max.

    QString pathToCurrentSeqFile = (musicRootPath + "/sd/frames/" + frameName + "/.current.csv");
    QFile f(pathToCurrentSeqFile);
    if (QFileInfo::exists(pathToCurrentSeqFile)) {
        if (!f.open(QFile::ReadOnly | QFile::Text)) {
            qDebug() << "ERROR: file exists, but could not open: " << f.fileName();
        } else {
            QTextStream in(&f);
            // simple CSV parser, for exactly 2 fields: filename (string), currentSequenceNumber (uint)
            while (!in.atEnd()) {
                QString line = in.readLine();
//                qDebug() << "current.csv line: " << line;
                QStringList fields = line.split(','); // should be exactly 2 fields
                if (!(fields[0] == "filename")) {
                    // if not the header line
                    QString filename = fields[0];
                    int currentSeqNum = fields[1].toInt();
                    int which = frameFiles.indexOf(filename);
                    if (which != -1) {
//                        qDebug() << "Setting frameCurSeq[" << which << "] to " << currentSeqNum;
//                        frameCurSeq[which] = (int)fmin(currentSeqNum, frameMaxSeq[which]); // can't be bigger than actual known max seqNum
                        frameCurSeq[which] = currentSeqNum; // do the MAX later
                    }
                }
            }
            f.close();
        }
    } else {
        // use 1 internally
        qDebug() << "Didn't find .current.csv, so making one with all 1's";
        for (int i = 0; i < frameFiles.length(); i++) {
            frameCurSeq[i] = 1; // if .current.csv file does not exist yet, just init all to zero internally
        }
        // create a new /sd/.current.csv which has 1 for every known file
        QFile currentFile(pathToCurrentSeqFile);
        if (currentFile.open(QFile::WriteOnly)) {
            QTextStream out(&currentFile);
            out << "filename,currentSeqNum,maxSeqNum\n";  // HEADER
            for (auto s: frameFiles) {
                out << s << ",1\n"; // set CurSeq = 1 for all known files.
            }
            currentFile.close();
        } else {
            qDebug() << "ERROR: could not open for writing: " << pathToCurrentSeqFile;
        }
    }
//    qDebug() << "Final frameFiles: "  << frameFiles;
//    qDebug() << "Final frameMaxSeq: " << frameMaxSeq;
//    qDebug() << "Final frameCurSeq: " << frameCurSeq;
}


void MainWindow::SDScanFramesForMax() { // i = 0 to 6
//    qDebug() << "SDScanFramesForMax triggered";

    for (int i = 0; i < frameVisible.length(); i++) {
//        qDebug() << "MAGIC [" << i << "]:" << frameCurSeq[i];
        // for each frame (0 - 6)
        QString fileName = frameFiles[i]; // get the filename
        QString pathToScanFile = (musicRootPath + "/sd/frames/" + frameName + "/%1.txt").arg(fileName); // NOTE: FILENAME STRUCTURE IS HERE, TOO (TODO: FACTOR THIS)

        QFile file(pathToScanFile);
        int AtCount = 0;
        if (file.open(QIODevice::ReadOnly))
        {
//            qDebug() << "Scanning for max: " << pathToScanFile;
            while(!file.atEnd()) {
                QString line = file.readLine();
//                qDebug() << "frameName: " << frameName << ", line: " << line;
                if (!line.isEmpty() && line.front() == '@') {
                    AtCount++;
                }
            }

//            qDebug() << "     MAX RESULT:"  << AtCount - 1;
//            qDebug() << "XYZZY: " << frameCurSeq[i] << frameMaxSeq[i];
            frameMaxSeq[i] = AtCount - 1;  // one sequence = @ seq @
            frameCurSeq[i] = (int)fmax(0, fmin(frameCurSeq[i], frameMaxSeq[i])); // ensure that CurSeq is 1 - N
//            qDebug() << "    XYZZY2: " << frameCurSeq[i] << frameMaxSeq[i];
            file.close();
        } else {
            frameMaxSeq[i] = 1;
            frameCurSeq[i] = 1;
            qDebug() << "File " << pathToScanFile << " could not be opened.";
        }

    }

    SDSetCurrentSeqs(5); // persist the new Current Sequence numbers

    // refreshSDframes(); // we have new framMax's now

//    qDebug() << "Final frameFiles: "  << frameFiles;
//    qDebug() << "Final frameLevel: "  << frameLevel;
//    qDebug() << "Final frameMaxSeq: " << frameMaxSeq;
//    qDebug() << "Final frameCurSeq: " << frameCurSeq;
}


void MainWindow::SDAppendCurrentSequenceToFrame(int i) {
//    qDebug() << "SDAppendCurrentSequenceToFrame " << i << frameFiles[i] << " triggered";

//    QString who     = QString(frameFiles[i]).replace(QRegularExpression("\\..*"), "");
//    QString level   = QString(frameFiles[i]).replace(QRegularExpression("^.*\\."), ""); // TODO: filename is <name>.<level> right now

//    QString pathToAppendFile = (musicRootPath + "/sd/%1/SStoSS/%2.choreodb_to_csds.in").arg(who).arg(level);
    QString pathToAppendFile = (musicRootPath + "/sd/%1.seq.txt").arg(frameFiles[i]);

//    qDebug() << "APPEND: currentFrameTextName/who/level/path: " << currentFrameTextName << who << level << pathToAppendFile;

    QFile file(pathToAppendFile);
    if (file.open(QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << "#REC=" << 100000 * userID + nextSequenceID << "#\n";     // Use a new sequence number, whenever edited
        nextSequenceID++;  // increment sequence number every time it's saved (it's a new sequence, right?)
        writeMetadata(userID, nextSequenceID, authorID);   // update the metadata file real quick with the new nextSequenceID
        stream << "#AUTHOR=" << authorID << "#\n";    // Use the new author string here

        for (int i = 0; i < ui->tableWidgetCurrentSequence->rowCount(); i++) {
//            qDebug() << "APPENDING: " << ui->tableWidgetCurrentSequence->item(i, 0)->text();
            stream << ui->tableWidgetCurrentSequence->item(i, 0)->text() << "\n";
        }

//        qDebug() << "APPEND (RESOLVE TEXT): " << ui->label_SD_Resolve->text();

        stream << "@\n";  // end of sequence
        file.close();

        // Appending increases the number of sequences in a frame, but it does NOT take us to the new frame.  Continue with the CURRENT frame.
        //int centralNum = frameVisible.indexOf("central");
        frameMaxSeq[i] += 1;    // add a NEW sequence to the receiving frame
                                // but do not change the CurSeq of that receiving frame
        refreshSDframes();
    } else {
        qDebug() << "ERROR: could not append to " << pathToAppendFile;
    }
    SDExitEditMode();
}

void MainWindow::SDReplaceCurrentSequence() {
    int currentFrame  = frameVisible.indexOf("central"); // 0 to M
    int currentSeqNum = frameCurSeq[currentFrame]; // 1 to N
//    qDebug() << "SDReplaceCurrentSequence REPLACING SEQUENCE #" << currentSeqNum << " FROM " << frameFiles[currentFrame];

//    QString who     = QString(frameFiles[currentFrame]).replace(QRegularExpression("\\..*"), "");
//    QString level   = QString(frameFiles[currentFrame]).replace(QRegularExpression("^.*\\."), ""); // TODO: filename is <name>.<level> right now

    // open the current file for READ ONLY
//    QString pathToOLDFile = (musicRootPath + "/sd/%1/SStoSS/%2.choreodb_to_csds.in").arg(who).arg(level);
    QString pathToOLDFile = (musicRootPath + "/sd/%1.txt").arg(frameFiles[currentFrame]);
    QFile OLDfile(pathToOLDFile);

    if (!OLDfile.open(QIODevice::ReadOnly)) {
        qDebug() << "ERROR: could not open " << pathToOLDFile;
        return;
    }

    // in SD directory, open a new file for APPEND called "foo.bar.NEW", which is where we will build a new file to replace the old one
//    QString pathToNEWFile = (musicRootPath + "/sd/%1/SStoSS/%2.choreodb_to_csds.in.NEW").arg(who).arg(level);
    QString pathToNEWFile = (musicRootPath + "/sd/%1.txt.NEW").arg(frameFiles[currentFrame]);
    QFile NEWfile(pathToNEWFile);

    if (!NEWfile.open(QIODevice::Append)) {
        qDebug() << "ERROR: could not open " << pathToNEWFile;
        OLDfile.close(); // we know that this one WAS opened correctly, so close it
        return;
    }

    // DO THE REPLACEMENT -------------------------
    QTextStream inFile(&OLDfile);
    QTextStream outFile(&NEWfile);

    int sn = 0; // current sequence number as we scan
    while(!inFile.atEnd()) { // for each line in OLDfile
        QString line = inFile.readLine();
        if (line == "@") {
            sn++;
            if (sn == currentSeqNum) {
                // This IS the one we want to replace, so let's write out the replacement right here to the outFile
                outFile << "@\n"; // First item in the replacement sequence is the '@'
                outFile << "#REC=" << 100000 * userID + nextSequenceID << "#\n";     // Use a new sequence number, whenever edited
                nextSequenceID++;  // increment sequence number every time it's saved (it's a new sequence, right?)
                writeMetadata(userID, nextSequenceID, authorID);   // update the metadata file real quick with the new nextSequenceID
                outFile << "#AUTHOR=" << authorID << "#\n";    // Use the new author here

                if (ui->tableWidgetCurrentSequence->rowCount() >= 1) {
                    for (int i = 0; i < ui->tableWidgetCurrentSequence->rowCount(); i++) {
                        outFile << ui->tableWidgetCurrentSequence->item(i, 0)->text() << "\n"; // COPY IN THE REPLACEMENT
                    }
                } else {
                    outFile << "just as you are\n"; // EMPTY SEQUENCE
                }

                QString resolve = ui->label_SD_Resolve->text().simplified();
//                qDebug() << "REPLACE (RESOLVE TEXT): " << resolve;
                if (resolve != "") {
                    outFile << "( " << resolve << " )\n";  // write a comment with the resolve, if it's not NULL
                }
            }
        }
        if (sn != currentSeqNum) {
            // NOT the one we want to delete, so append to the NEW file
            outFile << line << "\n";
        }
    }

//    qDebug() << "XYZZY: " << currentSeqNum << frameMaxSeq[currentFrame] << newSequenceInProgress;
    if (newSequenceInProgress) {
        // if we're replace the very last "@" in the file, it means that this must be a NEW then SAVE
        //   so, add the EOF @
        outFile << "@\n";
    }

    // close both files
    NEWfile.close();
    OLDfile.close();

    // rename foo.bar --> foo.bar.OLD
    QFile::rename(pathToOLDFile, pathToOLDFile + ".BU");

    // rename foo.bar.NEW --> foo.bar
    QFile::rename(pathToNEWFile, pathToOLDFile);

    // delete foo.bar.OLD
    QFile::remove(pathToOLDFile + ".BU");

    // REPLACEMENT HAS NO EFFECT ON FRAME INFO
    // NO NEED TO REFRESH FRAMES, BECAUSE FRAMES ARE ALL FINE

    SDExitEditMode();
}

void MainWindow::SDDeleteCurrentSequence() {
    int currentFrame  = frameVisible.indexOf("central"); // 0 to M
    int currentSeqNum = frameCurSeq[currentFrame]; // 1 to N
//    qDebug() << "SDDeleteCurrentSequence DELETING SEQUENCE #" << currentSeqNum << " FROM " << frameFiles[currentFrame];

//    QString who     = QString(frameFiles[currentFrame]).replace(QRegularExpression("\\..*"), "");
//    QString level   = QString(frameFiles[currentFrame]).replace(QRegularExpression("^.*\\."), ""); // TODO: filename is <name>.<level> right now

    // open the current file for READ ONLY
//    QString pathToOLDFile = (musicRootPath + "/sd/%1/SStoSS/%2.choreodb_to_csds.in").arg(who).arg(level);
    QString pathToOLDFile = (musicRootPath + "/sd/%1.seq.txt").arg(frameFiles[currentFrame]);
    QFile OLDfile(pathToOLDFile);

    if (!OLDfile.open(QIODevice::ReadOnly)) {
        qDebug() << "ERROR: could not open " << pathToOLDFile;
        return;
    }

    // in SD directory, open a new file for APPEND called "foo.bar.NEW", which is where we will build a new file to replace the old one
//    QString pathToNEWFile = (musicRootPath + "/sd/%1/SStoSS/%2.choreodb_to_csds.in.NEW").arg(who).arg(level);
    QString pathToNEWFile = (musicRootPath + "/sd/%1.seq.txt.NEW").arg(frameFiles[currentFrame]);
    QFile NEWfile(pathToNEWFile);

    if (!NEWfile.open(QIODevice::Append)) {
        qDebug() << "ERROR: could not open " << pathToNEWFile;
        OLDfile.close(); // we know that this one WAS opened correctly, so close it
        return;
    }

    // DO THE REPLACEMENT -------------------------
    QTextStream inFile(&OLDfile);
    QTextStream outFile(&NEWfile);

    int sn = 0; // current sequence number as we scan
    while(!inFile.atEnd()) { // for each line in OLDfile
        QString line = inFile.readLine();
        if (line == "@") {
            sn++;
        }
        if (sn != currentSeqNum) {
            // NOT the one we want to delete, so append to the NEW file
            outFile << line << "\n";
        }
    }

    // close both files
    NEWfile.close();
    OLDfile.close();

    // rename foo.bar --> foo.bar.OLD
    QFile::rename(pathToOLDFile, pathToOLDFile + ".BU");

    // rename foo.bar.NEW --> foo.bar
    QFile::rename(pathToNEWFile, pathToOLDFile);

    // delete foo.bar.OLD
    QFile::remove(pathToOLDFile + ".BU");

    // update frame info -----
    // NOTE: if a frame has ZERO sequences in it, frameCurSeq == frameMaxSeq == 0
    //    otherwise, 1 <= frameCurSeq <= frameMaxSeq
    frameMaxSeq[currentFrame] = (int)fmax(frameMaxSeq[currentFrame] - 1, 0);                     // 1 fewer sequences now
    frameCurSeq[currentFrame] = (int)fmin(frameMaxSeq[currentFrame], frameCurSeq[currentFrame]); // current might decrease by 1 if we deleted the last sequence, otherwise no change to CurSeq

    SDSetCurrentSeqs(6); // persist the new Current Sequence numbers

    refreshSDframes(); // show the new current frame

    SDExitEditMode();
}

void MainWindow::SDMoveCurrentSequenceToFrame(int i) {  // i = 0 to 6
    int currentFrame = frameVisible.indexOf("central");

    if (i == currentFrame) {
        // save to Current Frame (replace sequence in current frame)
//        qDebug() << "MoveCurrentSequenceToFrame(current) => ReplaceCurrentSequence" << i << currentFrame;
        SDReplaceCurrentSequence();
    } else {
        // save to a different frame (NOT the current frame)
//        qDebug() << "SDMoveCurrentSequenceToFrame " << i << frameFiles[i] << " triggered, then exiting edit mode";

        SDAppendCurrentSequenceToFrame(i);          // append a copy of the current sequence to a different frame
        SDDeleteCurrentSequence();                  // delete current sequence from current frame
    }
    // SDExitEditMode();  // NOT NEEDED HERE: already invoked by all of the Replace, Append, and Delete operations
}

void MainWindow::on_pushButtonSDNew_clicked()
{
//    qDebug() << "on_pushButtonSDNew_clicked()";
    on_actionSDSquareYourSets_triggered(); // clear everything out of the Current Sequence window
    on_pushButtonSDUnlock_clicked(); // unlock

    // ====== new idea: don't do anything to files at NEW time, only at SAVE or APPEND or REPLACE time =========
    // This idea requires that we distinguish between a NEW sequence (unsaved) and a LOADED sequence (already in the DB).
    // In the first case, we want to increment the sequence number then APPEND.  In the second, we want to
    // increment and then REPLACE.  Right now, since we have a placeholder due to NEW, we always REPLACE.  It's simpler.
    // So, consider doing the above new idea later....

    // append this null sequence to the current frame file, and then refreshFrames to load it
    //    #REC=<next sequence number>#
    //    #AUTHOR=<authorID>#
    //    @
//    QString who     = QString(currentFrameTextName).replace(QRegularExpression("\\..*"), "");
//    QString level   = QString(currentFrameTextName).replace(QRegularExpression("^.*\\."), ""); // TODO: filename is <name>.<level> right now


////    QString pathToAppendFile = (musicRootPath + "/sd/%1/SStoSS/%2.choreodb_to_csds.in").arg(who).arg(level);
//    QString pathToAppendFile = (musicRootPath + "/sd/%1.txt").arg(currentFrameTextName);

////    qDebug() << "currentFrameTextName/who/level/path: " << currentFrameTextName << who << level << pathToAppendFile;

//    // TODO: When making a new Frame file, it must start out with a single "@\n" line.

//    QFile file(pathToAppendFile);
//    if (file.open(QIODevice::Append))
//    {
//        QTextStream stream(&file);
//        stream << "#REC=" << 100000 * userID + nextSequenceID << "#\n";  // nextSequenceID is 5 decimal digits
//        nextSequenceID++;
//        writeMetadata(userID, nextSequenceID, authorID);   // update the metadata file real quick with the new nextSequenceID
//        stream << "#AUTHOR=" << authorID << "#\n";
//        stream << "@\n";
//        file.close();

//        int centralNum = frameVisible.indexOf("central");
//        frameMaxSeq[centralNum] += 1;                       // add a NEW sequence to the one currently loaded into the Current Sequence window
//        frameCurSeq[centralNum] = frameMaxSeq[centralNum];  // look at the new one

//        SDSetCurrentSeqs(7); // persist the new Current Sequence numbers

//        refreshSDframes();
//    } else {
////        qDebug() << "ERROR: could not append to " << pathToAppendFile;
//    }

        int centralNum = frameVisible.indexOf("central");
        frameMaxSeq[centralNum] += 1;                       // add a NEW sequence to the one currently loaded into the Current Sequence window
        frameCurSeq[centralNum] = frameMaxSeq[centralNum];  // look at the new one
        // NOTE: DO NOT CALL SDSETCURRENSEQS(7) HERE.  WE DO NOT WANT TO PERSIST UNTIL SAVE TIME.
        // TODO: DISABLE ALL LEFT/RIGHT ARROW COMBINATIONS WHEN EDITING A SEQUENCE.  REENABLE WHEN SAVE OR ABORT.

        newSequenceInProgress = true;

        refreshSDframes();

}

QString MainWindow::translateCall(QString call) {
    // TODO: lots here
    return(call);
}

bool MainWindow::checkSDforErrors() {
    bool success = true;

    bool suppressBoilerplate = true;

    for (int i = 0; i < ui->listWidgetSDOutput->count(); i++) {
        QString line = ui->listWidgetSDOutput->item(i)->text();

        if (!suppressBoilerplate) {
//            qDebug() << "SD_OUTPUT: " << i << line;
        }

        if (line.contains("uiSquareDesk")) { // end of boilerplate
            suppressBoilerplate = false;
        }
    }

    return(success);
}

void MainWindow::debugCSDSfile(QString level) {
    Q_UNUSED(level)
//    for (int i = 2; i < 3; i++) {
//        qDebug() << "***** LOADING FRAME FOR TEST: " << i;
//        loadFrame(i, "local.plus", 0, NULL); // NULL means "use the Central widget, which is a table"
//        qDebug() << "sleeping 500ms...";
//        QThread::msleep(500);
//        qDebug() << "awake!";
//        checkSDforErrors();
//    }
//    qDebug() << "***** END TEST";

}

QString MainWindow::makeLevelString(const char *levelCalls[]) {
    static QRegularExpression regex_numberCommaName(QRegularExpression("^((\\s*\\d+)(\\.\\w+)?)\\,?\\s+(.*)$"));
    QString allCallsString = ";";

    // iterate over all the plus calls (NOTE: This list is NOT the one in the file, it's the internal one for now.)
    for (int i = 0; levelCalls[i]; ++i)
    {
        QString line = QString(levelCalls[i]); // the line that would be in the file
        QString number;
        QString name;
//        qDebug() << "theLine: " << line;

        QRegularExpressionMatch match = regex_numberCommaName.match(line);
        if (match.hasMatch())
        {
            QString prefix("");
            if (match.captured(2).length() < 2)
            {
                prefix = " ";
            }
            number = prefix + match.captured(1);
            name = match.captured(4);

            // remove parenthesized stuff and trim spaces
            name = name.replace(QRegularExpression("\\(.*\\)", QRegularExpression::InvertedGreedinessOption), "").trimmed().toLower();

//            qDebug() << number << name;

            allCallsString += (name + ";");
        }
    }

    return(allCallsString);
}

QString MainWindow::translateCallToLevel(QString thePrettifiedCall) {
    QString theCall = thePrettifiedCall.toLower();

    QString allPlusCallsString = makeLevelString(danceprogram_plus);    // make big long string
    QString allA1CallsString   = makeLevelString(danceprogram_a1);      // make big long string
    QString allA2CallsString   = makeLevelString(danceprogram_a2);      // make big long string

    QString allC1CallsString = ";ah so;alter the wave;chase your neighbor;checkover;cross and turn;cross by;";
//    allC1CallsString += "";

    // TODO: not handled yet:
    // circle by (because that requires numbers, i.e. QRegularExpression

    // ***** TODO: only makeLevelString once at startup for each level
    // ***** TODO: make a C1 level string, AND stick it into a Dance Programs file

//    qDebug() << "CHECKING: " << theCall;
//    qDebug() << "AGAINST: " << allPlusCallsString;
//    qDebug() << "AGAINST: " << allA1CallsString;
//    qDebug() << "AGAINST: " << allA2CallsString;

    // must go from highest level to lowest!
    if (theCall.contains("block") ||
            theCall.contains("butterfly") ||
            theCall.contains("cast back") ||
            theCall.contains("concentric") ||
            (theCall.contains("counter rotate") && !theCall.contains("split counter rotate")) ||
            theCall.contains("cross chain") ||
            theCall.contains("cross extend") ||
            theCall.contains("cross roll") ||
            theCall.contains("cross your neighbor") ||
            theCall.contains("pass and roll your cross neighbor") ||
            (theCall.contains("chain thru") && !theCall.contains("spin chain")) ||
            theCall.contains("dixie diamond") ||
            theCall.contains("dixie sashay") ||
            theCall.contains("flip") ||
            theCall.contains("follow thru") ||
            theCall.contains("galaxy") ||
            theCall.contains("interlocked") ||
            theCall.contains("jaywalk") ||
            theCall.contains("linear action") ||
            theCall.contains("magic") ||
            theCall.contains("O ") ||
            theCall.contains("the axle") ||
            theCall.contains("percolate") ||
            theCall.contains("phantom") ||
            theCall.contains("press ahead") ||
            theCall.contains("2/3") ||
            theCall.contains("box recycle") ||
            theCall.contains("regroup") ||
            theCall.contains("relay the shadow") ||
            theCall.contains("relay the top") ||
            theCall.contains("reverse explode") ||
            theCall.contains("rotary spin") ||
            theCall.contains("rotate") ||
            theCall.contains("scatter") ||
            theCall.contains("little") ||
            theCall.contains("plenty") ||
            theCall.contains("ramble") ||
            theCall.contains("shakedown") ||
            theCall.contains("siamese") ||
            (theCall.contains("the windmill") && !theCall.contains("spin the windmill")) ||
            theCall.contains("square chain the top") ||
            theCall.contains("split dixie style to a wave") ||
            theCall.contains("square the bases") ||
            theCall.contains("squeeze") ||
            theCall.contains("step and flip") ||
            theCall.contains("step and fold") ||
            theCall.contains("stretch") ||
            theCall.contains("substitute") ||
            theCall.contains("swing and circle") ||
            theCall.contains("fractions") ||
            theCall.contains("switch the line") ||
            theCall.contains("t-bone") ||
            theCall.contains("back to a wave") ||
            theCall.contains("tally ho") ||
            theCall.contains("tandem") ||
            theCall.contains("3 by 2") ||
            theCall.contains("Track 0") ||
            theCall.contains("Track 1") ||
            theCall.contains("Track 3") ||
            theCall.contains("Track 4") ||
            theCall.contains("triangle") ||
            theCall.contains("triple box") ||
            theCall.contains("triple column") ||
            theCall.contains("triple wave") ||
            theCall.contains("twist") ||
            theCall.contains("vertical tag") ||
            (theCall.contains("to a wave") && !theCall.contains("single circle") && !theCall.contains("step to a")) ||
            (theCall.contains("weave") && !theCall.contains("the ring")) ||
            (theCall.contains("wheel and") && !theCall.contains("deal")) ||
            theCall.contains("wheel fan thru") ||
            theCall.contains("wheel thru") ||
            theCall.contains("with the flow") ||
            theCall.contains("zing") ||
            allC1CallsString.contains(";" + theCall + ";")
            ) {
        return("C1");
    } else if (theCall.contains("windmill") ||
            allA2CallsString.contains(";" + theCall + ";") ||
            theCall.contains("motivate")                // takes care of "motivate, turn the star 1/4"
               ) {
        return("A2");
    } else if (theCall.contains("any hand") ||
               theCall.contains("clover and") ||
               theCall.contains("cross clover") ||
               theCall.contains("and cross") ||
               theCall.contains("1/4 in") ||
               theCall.contains("1/4 out") ||
               theCall.contains("chain reaction") ||    // takes care of "chain reaction, turn the star 1/2"
               allA1CallsString.contains(";" + theCall + ";")) {
        return("A1");
    } else if (theCall.contains("and roll") ||
               theCall.contains("and spread") ||
               allPlusCallsString.contains(";" + theCall + ";")) {
        return("Plus");
    }
    return("Mainstream"); // else it's probably Basic or MS
}

// =================================
// ----------------------------------------------------------------------
void MainWindow::on_actionShow_group_station_toggled(bool showGroupStation)
{
//    Q_UNUSED(showGroupStation)
//    qDebug() << "TOGGLED: " << showGroupStation;
    ui->actionShow_group_station->setChecked(showGroupStation); // when called from constructor
    prefsManager.Setenablegroupstation(showGroupStation);  // persistent menu item
    on_sd_update_status_bar(sdLastFormationName);  // refresh SD graphical display
}

void MainWindow::on_actionShow_order_sequence_toggled(bool showOrderSequence)
{
//    Q_UNUSED(showOrderSequence)
//    qDebug() << "TOGGLED ORDER SEQUENCE: " << showOrderSequence;
    ui->actionShow_order_sequence->setChecked(showOrderSequence); // when called from constructor
    prefsManager.Setenableordersequence(showOrderSequence);  // persistent menu item
    on_sd_update_status_bar(sdLastFormationName);  // refresh SD graphical display
}

// ==================================
// fetch the GLUID etc from .squaredesk/metadata.csv
void MainWindow::getMetadata() {
    QFile metadata(musicRootPath + "/.squaredesk/metadata.csv");

    QFileInfo fi(metadata);

    if (fi.exists()) {
        QString line;
        if (!metadata.open(QFile::ReadOnly)) {
            qDebug() << "Could not open: " << metadata.fileName() << " for reading metadata";
            return;
        }

//        qDebug() << "OPENED FOR READING: " << metadata.fileName();
        QTextStream in(&metadata);

        // read first line
        line = in.readLine();
        if (line != "key,value") {
            qDebug() << "Unexpected header line: " << line;
            metadata.close();
            return;
        }

        userID = -1;
        nextSequenceID = -1;
        authorID = "";

        // read key,value pairs
        while (!in.atEnd()) {
            line = in.readLine();
            QStringList L = line.split(',');
            QString key = L[0];
            QString value = L[1];

            if (key == "userID") {
                userID = value.toInt();
//                qDebug() << "***** USERID: " << userID;
            } else if (key == "nextSequenceID") {
                nextSequenceID = value.toInt();
//                qDebug() << "***** NEXTSEQUENCEID: " << nextSequenceID;
            } else if (key == "authorID") {
                authorID = value;
//                qDebug() << "***** AUTHODID: " << authorID;
            } else {
                qDebug() << "SKIPPING UNKNOWN KEY: " << key;
            }
        }

        if (userID == -1 || nextSequenceID == -1 || authorID == "") {
            qDebug() << "BAD METADATA: " << userID << nextSequenceID << authorID;
        } else {
//            qDebug() << "METADATA LOOKS OK!";
        }

        metadata.close();
    } else {
        userID = (int)(QRandomGenerator::global()->bounded(1, 21474)); // range: 1 - 21473 inclusive
        nextSequenceID = 1;
        authorID.setNum(userID);  // default Author is a number, unless user changes it manually in hidden file
        writeMetadata(userID, nextSequenceID, authorID);
//        qDebug() << "***** USERID/NEXTSEQUENCEID WRITTEN SUCCESSFULLY AND VALUES SET INTERNALLY: " << userID << nextSequenceID;
    }
}

// update the metadata file real quick with the new nextSequenceID
void MainWindow::writeMetadata(int userID, int nextSequenceID, QString authorID) {
    QFile metadata(musicRootPath + "/.squaredesk/metadata.csv");
    if (!metadata.open(QFile::WriteOnly)) {
        qDebug() << "Could not open: " << metadata.fileName() << " for writing metadata";
        return;
    }

//    qDebug() << "OPENED FOR WRITING: " << metadata.fileName();
    QTextStream out(&metadata);

    out << "key,value\n";

    out << "userID," << userID << "\n";
    out << "nextSequenceID," << nextSequenceID << "\n";
    out << "authorID," << authorID << "\n";

//    qDebug() << "***** USERID/NEXTSEQUENCEID UPDATED " << userID << nextSequenceID;
    metadata.close();
}

// update the local cache with the status that was persisted in this sequencesUsed.csv
void MainWindow::SDReadSequencesUsed() {

    // TODO: read sequencesUsed file for this frame
    //       and update sequenceStatus[] hash table
    QFile file((musicRootPath + "/sd/frames/" + frameName + "/sequenceUsed.csv"));
//    qDebug() << "Reading file: " << file.fileName();

    if (file.open(QIODevice::ReadOnly))
    {
        while(!file.atEnd()) {
            QString line = file.readLine().trimmed();
//            qDebug() << "frameName: " << frameName << ", sequenceUsed line: " << line;
            QStringList fields = line.split(','); // should be exactly 5 fields
            if (!(fields[0] == "datetime")) {
                // if not the header line: datetime,userID,sequenceID,status,reason
//                QString datetime   = fields[0];
//                QString userID     = fields[1];
                QString sequenceID = fields[2];
                QString status     = fields[3];
//                QString reason     = fields[4];

                int sequenceIDi = sequenceID.toInt();
                if (status == "GOOD") {
                    sequenceStatus[sequenceIDi] = 1;
                } else if (status == "BAD") {
                    sequenceStatus[sequenceIDi] = 2;
                }

//                qDebug() << "decoded: " << sequenceIDi << status << sequenceStatus[sequenceIDi];
            }
        }
        file.close();
//        qDebug() << "sequenceStatus: " << sequenceStatus;
    } else {
        qDebug() << "File " << file.fileName() << " could not be opened, so creating one.";
        QFileInfo info(file.fileName());

        if (!info.exists()) {
            if (file.open(QFile::WriteOnly | QFile::Append)) {
                // file is created, and CSV header is written
                QTextStream out(&file);
                out << "datetime,userID,sequenceID,status,reason\n";
                file.close();
            } else {
                qDebug() << "ERROR: could not make a new sequenceUsed file: " << file.fileName();
            }
        }
    }
}


