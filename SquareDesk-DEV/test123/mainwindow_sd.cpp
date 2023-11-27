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
#include "addcommentdialog.h"
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
    // QGraphicsPolygonItem *hex = new QGraphicsPolygonItem();
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
    // hex->setPolygon(hexagon);
    // hex->setPen(pen);
    QGraphicsItem *hexItem = dynamic_cast<QGraphicsItem*>(sdscene.addPolygon(hexagon, pen, coupleColorBrushes[number]));

    boyItem->setVisible(boy);
    girlItem->setVisible(!boy);
    hexItem->setVisible(false);  // always invisible to start

    QGraphicsRectItem *directionRectItem = sdscene.addRect(directionRect, pen, coupleColorBrushes[number]);

    QGraphicsTextItem *label = sdscene.addText(QString("  %1  ").arg(number + 1), // exactly 3 characters wide when numbers used (sets the hjust)
                                               dancerLabelFont);
    label->setDefaultTextColor(Qt::black);

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
    const QString gridStyle = "QTableWidget { gridline-color: #000000; color: #000000; font-size: 18pt;}";
//    const QString gridStyle = "QTableWidget { font-family: \"Courier New\"; gridline-color: #000000; }";  // how to change font
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
    static QRegularExpression re("resolve: \\d+ out of \\d+", QRegularExpression::CaseInsensitiveOption);
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

    graphicsTextItemSDStatusBarText_fixed->setDefaultTextColor(Qt::black);
    graphicsTextItemSDStatusBarText_animated->setDefaultTextColor(Qt::black);
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
//    qDebug() << "on_listWidgetSDAdditionalOptions_itemDoubleClicked setCurrentIndex to 0";
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

static QRegularExpression who("(Center Box |Center Lady |That Boy |That Girl |Those Girls |Those Boys |Points |On Each Side |Center Line Of 4 |New Outsides |Each Wave |Outside Boys |Lines Of 3 Boys |Side Boys |Side Ladies |Out-Facing Girls |Line Of 8 |Line Of 3 |Lead Boy |Lead Girl |Lead Boys |Just The Boys |Heads In Your Box |Sides In Your Box |All Four Ladies |Four Ladies |Four Girls |Four Boys |Center Girls |Center Four |All 8 |Both |Side Position |Same Girl |Couples 1 and 2 |Head Men and Corner |Outer 4 |Outer 6 |Couples |New Couples 1 and 3 |New Couples 1 and 4 |Couples 1 & 2 |Ladies |Head Man |Head Men |Lead Couple |Men |Those Who Can |New Ends |End Ladies |Other Ladies |Lines Of 3 |Waves Of 3 |All 4 Girls |All 4 Ladies |Each Side Boys |Those Boys |Each Side Centers |Center 4 |Center 6 |Center Boys |Center Couples |Center Diamond |Center Boy |Center Girl |Center Wave |Center Line |All Eight |Other Boy |Other Girl |Centers Only |Ends Only |Outside Boy |All The Boys |All The Girls |Centers |Ends |All 8 |Boys Only |Girls Only |Boys |Girls |Heads |Sides |All |New Centers |Wave Of 6 |Others |Outsides |Leaders |Side Boy |4 Girls |4 Ladies |Very Center Boy |Very Center Boys |Very End Boys |Very Centers |Very Center Girls |Those Facing |Those Boys |Head Position |Head Boys |Head Ladies |End Boy |End Girl |Everybody |Each Side |4 Boys |4 Girls |Same 4 )",
                              QRegularExpression::CaseInsensitiveOption);

QString MainWindow::upperCaseWHO(QString call) {
    QString call1 = call;

    // from Mike's process_V1.5.R
    // ALL CAPS the who of each line (the following explicitly defined terms, at the beginning of a line)
    // This matches up with what Mike is doing for Ceder cards.
    QRegularExpressionMatch match = who.match(call1);
    if (match.hasMatch()) {
        QString matched = match.captured(0);
        call1.replace(match.capturedStart(), match.capturedLength(), matched.toUpper());  // replace the match with the toUpper of the match
    }
    return (call1);
}

QString MainWindow::prettify(QString call) {
    return(SDOutputToUserOutput(call));
}

void MainWindow::on_sd_add_new_line(QString str, int drawing_picture)
{
//    qDebug() << "on_sd_add_new_line str: " << str << sdLastLine << sdLastNonEmptyLine;
    if (sdOutputtingAvailableCalls)
    {
        SDAvailableCall available_call;
        available_call.call_name = str.simplified();
        available_call.dance_program = sdthread->find_dance_program(str);

        // we can't show "exit from the program" to the user as an allowable thing to double-click on!
        if (available_call.call_name.toLower() != "exit from the program") {
            sdAvailableCalls.append(available_call);
        }
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

            static QRegularExpression parens("\\((.*)\\)");
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

    if (str.startsWith("Output file is") || str.contains("uiSquareDesk"))
    {
        // UNDO does "refresh", which spits out another of these:
        //    Sd 39.45 : db39.45 : uiSquareDesk-1.0.3
        // which needs to reset the sdLastNonEmptyLine counter.
        // then we will get the whole sequence from 1 to N again

        // OR end of copyright section, so reinit the engine
//        qDebug() << "Output file OR uiSquareDesk";
        sdLastLine = 0;
        sdLastNonEmptyLine = 0;
        sd_redo_stack->initialize();
        ui->label_SD_Resolve->clear(); // get rid of extra "(no matches)" or "left allemande" in resolve area when changing levels
//        if (str.contains("uiSquareDesk"))
//        {
//            ui->tableWidgetCurrentSequence->clear(); // this cannot be here, see #878.  TODO: Figure out why I put this here later...or, just delete it
//        }
        return;
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
        static QRegularExpression regexMove("^\\s*(\\d+)\\:\\s*(.*)$"); // e.g. "2: call"
        QRegularExpressionMatch match = regexMove.match(str);
        if (match.hasMatch())
        {
            QString move = match.captured(2).trimmed();
            sdHasSubsidiaryCallContinuation = false;
            if (move.contains("[") && !move.contains("]")) {
                sdHasSubsidiaryCallContinuation = true;
            }
            sdLastLine = match.captured(1).toInt();
//            qDebug() << "   MATCH: " << sdLastLine << sdLastNonEmptyLine;
            if (sdLastLine > 0 && !move.isEmpty())
            {
                sdLastNonEmptyLine = fmax(sdLastNonEmptyLine, sdLastLine); // if we're here, sdLastLine means it's a non-empty line
//                qDebug() << "HERE 1: " << ui->tableWidgetCurrentSequence->rowCount() << sdLastLine << sdLastNonEmptyLine;
                if (ui->tableWidgetCurrentSequence->rowCount() < sdLastLine)
                {
//                    qDebug() << "     Bumping up rowCount to: " << sdLastLine;
                    ui->tableWidgetCurrentSequence->setRowCount(sdLastLine);
                    sdLastNonEmptyLine = sdLastLine;
                }

//                qDebug() << "ROW COUNT IS: " << ui->tableWidgetCurrentSequence->rowCount();

#ifdef NO_TIMING_INFO
                QString theCall = match.captured(2);
                QString thePrettifiedCall = prettify(theCall);

#ifndef darkgreencomments
                QString lcPrettifiedCall = thePrettifiedCall.toLower();
                bool containsHighlightedCall = false;

                for ( const auto& call : highlightedCalls )
                {
                    if (lcPrettifiedCall.contains(call)) {
                        containsHighlightedCall = true;
                    }
                }

                QTableWidgetItem *moveItem(new QTableWidgetItem(thePrettifiedCall));
                moveItem->setFlags(moveItem->flags() & ~Qt::ItemIsEditable);

                if (containsHighlightedCall) {
                    moveItem->setForeground(QBrush(QColor("red"))); // set highlighted items to RED
                }

#else
                QLabel *moveItem(new QLabel(thePrettifiedCall));
#endif

                QString level = translateCallToLevel(thePrettifiedCall);
//                qDebug() << "level: " << level;

#ifndef darkgreencomments
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

//                ui->tableWidgetCurrentSequence->setRowCount(sdLastLine + sdLastLineOffset);
                ui->tableWidgetCurrentSequence->setItem(sdLastLine - 1, kColCurrentSequenceCall, moveItem);
#else
                if (level == "Mainstream") {
                    moveItem->setStyleSheet("background-color: #E0E0FF;");
                } else if (level == "Plus") {
                    moveItem->setStyleSheet("background-color: #BFFFC0;");
                } else if (level == "A1" || level == "A2") {
                    moveItem->setStyleSheet("background-color: #FFF0C0;");
                } else if (level == "C1") {
                    moveItem->setStyleSheet("background-color: #FEE0E0;");
                } else {
//                    qDebug() << "ERROR: unknown level for setting BG color of SD item: " << level;
                }

                moveItem->setFont(ui->songTable->font());
                ui->tableWidgetCurrentSequence->setCellWidget(sdLastLine - 1, kColCurrentSequenceCall, moveItem);
#endif

//                qDebug() << "on_sd_add_new_line: adding " << thePrettifiedCall;

                // This is a terrible kludge, but I don't know a better way to do this.
                // The tableWidgetCurrentSequence is updated asynchronously, when sd gets around to sending output
                //   back to us.  There is NO indication that SD is done processing, near as I can tell.  So, there
                //   is no way for us to select the first row after SD is done.  If we try to do it before, BOOM.
                // This kludge gives SD 100ms to do all its processing, and get at least one valid row and item in there.
                QTimer::singleShot(100, [this]{
                    if (selectFirstItemOnLoad) {
//                        qDebug() << "selectFirstItemOnLoad was TRUE, so setting focus to Current Sequence and selecting first item...";
                        selectFirstItemOnLoad = false;
                        ui->tableWidgetCurrentSequence->setFocus();
                        ui->tableWidgetCurrentSequence->clearSelection();
                        if (ui->tableWidgetCurrentSequence->item(0,0) != nullptr) { // make darn sure that there's actually an item there
                            // Surprisingly, both of the following are necessary to get the first row selected.
                            ui->tableWidgetCurrentSequence->setCurrentItem(ui->tableWidgetCurrentSequence->item(0,0));
                            ui->tableWidgetCurrentSequence->item(0,0)->setSelected(true);

                        }

                        if ((newSequenceInProgress || editSequenceInProgress) && ui->lineEditSDInput->isVisible()) {
                            // if this is a new sequence, and SD is done loading, and we're in edit mode, then set the focus
                            //   to the SD input field.
//                            qDebug() << "overriding that.  Set focus to SD Input field.";
                            ui->lineEditSDInput->setFocus();
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
        } else {
//            qDebug() << "NO MATCH.";
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
//    qDebug() << "on_sd_awaiting_input()" << sdLastLine << rowCount << sdLastNonEmptyLine;
    ui->tableWidgetCurrentSequence->setRowCount(sdLastNonEmptyLine);
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

    if (ui->lineEditSDInput->isVisible()) {
        ui->tabWidgetSDMenuOptions->setCurrentIndex(0);
//        qDebug() << "on_sd_set_matcher_options: setCurrentIndex to 0";
    }

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

#ifndef NO_TIMING_INFO
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
#endif

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

    static QRegularExpression prefixComment("^([\\(\\{].*[\\)\\}]\\s*)(.*)$"); // with capture of the comment (INCLUDES the parens/curly braces)
    QRegularExpressionMatch matchPrefixComment = prefixComment.match(callSearch);

    QString thePrefixComment;

    if (matchPrefixComment.hasMatch()) {
        thePrefixComment   = matchPrefixComment.captured(1);              // do NOT trim whitespace here
        callSearch         = matchPrefixComment.captured(2).simplified(); // and trim whitespace
        originalText       = originalText.replace(thePrefixComment, "");  // remove it from the originalText, for searching
    }

//    qDebug() << "-------------------";
//    qDebug() << "originalText: " << originalText;
//    qDebug() << "callSearch: " << callSearch;
//    qDebug() << "prefix: " << prefix;
//    qDebug() << "thePrefixComment: " << thePrefixComment;

    QString longestMatch(get_longest_match_from_list_widget(ui->listWidgetSDOptions,
                                                            originalText,
                                                            callSearch));
    longestMatch = get_longest_match_from_list_widget(ui->listWidgetSDQuestionMarkComplete,
                                                      originalText,
                                                      originalText,
                                                      longestMatch);
//    qDebug() << "longestMatch: " << longestMatch;
    
    if (longestMatch.length() > callSearch.length())
    {
        if (prefix.length() != 0 && !prefix.endsWith(" "))
        {
            prefix = prefix + " ";
        }
        QString new_line((longestMatch.startsWith(prefix)) ? longestMatch : (prefix + longestMatch));

        new_line = thePrefixComment + new_line;  // stick the prefix comment back onto the start of the line

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
            SDDebug("------------------", true);
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
            SDDebug("------------------", true);
        }
        ui->lineEditSDInput->clear();
    }

    sd_redo_stack->set_doing_user_input();
    submit_lineEditSDInput_contents_to_sd();
    sd_redo_stack->clear_doing_user_input();
}

// ================================================================================================
void MainWindow::submit_lineEditSDInput_contents_to_sd(QString s, int firstCall) // s defaults to "", firstCall default to 0
{
    // SDCOMMENTS: INPUT SIDE OF COMMENT HANDLING ========================================

//    qDebug() << "submit_lineEditSDInput_contents_to_sd: " << s;
    QString cmd;
    if (s.isEmpty()) {
        // if s was empty, it means "get the input from the SDInput field
        cmd = ui->lineEditSDInput->text().simplified(); // both trims whitespace and consolidates whitespace
    } else {
        // if s was NOT empty, it means "send this comment to SD"
        cmd = s;
    }

//    qDebug() << "original cmd: " << cmd << ", firstCall = " << firstCall;

    ui->lineEditSDInput->clear();

//    // SUFFIX COMMENTS --------------------------
//    // Example:
//    // Square Thru ( this is a suffix comment )

//    static QRegularExpression suffixComment("([/a-zA-Z0-9\\s]+)\\s*[\\(\\{](.*)[\\)\\}]$"); // with capture of the comment (not including the parens/curly braces)
//    // must not match single line comments
//    QRegularExpressionMatch matchSC = suffixComment.match(cmd);
//    if (matchSC.hasMatch()) {
//        QString theSC    = matchSC.captured(2).simplified(); // and trim whitespace
//        QString theCall1 = matchSC.captured(1).simplified(); // and trim whitespace
////        qDebug() << "SUFFIX COMMENT DETECTED: " << cmd << theSC << theCall1;

//        // modify cmd to reverse, and use vertical bars to delimit, that is: suffix-as-prefix comment (with vertical bars)
//        cmd = QString("(|") + theSC + "|) " + theCall1; // reverse to "(|suffix comment|) call" == a type of prefix comment

////        qDebug() << "submit_lineEditSDInput_contents_to_sd SUFFIX COMMENT: " << theSC << theCall1 << cmd;
//    }

//    // PREFIX COMMENTS --------------------------
//    // Example:
//    // ( this is a prefix comment ) Square Thru

//    // BUG: cannot put a comment on the very first line, because it's waiting for HEADS/SIDES START
//    static QRegularExpression prefixComment("^[\\(\\{](.*)[\\)\\}][ ]+(.*)$"); // with capture of the comment (not including the parens/curly braces)
//    QRegularExpressionMatch matchPC = prefixComment.match(cmd);
//    if (matchPC.hasMatch()) {
//        QString thePC   = matchPC.captured(1).simplified(); // and trim whitespace
//        QString theCall = matchPC.captured(2).simplified(); // and trim whitespace
////        qDebug() << "submit_lineEditSDInput_contents_to_sd PREFIX COMMENT: " << thePC << theCall;

//        sdthread->do_user_input("insert a comment"); // insert the comment into the SD stream
//        sdthread->do_user_input(thePC);

//        cmd = theCall;  // and then send the call itself
//    }

//    // TODO:
//    // IDEA: Perhaps SINGLE LINE comments should be encoded as ( SINGLE LINE COMMENT ) nullcall


    QStringList todo = UserInputToSDCommand(cmd); // returns either 1 (call) or 2 items (entire user input, call)
//    qDebug() << "TODO: " << todo;

    if (todo.count() == 2) {
//        sdthread->do_user_input("insert a comment"); // insert the comment into the SD stream
//        sdthread->do_user_input(todo[0]); // first item was the entire thing the user typed, including comments

        cmd = todo[1];  // and then send the call itself
    } else {
        cmd = todo[0];  // possibly cleaned up user input line
    }

    cmd = cmd.toLower();  // JUST THE CALL portion is lower-cased

    // Protect "X" from abbreviation expansion, just for the "circle by" case
    cmd = cmd.replace("circle by ", "CBY ").replace("x 1/4", "by 1/4").replace("x 1/2", "by 1/2").replace("x 2/4", "by 2/4").replace("x 3/4", "by 3/4").replace("x [", "by [");

    // ***** EXPAND ABBREVIATIONS *****
    cmd = expandAbbreviations(cmd); // TODO: maybe not when read from file, but yes when user input?

    cmd = cmd.replace("by 1/4", "x 1/4").replace("by 1/2", "x 1/2").replace("by 2/4", "x 2/4").replace("by 3/4", "x 3/4").replace("by [", "x [").replace("CBY ", "circle by ");

    // ==========================================================================================

    if (!cmd.compare("quit", Qt::CaseInsensitive))
    {
        cmd = str_abort_this_sequence;
    }

    // handle C1 calls that SD misspells
    cmd = cmd.replace("jay walk","jaywalk");

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
//    cmd = cmd.replace(QRegularExpression("\\(.*\\)"),"").simplified(); // remove parens, trims whitespace (PARENS HANDLED ELSEWHERE NOW)

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
    cmd = cmd.replace("twosome", "TWOSOME"); // protect "twosome" from two -> 2
    cmd = cmd.replace("one","1").replace("two","2").replace("three","3").replace("four","4");
    cmd = cmd.replace("five","5").replace("six","6").replace("seven","7").replace("eight","8");
    cmd = cmd.replace("EIGHT CHAIN","eight chain"); // sd wants "eight chain 4", not "8 chain 4", so protect it
    cmd = cmd.replace("TWOSOME", "twosome"); // protect "twosome" from two -> 2

    // handle optional words at the beginning

    cmd = cmd.replace("do the centers", "DOTHECENTERS");  // special case "do the centers part of load the boat"
    cmd = cmd.replace(QRegularExpression("^go "),"").replace(QRegularExpression("^do a "),"").replace(QRegularExpression("^do "),"");
    cmd = cmd.replace("DOTHECENTERS", "do the centers");  // special case "do the centers part of load the boat"

    // handle specialized sd spelling of flutter wheel, and specialized wording of reverse flutter wheel
    cmd = cmd.replace("flutterwheel","flutter wheel");
    cmd = cmd.replace("reverse the flutter","reverse flutter wheel");

    // better import from taminations
    cmd = cmd.replace("wheelaround","wheel around");
    cmd = cmd.replace("rollaway","roll away");
    cmd = cmd.replace("crossfold","cross fold");
    cmd = cmd.replace("clover leaf","cloverleaf");
    cmd = cmd.replace("allemande left", "").replace("left allemande", ""); // SD does not understand these

    // better import from Canadians (and Brits (and Aussies (and ...)))!
    cmd = cmd.replace("centres","centers");

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
        // at A-level, "Pass and Roll" must not be turned into "[Pass] and Roll"
        // at C-level, "Cross Chain and Roll" must not be turned into "[Cross Chain] and Roll"
        if (!cmd.contains("pass and roll") && !cmd.contains("cross chain and roll")) {
            static QRegularExpression andRollCall("(.*) and roll.*");
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
        static QRegularExpression explodeAndRollCall("\\[explode and (.*)\\] and roll");
        static QRegularExpression explodeAndNotRollCall("^explode and (.*)");

        cmd.replace(explodeAndRollCall, "[explode and \\1] and roll");
        cmd.replace(explodeAndNotRollCall, "explode and [\\1]");

        // similar for Transfer and Anything, Clover and Anything, and Cross Clover and Anything
        static QRegularExpression transferAndRollCall("\\[transfer and (.*)\\] and roll");
        static QRegularExpression transferAndNotRollCall("^transfer and (.*)");

        cmd.replace(transferAndRollCall, "[transfer and \\1] and roll");
        cmd.replace(transferAndNotRollCall, "transfer and [\\1]");

        static QRegularExpression cloverAndRollCall("\\[clover and (.*)\\] and roll");
        static QRegularExpression cloverAndNotRollCall("^clover and (.*)");

        cmd.replace(cloverAndRollCall, "[clover and \\1] and roll");
        cmd.replace(cloverAndNotRollCall, "clover and [\\1]");

        static QRegularExpression crossCloverAndRollCall("\\[cross clover and (.*)\\] and roll");
        static QRegularExpression crossCloverAndNotRollCall("^cross clover and (.*)");

        cmd.replace(crossCloverAndRollCall, "[cross clover and \\1] and roll");
        cmd.replace(crossCloverAndNotRollCall, "cross clover and [\\1]");
    }

    // handle <ANYTHING> and spread
    if (!cmd.contains("[")) {
        static QRegularExpression andSpreadCall("(.*) and spread");
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
    static QRegularExpression andSweepPart(" and sweep.*");
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

    cmd.replace("1/2 breed thru", "brace thru");
    cmd.replace(" 1 times", ""); // 1 times can always be deleted, as in "Circulate 1 TIMES"

    // SD COMMANDS -------

//    qDebug() << "CMD: " << cmd << firstCall;

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
        if ((
             (firstCall == 0 && ui->tableWidgetCurrentSequence->rowCount() == 0) ||  // SD Input AND this is the first call being entered
             firstCall == 1     // loading a frame, and this is the first call in the frame
             ) &&
                !cmd.contains("1p2p")) {
            // TODO: something like "HEAD BOYS do X" as the first call will probably fail here...
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

        if (todo.count() == 2) {
            // if first call is "HEADS ( foo ) Square Thru"
            // we will do:
            // heads start
            // insert a comment
            // "HEADS ( Foo ) Square Thru"
            // square thru 4
            sdthread->do_user_input("insert a comment"); // insert the comment into the SD stream
            sdthread->do_user_input(todo[0]); // first item was the entire thing the user typed, including comments
        }

        if (cmd.isEmpty()) {
            // nothing to submit, e.g. was a single-line comment like "( NEW SEQUENCE )"
//            qDebug() << "no call here, so returning...";
//            qDebug() << "------------";
            return;
        }

//        qDebug() << "SUBMIT: " << cmd;
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

//        qDebug() << "------------";
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

    // check to see if we are in the middle of writing a comment
    static QRegularExpression unfinishedComment("\\([^\\)]*$"); // open paren followed by a bunch of non-closed-parens up to the end of line
    bool containsUnfinishedComment = currentText.contains(unfinishedComment);
    
    if (!containsUnfinishedComment &&
        currentTextLastChar >= 0 &&
        (currentText[currentTextLastChar] == '!' || currentText[currentTextLastChar] == '?')
        )
    {
        // '!' and '?' cause immediate execution, UNLESS they are in a comment
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
    
//    qDebug() << "on_lineEditSDInput_textChanged: " << currentText;

    static QRegularExpression prefixComment("^([\\(\\{].*[\\)\\}]\\s*)(.*)$"); // with capture of the comment (INCLUDES the parens/curly braces)
    QRegularExpressionMatch matchPrefixComment = prefixComment.match(currentText);

    QString thePrefixComment;

    if (matchPrefixComment.hasMatch()) {
        thePrefixComment   = matchPrefixComment.captured(1);              // do NOT trim whitespace here
        currentText        = matchPrefixComment.captured(2).simplified(); // and trim whitespace
//        qDebug() << "on_lineEditSDInput_textChanged: NEW currentText: " << currentText;
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
            if (item != nullptr) {
                item->setSelected(true);
            }
        }
    }    
}

void MainWindow::undo_last_sd_action()
{
    bool inSDEditMode = ui->lineEditSDInput->isVisible(); // if we're in UNLOCK/EDIT mode

    if (inSDEditMode) {
        sdthread->do_user_input(str_undo_last_call);
        sdthread->do_user_input("refresh");
        sd_redo_stack->did_an_undo();

        ui->lineEditSDInput->setFocus();
    }
}

void MainWindow::redo_last_sd_action()
{
    bool inSDEditMode = ui->lineEditSDInput->isVisible(); // if we're in UNLOCK/EDIT mode

    if (inSDEditMode) {
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

    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    QString theText;
    if (mimeData->hasHtml() && !mimeData->hasText()) {
//        qDebug() << "TRYING TO CONVERT HTML TO TXT" << mimeData;
    // if there is HTML (and NOT Plain Text), try to convert HTML to plain text...
    static QRegularExpression htmlTags("<.*>", QRegularExpression::InvertedGreedinessOption); // <...>
    theText = mimeData->html().replace(htmlTags, "");  // delete HTML tags and let's try that
    } else if (mimeData->hasText()) {
//        qDebug() << "WE HAVE TEXT DATA:" << mimeData;
        theText = mimeData->text();
    } else {
        qDebug() << "ERROR: CANNOT UNDERSTAND CLIPBOARD DATA"; // error? warning?
        return;
    }

    static QRegularExpression comments("\\(.*\\)", QRegularExpression::InvertedGreedinessOption); // (...) anywhere in the line
    static QRegularExpression singleLineComments("^\\(.*\\)$", QRegularExpression::InvertedGreedinessOption); // ^(...)$
    static QRegularExpression EOLs("[\r\n]");    // CR or NL or CR/NL
    static QRegularExpression nonNullStrings("^.+$");    // one or more characters

    QStringList theNewCalls =
            theText
            .replace(",", "\n")                     // commas become NLs
            .split(EOLs, Qt::SkipEmptyParts)
            .replaceInStrings(singleLineComments, "")   // ^(...)$ are reduced to null strings
            .filter(nonNullStrings)                     // null strings (single line comments) removed
            ;

//    qDebug() << "theNewCalls: " << theNewCalls;

    // get all existing calls -----
    QStringList theExistingCalls;
    for (int row = 0; row < ui->tableWidgetCurrentSequence->rowCount(); ++row)
    {
        QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,0);
        theExistingCalls << item->text();
    }

    QStringList allTheCalls = theExistingCalls + theNewCalls;

    // TODO: right now, we can't deal with ANY comments on the first line, so just delete them for now, for safety
    //   and greater chance of success in pasting
    allTheCalls[0] = allTheCalls[0].replace(comments, ""); // just the first line!

//    qDebug() << "allTheCalls:" << allTheCalls;

    // reset the SD engine, and load in the new concatenated list -----
    selectFirstItemOnLoad = false;  // one-shot, when we get the first item actually loaded into tableWidgetCurrentSequence, select the first row
    sdthread->resetAndExecute(allTheCalls);
    ui->lineEditSDInput->setFocus();  // leave the focus in the SD input field
}

void MainWindow::add_comment_to_tableWidgetCurrentSequence() {
//    qDebug() << "add_comment_to_tableWidgetCurrentSequence() called";

    // get all existing calls -----
    QStringList theExistingCalls;
    for (int row = 0; row < ui->tableWidgetCurrentSequence->rowCount(); ++row)
    {
        QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,0);
        theExistingCalls << item->text();
    }

//    qDebug() << "theExistingCalls: " << theExistingCalls;
//    qDebug() << "sdUndoToLine: " << sdUndoToLine; // number of calls to undo to get to this one
    // if we undo sdUndoToLine calls, then the last entry in the tableWidgetCurrentSequence is the one to modify
    int rowCount = ui->tableWidgetCurrentSequence->rowCount();
//    qDebug() << "rowCount: " << rowCount;
    int itemNumberToModify = rowCount - sdUndoToLine;
    QString item = ui->tableWidgetCurrentSequence->item(itemNumberToModify,0)->text();
//    qDebug() << "item number/item: " << itemNumberToModify << item;

    QString modifiedItem;

    // ASK THE USER FOR THE NEW COMMENT
    RecursionGuard dialog_guard(inPreferencesDialog);
    trapKeypresses = false;

    AddCommentDialog *dialog = new AddCommentDialog();
    dialog->populateFields(item);
    int dialogCode = dialog->exec();
    trapKeypresses = true;
//    RecursionGuard keypress_guard(trapKeypresses);
    QString newComment;
    if (dialogCode == QDialog::Accepted)
    {
        newComment = dialog->getComment();
//        qDebug() << "new comment is: " << newComment;
        modifiedItem = item + " ( " + newComment + " )";
    } else {
//        qDebug() << "USER CANCELLED.";
        return; // user clicked CANCEL
    }
    delete dialog;
    dialog = nullptr;

//    qDebug() <<  "newComment: " << newComment;

//    qDebug() << "modifiedItem: " << modifiedItem;
    QStringList modifiedCalls = theExistingCalls;
    modifiedCalls[itemNumberToModify] = modifiedItem;
//    qDebug() << "modified list of calls: " << modifiedCalls;

    // Now just replace the current sequence with this one!
    on_actionSDSquareYourSets_triggered();      // init everything
    ui->tableWidgetCurrentSequence->clear();    // clear the current sequence pane
    selectFirstItemOnLoad = true;              // when the resetAndExecute is done, do NOT select the first item in the sequence
    sdthread->resetAndExecute(modifiedCalls);        // DO IT
//    ui->tableWidgetCurrentSequence->setFocus();
}

void MainWindow::delete_comments_from_tableWidgetCurrentSequence() {
//    qDebug() << "delete_comments_from_tableWidgetCurrentSequence() called";

    // get all existing calls -----
    QStringList theExistingCalls;
    for (int row = 0; row < ui->tableWidgetCurrentSequence->rowCount(); ++row)
    {
        QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,0);
        theExistingCalls << item->text();
    }

//    qDebug() << "theExistingCalls: " << theExistingCalls;
//    qDebug() << "sdUndoToLine: " << sdUndoToLine; // number of calls to undo to get to this one
    // if we undo sdUndoToLine calls, then the last entry in the tableWidgetCurrentSequence is the one to modify
    int rowCount = ui->tableWidgetCurrentSequence->rowCount();
//    qDebug() << "rowCount: " << rowCount;
    int itemNumberToModify = rowCount - sdUndoToLine;
    QString item = ui->tableWidgetCurrentSequence->item(itemNumberToModify,0)->text();
//    qDebug() << "item number/item: " << itemNumberToModify << item;
    static QRegularExpression comments2("\\(.*\\)");
    QString modifiedItem = item.replace(comments2, ""); // delete the comment
//    qDebug() << "modifiedItem: " << modifiedItem;
    QStringList modifiedCalls = theExistingCalls;
    modifiedCalls[itemNumberToModify] = modifiedItem;
//    qDebug() << "modified list of calls: " << modifiedCalls;

    // Now just replace the current sequence with this one!
    on_actionSDSquareYourSets_triggered();      // init everything
    ui->tableWidgetCurrentSequence->clear();    // clear the current sequence pane
    selectFirstItemOnLoad = false;              // when the resetAndExecute is done, do NOT select the first item in the sequence
    sdthread->resetAndExecute(modifiedCalls);        // DO IT

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
            if (item != nullptr && item->isSelected())
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

    QString resolve = ui->label_SD_Resolve->text();
    if (resolve != "") {
        selection += "(" + resolve + ")\n"; // stick the resolve on at the end as a comment
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
            if (item != nullptr && item->isSelected())
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
            selection += "      <li><b>" + item->text().toHtmlEscaped() + "</b>";
            QTableWidgetItem *imageItem = ui->tableWidgetCurrentSequence->item(row,kColCurrentSequenceFormation);
            if ( imageItem != nullptr && imageItem->isSelected() )
            {
                selection += render_image_item_as_html(imageItem, scene, people, graphics_as_text);
            }

            selection +=  + "</li>\n";
        }
    }

    QString resolve = ui->label_SD_Resolve->text();
    if (resolve != "") {
        selection += "      <li><b>(" + resolve.toHtmlEscaped() + ")</b></li>\n";  // stick the resolve on at the end as a comment
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

    QAction actionA("Toggle Highlight", this);
    connect(&actionA, SIGNAL(triggered()), this, SLOT(toggleHighlight()));
    if (!ui->lineEditSDInput->isVisible()) {
        actionA.setEnabled(false); // if not in EDIT mode, Toggle Highlight is greyed out
    }
    contextMenu.addAction(&actionA);

    QAction actionB("Clear All Highlights", this);
    connect(&actionB, SIGNAL(triggered()), this, SLOT(clearHighlights()));
    if (!ui->lineEditSDInput->isVisible()) {
        actionB.setEnabled(false);  // if not in EDIT mode, Toggle Highlight is greyed out
    }
    contextMenu.addAction(&actionB);

    sdUndoToLine = ui->tableWidgetCurrentSequence->rowCount() - ui->tableWidgetCurrentSequence->rowAt(pos.y());

    int rowToToggle = ui->tableWidgetCurrentSequence->rowCount() - sdUndoToLine;
//    qDebug() << "RowToToggle: " << rowToToggle;

    static QRegularExpression regex_comment("\\(.*\\)");
    bool hasComment = false;

    if (rowToToggle != -1) { // if it's -1, we're not on a row that has a call
        QString callToToggle = ui->tableWidgetCurrentSequence->item(rowToToggle,0)->text().simplified().toLower();
        callToToggle = callToToggle.replace(who, "").simplified(); // remove the WHO at the front of the call (and collapse whitespace)
        QString beforeCommentReplacement = callToToggle; // WHO has been removed

        QString callToToggleNoComments = callToToggle.replace(regex_comment, ""); // now COMMENT has been removed
        hasComment = (callToToggle != beforeCommentReplacement); // if they are different, it must have had a comment

        QString camelCall = toCamelCase(callToToggleNoComments);

        QString contextMenuItemText("Toggle Highlighting of '");  // something like "Toggle Highlighting of 'Square Thru'"
        contextMenuItemText += camelCall + "'";

//        qDebug() << "contextMenuItemText: " << contextMenuItemText;
        actionA.setText(contextMenuItemText);  // override the default generic text with specific call to highlight
    }

    contextMenu.addSeparator(); // ---------------
    bool inSDEditMode = ui->lineEditSDInput->isVisible(); // if we're in UNLOCK/EDIT mode, can do Add/Delete Comments

    QAction action100("Add Comment", this);
    connect(&action100, SIGNAL(triggered()), this, SLOT(add_comment_to_tableWidgetCurrentSequence()));
    action100.setEnabled((rowToToggle != -1) && !hasComment && inSDEditMode);  // if has a comment, can't add another one
    contextMenu.addAction(&action100);

    QAction action101("Delete Comment", this);
    connect(&action101, SIGNAL(triggered()), this, SLOT(delete_comments_from_tableWidgetCurrentSequence()));
    action101.setEnabled((rowToToggle != -1) && hasComment && inSDEditMode);  // if has no comments, can't delete comments
    contextMenu.addAction(&action101);

    contextMenu.addSeparator(); // ---------------

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
    action1a.setEnabled(inSDEditMode);  // if has not editing, can't PASTE SEQUENCE
    contextMenu.addAction(&action1a);

    contextMenu.addSeparator(); // ---------------

    QAction action2("Undo", this);
    connect(&action2, SIGNAL(triggered()), this, SLOT(undo_last_sd_action()));
    action2.setShortcut(QKeySequence::Undo);
    action2.setShortcutVisibleInContextMenu(true);
    action2.setEnabled(inSDEditMode);  // if has not editing, can't UNDO
    contextMenu.addAction(&action2);

    QAction action25("Redo", this);
    connect(&action25, SIGNAL(triggered()), this, SLOT(redo_last_sd_action()));
    action25.setShortcut(QKeySequence::Redo);
    action25.setShortcutVisibleInContextMenu(true);
    action25.setEnabled(sd_redo_stack->can_redo() & inSDEditMode); // if not editing, can't REDO
    contextMenu.addAction(&action25);
    
    QAction action3("Select All", this);
    connect(&action3, SIGNAL(triggered()), this, SLOT(select_all_sd_current_sequence()));
    action3.setShortcut(QKeySequence::SelectAll);
    action3.setShortcutVisibleInContextMenu(true);
    contextMenu.addAction(&action3);

    contextMenu.addSeparator(); // ---------------

    QAction action4("Go Back To Here", this);
    connect(&action4, SIGNAL(triggered()), this, SLOT(undo_sd_to_row()));
    action4.setEnabled(inSDEditMode);  // if has not editing, can't UNDO BACK TO HERE
    contextMenu.addAction(&action4);

    QAction action4a("Square Your Sets", this);
    connect(&action4a, SIGNAL(triggered()), this, SLOT(on_actionSDSquareYourSets_triggered()));
    action4a.setEnabled(inSDEditMode);  // if has not editing, can't SQUARE YOUR SETS
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

            // TODO: I think the toLower is handled elsewhere and is not needed here FIX FIX FIX?
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

//    qDebug() << "refreshSDframes frameName: " << frameName;
//    qDebug() << "refreshSDframes frameVisible: " << frameVisible;
//    qDebug() << "refreshSDframes frameCurSeq: " << frameCurSeq;
//    qDebug() << "refreshSDframes frameMaxSeq: " << frameMaxSeq;

    int whichSidebar = 0;

    QString FColor = "#0433ff"; // BLUE for non-dark mode

#ifdef DARKMODE
    FColor = "#6493ff";
#endif

//    QString frameTitleString("<html><head/><body><p><span style=\"font-weight:700; color:#0433ff;\">F%1</span><span style=\"font-weight:700;\"> %2%5 [%06%3/%4]</span></p></body></html>"); // %06 is intentional, do not use just %6
    QString frameTitleString("<html><head/><body><p><span style=\"font-weight:700; color:%7;\">F%1</span><span style=\"font-weight:700;\"> %2%5 [%06%3/%4]</span></p></body></html>"); // %06 is intentional, do not use just %6
    QString editingInProgressIndicator = (newSequenceInProgress || editSequenceInProgress ? "*" : "");

    for (int i = 0; i < frameFiles.length(); i++) {
//        qDebug() << "frameFile: " << frameFiles[i] << ", frameVisible: " << frameVisible[i];
        if (frameVisible[i] == "sidebar") {
            whichSidebar += 1;
            switch (whichSidebar) {
                case 1:
                    ui->labelEasy->setText(frameTitleString.arg(i+1).arg(frameFiles[i]).arg(frameCurSeq[i]).arg(frameMaxSeq[i]).arg("").arg("").arg(FColor));
                    loadFrame(i, frameFiles[i], fmin(frameCurSeq[i], frameMaxSeq[i]), ui->listEasy);
                    break;
                case 2:
                    ui->labelMedium->setText(frameTitleString.arg(i+1).arg(frameFiles[i]).arg(frameCurSeq[i]).arg(frameMaxSeq[i]).arg("").arg("").arg(FColor));
                    loadFrame(i, frameFiles[i], fmin(frameCurSeq[i], frameMaxSeq[i]), ui->listMedium);
                    break;
                case 3:
                    ui->labelHard->setText(frameTitleString.arg(i+1).arg(frameFiles[i]).arg(frameCurSeq[i]).arg(frameMaxSeq[i]).arg("").arg("").arg(FColor));
                    loadFrame(i, frameFiles[i], fmin(frameCurSeq[i], frameMaxSeq[i]), ui->listHard);
                    break;
                default: break; // by design, only the first 3 sidebar frames found are loaded (FIX)
            }

        } else if (frameVisible[i] == "central") {
//            qDebug() << "Loading CENTRAL frame...";
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
                QString html1 = frameTitleString.arg(i+1).arg(frameFiles[i]).arg(frameCurSeq[i]).arg(frameMaxSeq[i]).arg(editingInProgressIndicator).arg(statusString).arg(FColor);
            currentFrameNumber = i;
            currentFrameTextName = frameFiles[i]; // save just the name of the frame
            currentFrameHTMLName = html1;         // save fancy string

//            if ( ui->pushButtonSDMove->menu() != nullptr ) {
//                ui->pushButtonSDMove->menu()->actions()[0]->setText(QString("Save Current Sequence to ") + currentFrameTextName);       // first one in the list is Save to Current (item 0)
//                ui->pushButtonSDMove->menu()->actions()[1]->setText(QString("Delete Current Sequence from ") + currentFrameTextName);   // second one in the list is Delete from Current (item 1)
//            }

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
//    qDebug() << "----- loadFrame: " << i << filename << seqNum;

    QStringList callList;

    if (list != nullptr) {
        list->clear();  // clear out the current contents
    }

    // NOTE: THIS IS WHERE THE FILENAME STRUCTURE IS DEFINED:
//    QFile theFile((musicRootPath + "/sd/frames/" + frameName + "/" + filename + ".txt"));
    QFile theFile;
    if (frameVisible[i] == "central") {
        theFile.setFileName(musicRootPath + "/sd/dances/" + frameName + "/F" + QString::number(i+1) + "-" + filename + ".txt");
    } else {
        theFile.setFileName(musicRootPath + "/sd/dances/" + frameName + "/f" + QString::number(i+1) + "-" + filename + ".txt");
    }

//    qDebug() << "loadFrame: " << theFile.fileName();

    if(!theFile.open(QIODevice::ReadOnly)) {
        QMessageBox::information(0, "ERROR", QString("loadFrame: the file\n\"") + theFile.fileName() + "\" could not be opened."); // if file does not exist...
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

//            qDebug() << "LINE: " << line;

            if (line.startsWith("#REC=")) {
                seq++;
                wantThisSequence = (seq == seqNum); // want it, iff the seqNum matches the one we're looking at

                if (frameVisible[i] == "central") {
                    // if this is the central frame, then remember the REC
                    line = line.replace("#REC=", "").replace("#", "");  // #REC=<record number># --> <record number>
                    currentSequenceRecordNumber = line ;
//                    qDebug() << "currentSequenceRecordNumber: " << currentSequenceRecordNumber;
                    highlightedCalls.clear();  // if this is the central, then clear (it will be filled in below, if there is a #HIGHLIGHT= line)
                }
                continue;
            } else if (line.startsWith("#AUTHOR=")) {
                if (frameVisible[i] == "central") {
                    // if this is the central frame, then remember the AUTHOR
                    line = line.replace("#AUTHOR=", "").replace("#", "");  // #AUTHOR=<string># --> <string>
                    currentSequenceAuthor = line;
                }
                continue;
            } else if (line.startsWith("#TITLE=")) {
                if (frameVisible[i] == "central") {
                    // if this is the central frame, then remember the TITLE
                    line = line.replace("#TITLE=", "").replace("#", "");  // #TITLE=<string># --> <string>
                    currentSequenceTitle = line;
                }
                continue;
            } else if (line.startsWith("#HIGHLIGHT=")) {
                if (frameVisible[i] == "central") {
                    // if this is the central frame, then remember the HIGHLIGHT
                    line = line.replace("#HIGHLIGHT=", "").replace("#", "").simplified();  // #HIGHLIGHT=Zoom,square thru#
                    if (line == "") {
                        highlightedCalls.clear(); // if we don't clear, the resulting null string will match all calls
                    } else {
                        highlightedCalls = line.toLower().split(','); // highlighted calls list is always no whitespace and all lower case
                    }
                }
                continue;
            } else if (line.startsWith("@")) {
                if (wantThisSequence) {
//                    qDebug() << "ACTUAL currentSequenceRecordNumber: " << currentSequenceRecordNumber;
                    currentSequenceRecordNumberi = currentSequenceRecordNumber.toInt();  // convert to int and squirrel it away for use by sequenceStatus

                    break;  // break out of the loop, if we just read all the lines for seqNum
                } else {
                    // @ ending a sequence that we DID NOT want
                    currentSequenceAuthor = "";
                    currentSequenceTitle = "";
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
                qDebug() << "TODO: Sequence not found for Central widget, DO SOMETHING HERE";
            }
        }

        if (frameVisible[i] == "central") {
            // if this loadFrame is for the current sequence pane, then:
            // clear the current sequence pane; if NEW, then nothing will be added, else EXISTING then add via SD
//            qDebug() << "Clearing CENTRAL sequence pane.";
            ui->tableWidgetCurrentSequence->clear();

            currentSequenceTitle = currentSequenceTitle.replace("%%%", "#"); // # is a comment delimiter, so we have to escape it here
//            qDebug() << "loadFrame TITLE: " << currentSequenceTitle;
            ui->sdCurrentSequenceTitle->setText(currentSequenceTitle);
            ui->sdCurrentSequenceTitle->setVisible(currentSequenceTitle != "" || ui->lineEditSDInput->isVisible());

//            ui->sdCurrentSequenceTitle->setText(QString("<html><head/><body><p><span style=\" font-weight:700; color:#ff0000;\">%1</span></p></body></html>").arg(currentSequenceTitle));

            currentSequenceTitle = ""; // start afresh each time
        }

        if (callList.length() != 0) {
            // we are loading the central widget now
//            qDebug() << "Loading central widget with: " << callList;

            // in the new scheme, we do NOT change the level away from what the user selected.
//            setCurrentSDDanceProgram(dlevel);        // first, we need to set the SD engine to the level for these calls

//            on_actionSDSquareYourSets_triggered();   // second, init the SD engine (NOT NEEDED?)
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
//  NOTE: LEFT/RIGHT are already translated to F11/F12 by the time we get here
bool MainWindow::handleSDFunctionKey(QKeyCombination keyCombo, QString text) {
    Q_UNUSED(text)

    int  key = keyCombo.key();
    int  modifiers = keyCombo.keyboardModifiers();
    bool shiftDown   = modifiers & Qt::ShiftModifier;       // true iff SHIFT was down when key was pressed
    bool commandDown = modifiers & Qt::ControlModifier;     // true iff CMD (MacOS) or CTRL (others) was down when key was pressed

    const int kUNRATED = 0;
    const int kGOOD = 1;
    const int kBAD = 2;

//    qDebug() << "handleSDFunctionKey(" << key << ")" << shiftDown;
    if (inPreferencesDialog || !trapKeypresses || (prefDialog != nullptr)) {
//        qDebug() << "Returning because: " << inPreferencesDialog << !trapKeypresses << (prefDialog != nullptr);
        return false;
    }
//    qDebug() << "YES: handleSDFunctionKey(" << key << ")";

    int centralIndex = frameVisible.indexOf("central");
    int newIndex;

    QString pathToSequenceUsedFile = (musicRootPath + "/sd/dances/" + frameName + "/sequenceUsed.csv");
    QFile currentFile(pathToSequenceUsedFile);
    // QFileInfo info(pathToSequenceUsedFile);

    bool inSDEditMode = ui->lineEditSDInput->isVisible(); // if we're in UNLOCK/EDIT mode,
                                                          //   ignore all keys except UP/DOWN (moves Current Sequence selection up/down)

    if (inSDEditMode && key != Qt::Key_Up && key != Qt::Key_Down) {
//        qDebug() << "Ignoring key: " << key << modifiers;
        return(true);
    }

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
//            qDebug() << "CMD-F11";
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
                SDSetCurrentSeqs(3); // persist the new Current Sequence numbers
                refreshSDframes(); // load 3 sidebar and 1 central frame, update labels, update context menu for Save button
            }
            break;

        case Qt::Key_B:
        case Qt::Key_G:
            // Mark current sequence as "used", either "BAD" or "GOOD"
            // B will clear B to nothing, G will clear G to nothing. B will clear G to B, G will clear B to G.

//            if (true ||    // FOR NOW, TURNING OFF AUTO_GOTO_NEXT
//                sequenceStatus.contains(currentSequenceRecordNumberi))
            {

                int currentRating = (sequenceStatus.contains(currentSequenceRecordNumberi) ? sequenceStatus[currentSequenceRecordNumberi] : kUNRATED);
                int keyRating = (key == Qt::Key_B ? kBAD : kGOOD);

                if ( ((currentRating == kBAD) && (keyRating == kBAD)) ||            // rated bad, press B, and B is cleared
                     ((currentRating == kGOOD) && (keyRating == kGOOD)) ) {         // rated good, press G, and G is cleared
                    // clear to nothing, DO NOT GO TO NEXT SEQUENCE
                    sequenceStatus[currentSequenceRecordNumberi] = kUNRATED;
                } else if (keyRating == kBAD) {
                    // B keypress goes to B (G and unrated both go to B)
                    sequenceStatus[currentSequenceRecordNumberi] = kBAD;
                } else if (keyRating == kGOOD) {
                    // G keypress goes to G (B and unrated both go to G)
                    sequenceStatus[currentSequenceRecordNumberi] = kGOOD;
                }

                if (currentFile.open(QFile::WriteOnly | QFile::Append | QFile::ExistingOnly)) {
                    QTextStream out(&currentFile);

                    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
                    if (sequenceStatus[currentSequenceRecordNumberi] == kBAD) {
                        // MARK SEQUENCE AS BAD/USED, THEN MOVE TO NEXT SEQUENCE (equiv to CSDS Ctrl-P)
                        //  Note: marking a sequence as BAD will override the GOOD flag that might have been set earlier.
                        //        The LAST indicator in the sequenceUsed file will be the latest one.
                        out << now << "," << currentSequenceAuthor << "," << currentSequenceRecordNumber << ",BAD,NA\n"; // BAD: reason is always NA right now (TODO: add a reason code here!)
                    } else if (sequenceStatus[currentSequenceRecordNumberi] == kGOOD) {
                        // MARK SEQUENCE AS GOOD/USED, THEN MOVE TO NEXT SEQUENCE  (equiv to CSDS U)
                        //  Note: marking a sequence as GOOD will override the BAD flag that might have been set earlier.
                        //        The LAST indicator in the sequenceUsed file will be the latest one.
                        out << now << "," << currentSequenceAuthor << "," << currentSequenceRecordNumber << ",GOOD,NA\n"; // GOOD: reason is always NA
                    } else {
                        // MARK SEQUENCE AS UNRATED
                        out << now << "," << currentSequenceAuthor << "," << currentSequenceRecordNumber << ",UNRATED,NA\n"; // UNRATED: reason is always NA
                    }

                    currentFile.close();
                }

                refreshSDframes(); // need to update the title, GOOD/BAD might change or go away

//                break;  // NOTE: break out of the switch, DO NOT MOVE TO NEXT SEQUENCE via fallthrough!
                // Let's try the old way again: do NOT break out of the switch, and DO MOVE TO THE NEXT SEQUENCE via fallthrough

            }

            [[fallthrough]];

        case Qt::Key_F12:
            // increment sequence number in current frame
            // only do something if we are NOT looking at the last sequence in the current frame
//            qDebug() << "F12";
            if (frameCurSeq[centralIndex] < frameMaxSeq[centralIndex]) {
                frameCurSeq[centralIndex] = (int)(fmin(frameMaxSeq[centralIndex], frameCurSeq[centralIndex] + 1)); // increase, but not above how many we have
                SDSetCurrentSeqs(4); // persist the new Current Sequence numbers
                refreshSDframes(); // load 3 sidebar and 1 central frame, update labels, update context menu for Save button
            }
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
    ui->pushButtonSDNew->setVisible(false);
    ui->pushButtonSDUnlock->setVisible(false);
    ui->pushButtonSDSave->setVisible(true);
    ui->actionSave_Sequence->setEnabled(true);
    ui->pushButtonSDMove->setVisible(false); // arrange menu
    ui->pushButtonSDDelete->setVisible(true);
    ui->pushButtonSDRevert->setVisible(true);

    ui->lineEditSDInput->setVisible(true);
    ui->lineEditSDInput->setFocus();

    ui->sdCurrentSequenceTitle->setVisible(true);
    ui->sdCurrentSequenceTitle->setFrame(true);
    ui->sdCurrentSequenceTitle->setReadOnly(false);
    ui->sdCurrentSequenceTitle->setPlaceholderText("Sequence Title");
    ui->sdCurrentSequenceTitle->setFocusPolicy(Qt::ClickFocus); // NOT TabFocus

    QPalette palette = ui->sdCurrentSequenceTitle->palette();
    palette.setColor(QPalette::Base, QColor(255,255,255)); // back to original color (Mac)
    ui->sdCurrentSequenceTitle->setPalette(palette);

    ui->warningLabelSD->setVisible(false);
    ui->label_SD_Resolve->setVisible(true);

    ui->tabWidgetSDMenuOptions->setCurrentIndex(0); // When exiting Unlocked/Edit mode, bring up the Options tab
//    qDebug() << "on_pushButtonSDUnlock_clicked setCurrentIndex set to 0";

    editSequenceInProgress = true;
    refreshSDframes();  // clear the * editing indicator

    analogClock->setSDEditMode(true);
}

void MainWindow::SDExitEditMode() {

    ui->pushButtonSDNew->setVisible(true);
    ui->pushButtonSDUnlock->setVisible(true);
    ui->pushButtonSDSave->setVisible(false);
    ui->actionSave_Sequence->setEnabled(false);
    ui->pushButtonSDMove->setVisible(true);  // arrange menu
    ui->pushButtonSDDelete->setVisible(true);
    ui->pushButtonSDRevert->setVisible(false);

    ui->sdCurrentSequenceTitle->setVisible(ui->sdCurrentSequenceTitle->text() != "");
    ui->sdCurrentSequenceTitle->setFrame(false);
    ui->sdCurrentSequenceTitle->setReadOnly(true);
    ui->sdCurrentSequenceTitle->setPlaceholderText("");
    ui->sdCurrentSequenceTitle->setFocusPolicy(Qt::NoFocus);

    QPalette palette = ui->sdCurrentSequenceTitle->palette();
    palette.setColor(QPalette::Base, QColor(227,227,227));   // background to "not there" (Mac)
    ui->sdCurrentSequenceTitle->setPalette(palette);

    ui->lineEditSDInput->setVisible(false);
    ui->tableWidgetCurrentSequence->setFocus();

    ui->warningLabelSD->setVisible(true);
    ui->label_SD_Resolve->setVisible(true); // always visible now

    // we're exiting edit, so highlight the first line
    ui->tableWidgetCurrentSequence->clearSelection();
    if (ui->tableWidgetCurrentSequence->item(0,0) != nullptr) { // make darn sure that there's actually an item there
        // Surprisingly, both of the following are necessary to get the first row selected.
        ui->tableWidgetCurrentSequence->setCurrentItem(ui->tableWidgetCurrentSequence->item(0,0));
        ui->tableWidgetCurrentSequence->item(0,0)->setSelected(true);
    }

    ui->tabWidgetSDMenuOptions->setCurrentIndex(3); // When exiting Unlocked/Edit mode, bring up the Names tab
//    qDebug() << "SDExitEditMode setCurrentIndex to 3";
    newSequenceInProgress = false;
    editSequenceInProgress = false;
    refreshSDframes();  // clear the * editing indicator

    analogClock->setSDEditMode(false);
}

void MainWindow::SDSetCurrentSeqs(int i) {
    Q_UNUSED(i)
//    qDebug() << "SDSet: " << i;
    // persist the frameCurSeq values into /sd/dances/<frameName>/.current.csv (don't worry, it won't take very long!)
    QString pathToCurrentSeqFile = (musicRootPath + "/sd/dances/" + frameName + "/.current.csv");
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

    // NOTE: The search is intentionally for "biggie" rather than "f1-biggie", so that the Fn can be changed at any time,
    //  and the current sequence number will be retained.

    QString pathToCurrentSeqFile = (musicRootPath + "/sd/dances/" + frameName + "/.current.csv");
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


void MainWindow::SDScanFramesForMax() { // i = 0 to 3
//    qDebug() << "SDScanFramesForMax triggered";

    for (int i = 0; i < frameVisible.length(); i++) {
//        qDebug() << "MAGIC [" << i << "]:" << frameCurSeq[i];
        // for each frame (0 - 6)
        QString fileName = frameFiles[i]; // get the filename
        QString pathToScanFile;
//        = (musicRootPath + "/sd/dances/" + frameName + "/%1.txt").arg(fileName); // NOTE: FILENAME STRUCTURE IS HERE, TOO (TODO: FACTOR THIS)

        if (frameVisible[i] == "central") {
            pathToScanFile = musicRootPath + "/sd/dances/" + frameName + "/F" + QString::number(i+1) + "-" + fileName + ".txt";
        } else {
            pathToScanFile = musicRootPath + "/sd/dances/" + frameName + "/f" + QString::number(i+1) + "-" + fileName + ".txt";
        }

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
            qDebug() << "SDScanFramesForMax: File " << pathToScanFile << " could not be opened.";
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

    if (ui->listWidgetSDOptions->item(0) != nullptr) {

        if (ui->listWidgetSDOptions->item(0)->text().contains("search")) {
            // if we are resolving, and user clicked SAVE, they probably want what the resolver
            //   suggested.  So, just accept it and move on.  If we don't get out of resolve mode,
            //   Bad Things will happen.
            sdthread->do_user_input("accept current choice"); // end the search
        }
    }

    // NOTE: i == -1 means the "deleted" frame
    QString whichFile; //  = (i != -1 ? frameFiles[i] : "deleted");
    QString pathToAppendFile; //  = (musicRootPath + "/sd/dances/" + frameName + "/%1.txt").arg(whichFile);

    if (i == -1) {
         whichFile = "deleted";
         pathToAppendFile = musicRootPath + "/sd/dances/" + frameName + "/deleted.txt";
    } else {
        whichFile = frameFiles[i]; // i = 0 to 3
        if (frameVisible[i] == "central") {
            pathToAppendFile = musicRootPath + "/sd/dances/" + frameName + "/F" + QString::number(i+1) + "-" + whichFile + ".txt";
        } else {
            pathToAppendFile = musicRootPath + "/sd/dances/" + frameName + "/f" + QString::number(i+1) + "-" + whichFile + ".txt";
        }
    }

//    qDebug() << "APPEND: currentFrameTextName/who/level/path: " << currentFrameTextName << who << level << pathToAppendFile;

    QFile file(pathToAppendFile);
    if (file.open(QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << "#REC=" << 100000 * userID + nextSequenceID << "#\n";     // Use a new sequence number, whenever edited
        nextSequenceID++;  // increment sequence number every time it's saved (it's a new sequence, right?)
        writeMetadata(userID, nextSequenceID, authorID);   // update the metadata file real quick with the new nextSequenceID
        stream << "#AUTHOR=" << authorID << "#\n";    // Use the new author string here

        if (highlightedCalls.count() > 0) {
            // only write this if there are highlighted calls
            stream << "#HIGHLIGHT=" << highlightedCalls.join(',') << "#\n";    // remember what was highlighted in "central"
        }

        if (ui->sdCurrentSequenceTitle->text() != "") {
            // only write this if there is a title
            stream << "#TITLE=" << ui->sdCurrentSequenceTitle->text().replace("#", "%%%") << "#\n";    // Use the new title string here, and escape "#"
        }

        for (int i = 0; i < ui->tableWidgetCurrentSequence->rowCount(); i++) {
            // SDCOMMENTS: APPEND, WRITE OUT TO FILE
//            qDebug() << "APPENDING: " << ui->tableWidgetCurrentSequence->item(i, 0)->text();
            QString theText = ui->tableWidgetCurrentSequence->item(i, 0)->text();
            stream << theText.replace('{','(').replace('}', ')')  << "\n";
        }

//        qDebug() << "APPEND (RESOLVE TEXT): " << ui->label_SD_Resolve->text();

        stream << "@\n";  // end of sequence
        file.close();

        // Appending increases the number of sequences in a frame, but it does NOT take us to the new frame.  Continue with the CURRENT frame.
        //int centralNum = frameVisible.indexOf("central");
        if (i != -1) { // if "deleted" frame, do not muck with frameMaxSeq, and do not refresh the frames
            frameMaxSeq[i] += 1;    // add a NEW sequence to the receiving frame
                                    // but do not change the CurSeq of that receiving frame
            refreshSDframes();
        }
    } else {
        qDebug() << "ERROR: could not append to " << pathToAppendFile;
    }
    SDExitEditMode();
}

void MainWindow::SDReplaceCurrentSequence() {

    if (ui->listWidgetSDOptions->item(0) != nullptr) {

        if (ui->listWidgetSDOptions->item(0)->text().contains("search")) {
            // if we are resolving, and user clicked SAVE, they probably want what the resolver
            //   suggested.  So, just accept it and move on.  If we don't get out of resolve mode,
            //   Bad Things will happen.
            sdthread->do_user_input("accept current choice"); // end the search
        }
    }

    // NOTE: replacement cannot be done on the "deleted" frame, since it is invisible, therefore no code here for that
    int currentFrame  = frameVisible.indexOf("central"); // 0 to M
    int currentSeqNum = frameCurSeq[currentFrame]; // 1 to N

//    qDebug() << "SDReplaceCurrentSequence REPLACING SEQUENCE #" << currentSeqNum << " FROM " << frameFiles[currentFrame];

//    QString who     = QString(frameFiles[currentFrame]).replace(QRegularExpression("\\..*"), "");
//    QString level   = QString(frameFiles[currentFrame]).replace(QRegularExpression("^.*\\."), ""); // TODO: filename is <name>.<level> right now

    // open the current file for READ ONLY
//    QString pathToOLDFile = (musicRootPath + "/sd/dances/" + frameName + "/%1.txt").arg(frameFiles[currentFrame]);
//    QFile OLDfile(pathToOLDFile);

    // NOTE: i == -1 means the "deleted" frame
    QString pathToOLDFile;

    if (frameVisible[currentFrame] == "central") {
        pathToOLDFile = musicRootPath + "/sd/dances/" + frameName + "/F" + QString::number(currentFrame+1) + "-" + frameFiles[currentFrame] + ".txt";
    } else {
        pathToOLDFile = musicRootPath + "/sd/dances/" + frameName + "/f" + QString::number(currentFrame+1) + "-" + frameFiles[currentFrame] + ".txt";
    }

    QFile OLDfile(pathToOLDFile);

    if (!OLDfile.open(QIODevice::ReadOnly)) {
        qDebug() << "ERROR: could not open " << pathToOLDFile;
        return;
    }

    // in SD directory, open a new file for APPEND called "foo.bar.NEW", which is where we will build a new file to replace the old one
//    QString pathToNEWFile = (musicRootPath + "/sd/%1/SStoSS/%2.choreodb_to_csds.in.NEW").arg(who).arg(level);
//    QString pathToNEWFile = (musicRootPath + "/sd/frames/" + frameName + "/%1.txt.NEW").arg(frameFiles[currentFrame]);

    QString pathToNEWFile = pathToOLDFile + ".NEW";

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

                if (highlightedCalls.count() > 0) {
                    // only write this if there are highlighted calls
                    outFile << "#HIGHLIGHT=" << highlightedCalls.join(',') << "#\n";
                }

                if (ui->sdCurrentSequenceTitle->text() != "") {
                    // only write this if there is a non-blank title
                    outFile << "#TITLE=" << ui->sdCurrentSequenceTitle->text().replace("#", "%%%") << "#\n";    // Use the new title string here and escape "#"
                }

                if (ui->tableWidgetCurrentSequence->rowCount() >= 1) {
                    for (int i = 0; i < ui->tableWidgetCurrentSequence->rowCount(); i++) {
#ifndef darkgreencomments
                        // SDCOMMENTS: REPLACE, CHANGE CURLY BRACES TO PARENS ON WRITE OUT TO FILE
                        QString theText = ui->tableWidgetCurrentSequence->item(i, 0)->text();
                        QString theText2 = theText.replace('{','(').replace('}', ')');
                        outFile << theText2 << "\n"; // COPY IN THE REPLACEMENT
#else
                        QString theText = ((QLabel *)(ui->tableWidgetCurrentSequence->cellWidget(i,0)))->text();
                        QString theText2 = theText.replace('{','(').replace('}', ')');
                        QString theText3 = theText2.replace("<span style=\"color:darkgreen;\">","").replace("</span>","");
                        qDebug() << "writing replacement sequence: " << theText << theText2 << theText3;
                        outFile << theText3 << "\n"; // COPY IN THE REPLACEMENT
#endif
                    }
                } else {
                    outFile << "just as you are\n"; // EMPTY SEQUENCE
                }

                QString resolve = ui->label_SD_Resolve->text().simplified();
//                qDebug() << "REPLACE (RESOLVE TEXT): " << resolve;
                if (resolve != "" && resolve != "(no matches)" && resolve != "SUBSIDIARY CALL") {
                    // TODO: really should write out the LAST KNOWN RESOLVE rather than just suppressing "(no matches)"
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
//        outFile << "@\n";
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

    if (ui->listWidgetSDOptions->item(0) != nullptr) {

        if (ui->listWidgetSDOptions->item(0)->text().contains("search")) {
            // if we are resolving, and user clicked SAVE, they probably want what the resolver
            //   suggested.  So, just accept it and move on.  If we don't get out of resolve mode,
            //   Bad Things will happen.
            sdthread->do_user_input("accept current choice"); // end the search
        }
    }

    int currentFrame  = frameVisible.indexOf("central"); // 0 to M
    int currentSeqNum = frameCurSeq[currentFrame]; // 1 to N
//    qDebug() << "SDDeleteCurrentSequence DELETING SEQUENCE #" << currentSeqNum << " FROM " << frameFiles[currentFrame];

//    QString who     = QString(frameFiles[currentFrame]).replace(QRegularExpression("\\..*"), "");
//    QString level   = QString(frameFiles[currentFrame]).replace(QRegularExpression("^.*\\."), ""); // TODO: filename is <name>.<level> right now

    // open the current file for READ ONLY
//    QString pathToOLDFile = (musicRootPath + "/sd/dances/" + frameName + "/%1.txt").arg(frameFiles[currentFrame]);

    QString pathToOLDFile;
    if (frameVisible[currentFrameNumber] == "central") {
        pathToOLDFile = musicRootPath + "/sd/dances/" + frameName + "/F" + QString::number(currentFrameNumber+1) + "-" + frameFiles[currentFrameNumber] + ".txt";
    } else {
        pathToOLDFile = musicRootPath + "/sd/dances/" + frameName + "/f" + QString::number(currentFrameNumber+1) + "-" + frameFiles[currentFrameNumber] + ".txt";
    }

    QFile OLDfile(pathToOLDFile);

    if (!OLDfile.open(QIODevice::ReadOnly)) {
        qDebug() << "ERROR: could not open " << pathToOLDFile;
        return;
    }

    // in SD directory, open a new file for APPEND called "foo.bar.NEW", which is where we will build a new file to replace the old one
//    QString pathToNEWFile = (musicRootPath + "/sd/%1/SStoSS/%2.choreodb_to_csds.in.NEW").arg(who).arg(level);
//    QString pathToNEWFile = (musicRootPath + "/sd/%1.seq.txt.NEW").arg(frameFiles[currentFrame]);

    QString pathToNEWFile = pathToOLDFile + ".NEW";
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

void MainWindow::SDMoveCurrentSequenceToFrame(int i) {  // i = 0 to 3, and -1 means move to the "deleted" frame
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


//    QString pathToAppendFile = (musicRootPath + "/sd/%1/SStoSS/%2.choreodb_to_csds.in").arg(who).arg(level);
//    QString pathToAppendFile = (musicRootPath + "/sd/dances/" + frameName + "/%1.txt").arg(currentFrameTextName);

    QString pathToAppendFile;

    if (frameVisible[currentFrameNumber] == "central") {
        pathToAppendFile = musicRootPath + "/sd/dances/" + frameName + "/F" + QString::number(currentFrameNumber+1) + "-" + frameFiles[currentFrameNumber] + ".txt";
    } else {
        pathToAppendFile = musicRootPath + "/sd/dances/" + frameName + "/f" + QString::number(currentFrameNumber+1) + "-" + frameFiles[currentFrameNumber] + ".txt";
    }

//    qDebug() << "currentFrameTextName/who/level/path: " << currentFrameTextName << who << level << pathToAppendFile;

    // TODO: When making a new Frame file, it must start out with a single "@\n" line.

    QFile file(pathToAppendFile);
    if (file.open(QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << "#REC=" << 100000 * userID + nextSequenceID << "#\n";  // nextSequenceID is 5 decimal digits
        nextSequenceID++;
        writeMetadata(userID, nextSequenceID, authorID);   // update the metadata file real quick with the new nextSequenceID
        stream << "#AUTHOR=" << authorID << "#\n";
//        stream << "#HIGHLIGHT=#\n"; // no highlights, since nothing highlighted right now
        // NO TITLE RIGHT NOW, since this is new
        stream << "( NEW SEQUENCE )\n"; // this is important, so that the SD engine DOES get reset in loadFrame()
        stream << "@\n";
        file.close();

        int centralNum = frameVisible.indexOf("central");
        frameMaxSeq[centralNum] += 1;                       // add a NEW sequence to the one currently loaded into the Current Sequence window
        frameCurSeq[centralNum] = frameMaxSeq[centralNum];  // look at the new one

//        qDebug() << "now showing: " << frameCurSeq[centralNum];

        SDSetCurrentSeqs(7); // persist the new Current Sequence numbers

        currentSequenceTitle = "";
        ui->sdCurrentSequenceTitle->setText("");

        refreshSDframes();
    } else {
//        qDebug() << "ERROR: could not append to " << pathToAppendFile;
    }

//        int centralNum = frameVisible.indexOf("central");
//        frameMaxSeq[centralNum] += 1;                       // add a NEW sequence to the one currently loaded into the Current Sequence window
//        frameCurSeq[centralNum] = frameMaxSeq[centralNum];  // look at the new one
//        // NOTE: DO NOT CALL SDSETCURRENSEQS(7) HERE.  WE DO NOT WANT TO PERSIST UNTIL SAVE TIME.
//        // TODO: DISABLE ALL LEFT/RIGHT ARROW COMBINATIONS WHEN EDITING A SEQUENCE.  REENABLE WHEN SAVE OR ABORT.

        newSequenceInProgress = true;

        refreshSDframes();
}

void MainWindow::on_pushButtonSDSave_clicked()
{
//    qDebug() << "on_pushButtonSDSave_clicked";

    SDReplaceCurrentSequence(); // this is a SAVE operation to the current frame
}

void MainWindow::on_pushButtonSDRevert_clicked()
{
    //    qDebug() << "on_pushButtonSDRevert_clicked";

    // TODO: Factor this code into a function!
    if (ui->listWidgetSDOptions->item(0) != nullptr) {

        if (ui->listWidgetSDOptions->item(0)->text().contains("search")) {
            // if we are resolving, and user clicked SAVE, they probably want what the resolver
            //   suggested.  So, just accept it and move on.  If we don't get out of resolve mode,
            //   Bad Things will happen.
            sdthread->do_user_input("accept current choice"); // end the search
        }
    }

    SDExitEditMode();
    refreshSDframes(); // show the old frame, before edits (by reloading from file)
}

void MainWindow::on_pushButtonSDDelete_clicked()
{
//    qDebug() << "on_pushButtonSDDelete_clicked";
//    SDDeleteCurrentSequence(); // also exits edit mode [OBSOLETE]

    SDMoveCurrentSequenceToFrame(-1); // instead of deleting, move it to the "deleted" frame
    SDExitEditMode();  // NEEDED HERE: Move does not exit edit mode
}


// -----------------------------------------------------------------------------
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
            static QRegularExpression textInParens1("\\(.*\\)", QRegularExpression::InvertedGreedinessOption);
            name = name.replace(textInParens1, "").trimmed().toLower();

//            qDebug() << number << name;

            allCallsString += (name + ";");
        }
    }

    return(allCallsString);
}

QString MainWindow::translateCallToLevel(QString thePrettifiedCall) {
//    qDebug() << "translateCallToLevel: " << thePrettifiedCall;

    static QRegularExpression textInParens("\\(.*\\)");
    QString theCall = thePrettifiedCall.toLower().replace(textInParens, "").replace(who, "").simplified(); // lowecase, remove comments, remove WHO, consolidate whitespace

    // OK, sorry, but we CANNOT do this here, because the whole find_dance_program() function tells you the answer
    //  based on the current state of the SD engine, BEFORE the call is sent to SD.  It's designed for the "? completion"
    //  feature, and trying to look up something like "pass thru" will return "nope" if pass thru is not a valid
    //  call to ADD to the end of the current sequence.  Nice try, though.  Leaving this comment here for future spelunkers.
//    dance_level iDanceLevel = sdthread->find_dance_program(theCall); // use SD's leveler
//    QString strDanceLevel = sdLevelEnumsToStrings[iDanceLevel];      //  human-readable name for the level
//    qDebug() << "translateCallToLevel: " << theCall << strDanceLevel;

    QString allPlusCallsString = makeLevelString(danceprogram_plus);    // make big long string
    QString allA1CallsString   = makeLevelString(danceprogram_a1);      // make big long string
    QString allA2CallsString   = makeLevelString(danceprogram_a2);      // make big long string

    QString allC1CallsString = ";ah so;alter the wave;chase your neighbor;checkover;cross and turn;cross by;";
//    allC1CallsString += "";

    // TODO: not handled yet:
    // circle by (because that requires numbers, i.e. QRegularExpression

    // ***** TODO: only makeLevelString once at startup for each level
    // ***** TODO: make a C1 level string, AND stick it into a Dance Programs file

//    qDebug() << "******** CHECKING: " << theCall;
//    qDebug() << "AGAINST PLUS: " << allPlusCallsString;
//    qDebug() << "AGAINST A1: " << allA1CallsString;
//    qDebug() << "AGAINST A2: " << allA2CallsString;

    // must go from highest level to lowest!
    if (theCall.contains("alter") ||
        theCall.contains("block") ||
        theCall.contains("butterfly") ||
            theCall.contains("but ") ||  // NOTE: intentional space after "but"
            theCall.contains("cast back") ||
            theCall.contains("circle by") ||
            theCall.contains("concentric") ||
            (theCall.contains("counter rotate") && !theCall.contains("split counter rotate") && !theCall.contains("box counter rotate")) ||
            theCall.contains("cross chain") ||
            theCall.contains("cross extend") ||
            theCall.contains("cross roll") ||
            theCall.contains("cross your neighbor") ||
            theCall.contains("pass and roll your cross neighbor") ||
            (theCall.contains("chain thru") && !theCall.contains("spin chain") &&
             !theCall.contains("scoot chain") && !theCall.contains("diamond chain thru")
             && !theCall.contains("square chain thru")
             ) ||
            theCall.contains("dixie diamond") ||
            theCall.contains("dixie sashay") ||
            (theCall.contains("flip") && !theCall.contains("hourglass") && (!theCall.contains("diamond"))) ||
            theCall.contains("follow thru") ||
            theCall.contains("galaxy") ||
            theCall.contains("interlocked") ||
            theCall.contains("jaywalk") ||
            theCall.contains("jay walk") ||
            theCall.contains("linear action") ||
            theCall.contains("magic") ||
            theCall.contains("O ") || // NOTE: Intentional space after "O"
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
            (theCall.contains("rotate")  && (!theCall.contains("counter rotate"))) ||
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
            theCall.contains("vertical") ||
            (theCall.contains("to a wave") &&
             !theCall.contains("single circle") &&
             !theCall.contains("step to a") &&
             !theCall.contains("roll to a") &&
             !theCall.contains("dixie style to a wave")) ||
            (theCall.contains("weave") && !theCall.contains("the ring") && !theCall.contains("scoot and weave")) ||
            (theCall.contains("wheel and") && !theCall.contains("deal")) ||
            theCall.contains("wheel fan thru") ||
            theCall.contains("with the flow") ||
            theCall.contains("zing") ||
            allC1CallsString.contains(";" + theCall + ";")
            ) {
//        qDebug() << theCall << "is C1.";
        return("C1");
    } else if (theCall.contains("windmill") ||
               (allA2CallsString.contains(";" + theCall + ";") && (theCall != "recycle")) ||
               theCall.contains("in roll circulate") ||
               theCall.contains("out roll circulate") ||
               theCall.contains("scoot and weave") ||
               theCall.contains("counter rotate") ||
               theCall.contains("remake") ||
               theCall.startsWith("all 8") ||
               theCall.contains("motivate")                // takes care of "motivate, turn the star 1/4"
               ) {
//        qDebug() << theCall << "is A2.";
        return("A2");
    } else if (theCall.contains("any hand") ||
               theCall.contains("as couples") ||
               theCall.contains("belles") ||
               theCall.contains("beaus") ||
               theCall.contains("shadow") ||
               theCall.contains("clover and") ||
               theCall.contains("cross clover") ||
               theCall.contains("and cross") ||
               theCall.contains("explode and") ||
               theCall.contains("6x2 acey") ||
               theCall.contains("split square thru") ||
               theCall.contains("reverse swap around") ||
               theCall.contains("transfer and") ||
               theCall.contains("1/4 in") ||
               theCall.contains("1/4 out") ||
               theCall.contains("chain reaction") ||    // takes care of "chain reaction, turn the star 1/2"
               theCall.contains("wheel thru") ||
               allA1CallsString.contains(";" + theCall + ";")) {
//        qDebug() << theCall << "is A1.";
        return("A1");
    } else if (theCall.contains("and roll") ||
               theCall.contains("and spread") ||
               theCall.contains("flip the diamond") ||
               theCall.contains("cut the diamond") ||
               allPlusCallsString.contains(";" + theCall + ";")) {
//        qDebug() << theCall << "is Plus.";
        return("Plus");
    }
//    qDebug() << theCall << "is Mainstream.";

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
    QFile file((musicRootPath + "/sd/dances/" + frameName + "/sequenceUsed.csv"));
//    qDebug() << "Reading file: " << file.fileName();

    if (file.open(QIODevice::ReadOnly))
    {
        while(!file.atEnd()) {
            QString line = file.readLine().trimmed();
//            qDebug() << "frameName: " << frameName << ", sequenceUsed line: " << line;
            QStringList fields = line.split(','); // should be exactly 5 fields
            if ((fields[0] != "datetime") && (fields.count() == 5)) {  // MUST have 5 fields, or ignore
                // if not the header line: datetime,userID,sequenceID,status,reason
//                QString datetime   = fields[0];
//                QString userID     = fields[1];
                QString sequenceID = fields[2];
                QString status     = fields[3];
//                QString reason     = fields[4];

                int sequenceIDi = sequenceID.toInt();
                if (status == "GOOD") {
                    sequenceStatus[sequenceIDi] = 1;  // TODO: use named constants
                } else if (status == "BAD") {
                    sequenceStatus[sequenceIDi] = 2;  // TODO: use named constants
                } else if (status == "UNRATED") {
                    sequenceStatus[sequenceIDi] = 0;  // TODO: use named constants
                }

//                qDebug() << "decoded: " << sequenceIDi << status << sequenceStatus[sequenceIDi];
            }
        }
        file.close();
//        qDebug() << "sequenceStatus: " << sequenceStatus;
    } else {
        // TODO: This is duplicated code that could be factored into a single function.
        qDebug() << "SDReadSequencesUsed: File " << file.fileName() << " could not be opened, so creating one.";
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


void MainWindow::SDMakeFrameFilesIfNeeded() { // also sets frameVisible based on F1 vs f1 on filename

    // let's scan for files of the format: "fn-framename.txt" located in /sd/dances/danceName
    //   and populate the frameName variable with what we find.

    QStringList files;
    QString pathToFrameFile = musicRootPath + "/sd/dances/" + frameName;
    QDirIterator it(pathToFrameFile, {"*-*.txt"}, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        files << it.next().replace(pathToFrameFile + "/", ""); // just the filename, not the path
    }
    files.sort();
//    qDebug() << "SDMakeFrameFilesIfNeeded: " << files;

    // Find all files that look like: "f1-foo.txt" or "F1-foo.txt"
    static QRegularExpression f1to4Filter("[fF][1-4]-.*\\.txt");
    QStringList fList = files.filter(f1to4Filter);

    for (int ff = 1; ff < 5; ff++) { // f1-f4
        QRegularExpression regex1(QString("f%1-(.*)\\.txt").arg(ff),
                                  QRegularExpression::CaseInsensitiveOption); // case insensitive, matches f1-foo.txt and F1-foo.txt
        QStringList thisf = fList.filter(regex1);
        if (thisf.count() == 1) {
//            qDebug() << "GOOD: " << ff << thisf[0];

            QString which = "sidebar";
            if (thisf[0].startsWith("F")) {
                which = "central";          // capital F means "central", else "sidebar"
            }

            QRegularExpressionMatch match = regex1.match(thisf[0]);
            if (match.hasMatch()) {
                QString name = match.captured(1);
//                qDebug() << "MATCH: " << name << which;

                frameFiles[ff-1]   = name;  // e.g. frameFiles = "biggie"
                frameVisible[ff-1] = which; // e.g. frameVisible = "central"

            } else {
                qDebug() << "SDMakeFrameFilesIfNeeded: SOMETHING WENT WRONG: " << thisf[0];
            }


        } else if (thisf.count() == 0) {
            QString fileName = frameFiles[ff-1];                    // get the filename ('deleted' is invisible, so does not appear in frameFiles list
            QString pathToFrameFile;

            if (frameVisible[ff-1] == "central") {
                pathToFrameFile = (musicRootPath + "/sd/dances/" + frameName + "/F%1-%2.txt").arg(QString::number(ff), fileName); // NOTE: FILENAME STRUCTURE IS HERE, TOO (TODO: FACTOR THIS)
            } else {
                pathToFrameFile = (musicRootPath + "/sd/dances/" + frameName + "/f%1-%2.txt").arg(QString::number(ff), fileName); // NOTE: FILENAME STRUCTURE IS HERE, TOO (TODO: FACTOR THIS)
            }

            qDebug() << "MISSING A FRAME, SO MAKING ONE: " << ff << pathToFrameFile;  // counts on default existing in frameFiles that was NOT overwritten

            // we already know that it doesn't exist yet
            QFile file(pathToFrameFile);
            if (file.open(QFile::WriteOnly | QFile::Append)) {
                // file is created, and CSV header is written
                QTextStream out(&file);

                switch (ff) {
                    case 1: // f1-biggie
                        out << "@\n";
                        out << "#REC=725900001#\n";
                        out << "#AUTHOR=SquareDesk#\n";
//                        out << "#HIGHLIGHT=#\n";  // no highlights yet
                        out << "#TITLE=Sample Opening Biggie#\n";
                        out << "HEADS Square Thru 3\n";
                        out << "HEADS Partner Trade\n";
                        out << "( AL, HOME )\n";
                        out << "@\n";
                        break;
                    case 2: // f2-easy
                        out << "@\n";
                        out << "#REC=725900002#\n";
                        out << "#AUTHOR=SquareDesk#\n";
//                        out << "#HIGHLIGHT=#\n";  // no highlights yet
                        out << "#TITLE=Sample Easy Sequence#\n";
                        out << "HEADS Square Thru 4\n";
                        out << "( AL, HOME )\n";
                        out << "@\n";
                        break;
                    case 3: // f3-medium
                        out << "@\n";
                        out << "#REC=725900003#\n";
                        out << "#AUTHOR=SquareDesk#\n";
//                        out << "#HIGHLIGHT=#\n";  // no highlights yet
                        out << "#TITLE=Sample Medium Sequence#\n";
                        out << "HEADS Square Thru 4\n";
                        out << "Right and Left Thru\n";
                        out << "Dive Thru\n";
                        out << "CENTERS Square Thru 3\n";
                        out << "( AL, HOME )\n";
                        out << "@\n";
                        break;
                    case 4: // f4-hard
                        out << "@\n";
                        out << "#REC=725900004#\n";
                        out << "#AUTHOR=SquareDesk#\n";
//                        out << "#HIGHLIGHT=#\n";  // no highlights yet
                        out << "#TITLE=Sample Hard Sequence#\n";
                        out << "HEADS Pass The Ocean\n";
                        out << "Extend\n";
                        out << "Swing Thru\n";
                        out << "BOYS Run\n";
                        out << "Ferris Wheel\n";
                        out << "CENTERS Square Thru 3\n";
                        out << "( AL, HOME )\n";
                        out << "@\n";
                        break;
                    default:
                        break;
                }

                file.close();
            } else {
                qDebug() << "SDMakeFrameFilesIfNeeded: ERROR: could not make a new frame file: " << file.fileName();
            }
        } else {
            qDebug() << "SDMakeFrameFilesIfNeeded: ERROR: MORE THAN ONE MATCH FOR F" << ff << ":" << thisf;
        }
    }

    // special handling for "deleted.txt" file
    QString pathToDeletedDotTxtFile = (musicRootPath + "/sd/dances/" + frameName + "/deleted.txt"); // NOTE: FILENAME STRUCTURE IS HERE, TOO (TODO: FACTOR THIS)
    QFile file(pathToDeletedDotTxtFile);
    QFileInfo deletedDotTxtFileInfo(pathToDeletedDotTxtFile);

    if (!deletedDotTxtFileInfo.exists()) {
        // file doesn't exist, so create one
        qDebug() << "Making new deleted.txt file: " << file.fileName();
        if (file.open(QFile::WriteOnly | QFile::Append)) {
            // file is created, and CSV header is written
            QTextStream out(&file);
            out << "@\n";
            out << "#REC=725900005#\n";
            out << "#AUTHOR=SquareDesk#\n";
//                        out << "#HIGHLIGHT=#\n";  // no highlights yet
            out << "#TITLE=Archive of Deleted Files#\n";
            out << "( ARCHIVE OF DELETED FILES )\n";
            out << "@\n";
            file.close();
        }
    }

    // SPECIAL CHECK: All of the frameVisible's must be "sidebar" except for one "central"
    //  if this is not true, make it so.

    int firstCentral = 100;
    for (int i = 0; i < 4; i++) {
        if (i > firstCentral) {
            frameVisible[i] = "sidebar"; // all AFTER the first "central" are set to sidebar (takes care of more than one 'central')
        }
        if (frameVisible[i] == "central") {
            firstCentral = i;
        }
    }

    if (firstCentral == 100) {
        frameVisible[0] = "central";   // after we've scanned all of them, if there are no 'central's, then make the first one (F1) the 'central'
    }

//    qDebug() << "frameFiles: " << frameFiles;
//    qDebug() << "frameVisible: " << frameVisible;

}

// Dances/frames -------------------------------------
void MainWindow::sdLoadDance(QString danceName) {
    prefsManager.SetlastDance(danceName);
    QDir danceFolder(musicRootPath + "/sd/dances/" + danceName);
    if (!danceFolder.exists()) {
        qDebug() << "danceFolder does not exist, so we're gonna make one: " << danceFolder.dirName();
        danceFolder.mkpath(".");          // make the folder exist, so we can populate it with sample data below
    }

    frameName = danceName;    // TODO: "frameName" is a misnomer now.  This really should be fixed.

    frameFiles.clear();
    frameVisible.clear();
    frameCurSeq.clear();
    frameMaxSeq.clear();

    // NOTE: These are the default names for the files, if they don't already exist, e.g. f1-biggie, F2-easy, f3-medium, f4-sidebar
    frameFiles   << "biggie"           << "easy"           << "medium"            << "hard";
    frameVisible << "sidebar"          << "central"        << "sidebar"           << "sidebar";
    frameCurSeq  << 1                  << 1                << 1                   << 1;          // These are persistent in /sd/<frameName>/.current.csv
    frameMaxSeq  << 1                  << 1                << 1                   << 1;          // These are updated at init time by scanning.

//    qDebug() << "FrameVisible: " << frameVisible;

    // Do these whenever a new dance is loaded...
    SDMakeFrameFilesIfNeeded(); // if there aren't any files in <frameName>, make some
    SDGetCurrentSeqs();   // get the frameCurSeq's for each of the frameFiles (this must be
    SDScanFramesForMax(); // update the framMaxSeq's with real numbers (MUST BE DONE AFTER GETCURRENTSEQS)
    SDReadSequencesUsed();  // update the local cache with the status that was persisted in this sequencesUsed.csv

    refreshSDframes();
}


void MainWindow::sdActionTriggeredDances(QAction * action) {
//    qDebug() << "Dance selected:" << action->text();

//    if (frameName == action->text()) {
//        // if the name did not change (i.e. don't retrigger)
//        return;
//    }

//    frameName = action->text();

    sdLoadDance(action->text());
}

void MainWindow::toggleHighlight() {
    if (!ui->lineEditSDInput->isVisible()) {
//        qDebug() << "CANNOT TOGGLE, NOT IN EDIT MODE";
        return;  // need to be in Unlock/Edit mode to change highlights
    }

    int rowToToggle = ui->tableWidgetCurrentSequence->rowCount() - sdUndoToLine;
    if (rowToToggle == -1) {
      return; // not on a real row
    }

    QString callToToggle = ui->tableWidgetCurrentSequence->item(rowToToggle,0)->text().simplified().toLower();
    static QRegularExpression userComment("\\(.*\\)");  // user typed comment, e.g. "( foo )"
    callToToggle = callToToggle.replace(who, "").replace(userComment, "").simplified(); // remove the WHO at the front of the call and the optional comment (and collapse whitespace)

//    qDebug() << "TOGGLING: " << callToToggle;

    if (highlightedCalls.contains(callToToggle)) {
      // remove it from the list
      highlightedCalls.remove(highlightedCalls.indexOf(callToToggle));
    } else {
      // add it to the list
      highlightedCalls.append((callToToggle));
    }

    // refresh central frame
    refreshHighlights();
}

void MainWindow::clearHighlights() {
    if (!ui->lineEditSDInput->isVisible()) {
//        qDebug() << "CANNOT CLEAR, NOT IN EDIT MODE";
        return;  // need to be in Unlock/Edit mode to change highlights
    }

    highlightedCalls.clear();

    // refresh central frame
    refreshHighlights();
}

void MainWindow::refreshHighlights() {
    for (int i = 0; i < ui->tableWidgetCurrentSequence->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->tableWidgetCurrentSequence->item(i, 0);
        QString theCall = theItem->text().simplified().toLower();

        bool containsHighlightedCall = false;
        for ( const auto& call : highlightedCalls )
        {
            if (theCall.contains(call)) {
                containsHighlightedCall = true;
            }
        }

        if (containsHighlightedCall) {
            theItem->setForeground(QBrush(QColor("red"))); // set highlighted items to RED
        } else {
            theItem->setForeground(QBrush(QColor("black"))); // set unhighlighted items to BLACK (even if they were RED before)
        }
    }

    ui->lineEditSDInput->setFocus(); // move the focus to the input field, so that all highlights are visible
    // right now, if an item is selected, its foreground color is forced to WHITE.
}

// ---------------------------------------------------------------------------------------------------
// translates a user command to either 1 (if no comments) or 2 (if comments) strings
//   if 2, then first is user input line cleaned up, second is the call
QStringList MainWindow::UserInputToSDCommand(QString userInputString) {
//    qDebug() << "UserInputToSDCommand input: " << userInputString;

    QStringList outList;

    // STANDALONE COMMENTS ----------------
    // Example:
    // ( this is a standalone comment )
    // NOTE: This must be checked first, so it matches the whole line.

    static QRegularExpression standaloneComment("^[\\(\\{](.*)[\\)\\}]$"); // with capture of the standalone comment (not including the parens/curly braces)
    QRegularExpressionMatch matchSAC = standaloneComment.match(userInputString.trimmed()); // just get rid of spaces at start and end!

    if (matchSAC.hasMatch()) {
        QString theSAC  = matchSAC.captured(1).simplified(); // and trim whitespace

        outList << theSAC;        // standalone comment
        outList << "nothing";     // the call "nothing" is allowed by SD, and indicates a standalone comment

        return (outList);
    }

    // SUFFIX COMMENTS --------------------------
    // Example:
    // Square Thru ( this is a suffix comment )
    //
    // This will get translated to a prefix comment: "|this is a suffix comment|"

    static QRegularExpression suffixComment("([/a-zA-Z0-9\\s]+)\\s*[\\(\\{](.*)[\\)\\}]$"); // with capture of the comment (not including the parens/curly braces)
    // must not match single line comments
    QRegularExpressionMatch matchSC = suffixComment.match(userInputString);

    if (matchSC.hasMatch()) {
        QString theSC   = matchSC.captured(2).simplified(); // and trim whitespace
        QString theCall = matchSC.captured(1).simplified(); // and trim whitespace

        outList << QString("|" + theSC + "|"); // suffix comments are coded as prefix comments with vertical bars
        outList << theCall;   // the call with suffix comments removed

        return (outList);
    }

    // PREFIX COMMENTS --------------------------
    // Example:
    // ( this is a prefix comment ) Square Thru
    static QRegularExpression prefixComment("^[\\(\\{](.*)[\\)\\}][ ]*(.*)$"); // with capture of the comment (not including the parens/curly braces)
    QRegularExpressionMatch matchPC = prefixComment.match(userInputString);

    if (matchPC.hasMatch()) {
        QString thePC   = matchPC.captured(1).simplified(); // and trim whitespace
        QString theCall = matchPC.captured(2).simplified(); // and trim whitespace

        outList << thePC;   // prefix comments are sent to SD as normal prefix comments
        outList << theCall; // the call with prefix comments removed

        return (outList);
    }

    // TODO: prefix-suffix comment ---------------
    // Example:
    // ( prefix ) call ( suffix )
    // this could be encoded into a single prefix comment for SD, and restored on the other end

    // NO COMMENTS AT ALL ------------------------
    outList << userInputString;

    return (outList);
}

// translates an SD output string to a string to be presented to the user in the Current Sequence table
QString MainWindow::SDOutputToUserOutput(QString SDoutputString) {
//    qDebug() << "===============================";
//    qDebug() << "SDOutputToUserOutput input: " << SDoutputString;

    static QRegularExpression sdCommentAndCall("(.*)\\s*\\{(.*)\\}\\s+(.*)");  // capture the SD prefix comment and call
    // note: 3 cases
    //   1a. { comment } call
    //   1b. { comment } nothing <-- standalone comment!
    //   2. HEADS { comment } call
    //   3. call

    QString theWHO;
    QString theComment;
    QString theCall;

    QRegularExpressionMatch matchSDcomment = sdCommentAndCall.match(SDoutputString);
    if (matchSDcomment.hasMatch()) {
        theWHO     = matchSDcomment.captured(1);
        theComment = matchSDcomment.captured(2);
        theCall    = matchSDcomment.captured(3);
    } else {
        theWHO     = "";
        theComment = "";
        theCall    = SDoutputString;
    }

    QString finalUserVisibleResult;
    theCall    = upperCaseWHO(toCamelCase(theWHO + theCall)).simplified();

    if (theCall == "Nothing" && theComment != "") {
        // 1b. STANDALONE COMMENT
        finalUserVisibleResult = "(" + theComment.simplified() + ")";
    } else if (theComment.contains("|")) {
        // 1a. SUFFIX COMMENT
        theComment = theComment.replace("|","").simplified(); // replace all
        finalUserVisibleResult = theCall + " (" + theComment + ")";
    } else if (theComment != "") {
        // 1a. PREFIX COMMENT
        finalUserVisibleResult = "(" + theComment.simplified() + ") " + theCall;
    } else {
        // 3. NO COMMENT AT ALL
        finalUserVisibleResult = theCall;
    }

//    qDebug() << "SDOutputToUserOutput output:" << finalUserVisibleResult;
    return finalUserVisibleResult;
}

void MainWindow::sdtest() {
#ifdef NEWCOMMENTS
//    QStringList list1 = {"Square Thru 4",
//                         "( prefix comment ) Square Thru 4",
//                         "Square  ( infix comment ) Thru 4",
//                         "Square Thru 4 ( suffix comment )",
//                         "( comment1 ) Square ( comment 2 ) Thru (comment 3) 4 ( comment 4)",
//                         "Swing Thru"};
//    UserInputToSDCommand(list1);
//    qDebug() << "-------------";
//    UserInputToSDCommand(QStringList("HEADS Square Thru 4"));
//    UserInputToSDCommand(QStringList("( prefix comment ) HEADS Square Thru 4"));
//    UserInputToSDCommand(QStringList("HEADS Square  ( infix comment ) Thru 4"));
//    UserInputToSDCommand(QStringList("HEADS Square Thru 4 ( suffix comment )"));
//    UserInputToSDCommand(QStringList("( comment1 ) Square ( comment 2 ) Thru (comment 3) 4 ( comment 4)"));
//    qDebug() << "-------------";
//    UserInputToSDCommand(QStringList("HEADS Square Thru 4"), true);
//    UserInputToSDCommand(QStringList("( prefix comment ) HEADS Square Thru 4"), true);
//    UserInputToSDCommand(QStringList("HEADS Square  ( infix comment ) Thru 4"), true);
//    UserInputToSDCommand(QStringList("HEADS Square Thru 4 ( suffix comment )"), true);
//    UserInputToSDCommand(QStringList("( comment1 ) Square ( comment 2 ) Thru (comment 3) 4 ( comment 4)"), true);
//    QStringList list2 = {
//                         "( comment1 ) HEADS Square ( comment 2 ) Thru (comment 3) 4 ( comment 4)",
//                         "Heads Square Thru 4",
//                         "( prefix comment ) Square Thru 4",
//                         "heads Square  ( infix comment ) Thru 4",
//                         "Sides Square Thru 4 ( suffix comment )",
//                         "Sides Swing Thru"};
//    UserInputToSDCommand(list2, true);
//    qDebug() << "-------------";
//    SDOutputToUserOutput(QStringList("square thru 4"));
//    SDOutputToUserOutput(QStringList("{ ( comment1 ) heads Square ( comment 2 ) Thru (comment 3) 4 ( comment 4) } heads square thru 4"));
//    SDOutputToUserOutput(QStringList("heads square thru 4"));
//    SDOutputToUserOutput(QStringList("{ ( comment1 ) sides Square ( comment 2 ) Thru (comment 3) 4 ( comment 4) } heads square thru 4"));
//    QStringList list3 = {
//                         "{( comment1 ) HEADS Square ( comment 2 ) Thru (comment 3) 4 ( comment 4)} heads square thru 4",
//                         "Heads Square Thru 4",
//                         "{( prefix comment ) Square Thru 4} square thru 4",
//                         "{heads Square  ( infix comment ) Thru 4} heads square thru 4",
//                         "{Sides Square Thru 4 ( suffix comment )} sides square thru 4",
//                         "Sides Swing Thru"};
//    SDOutputToUserOutput(list3);
#endif
}

void MainWindow::readAbbreviations() {
    // reads the abbrevs.txt file into a struct called abbreviations
    // if there isn't one, a default one is copied in

    abbrevs.clear();

//    qDebug() << "reading abbrevs.txt file...";

    QString fileName(musicRootPath + "/sd/abbrevs.txt");  // user-editable file in the SD directory
    QFile file(fileName);

    bool fileExists = QFileInfo::exists(fileName);

    if (!fileExists) {
//        qDebug() << "***** readAbbreviations: abbrevs.txt file not found, so making one";
        QString source = QCoreApplication::applicationDirPath() + "/../Resources/abbrevs.txt"; // default abbreviations file location
        QString destination = musicRootPath + "/sd/abbrevs.txt";
        QFile::copy(source, destination);
//        qDebug() << "***** file copied from: " << source << " to: " << destination;
    }

    if ( file.open(QIODevice::ReadOnly) )
    {
        QTextStream in(&file);

        while(!in.atEnd()) {
            QString line = in.readLine();

            static QRegularExpression commentDelimiter("^\\s*#");
            if (!line.contains(commentDelimiter)) {
                // if it's NOT a comment...
                static QRegularExpression whitespace("\\s+");
                QStringList list = line.split(whitespace);  // tokenize it

                if (list.count() >= 2) { // ignore malformed lines
                        QString abbrev = list[0];                             // first item is the key
                        QString call   = list.last(list.count()-1).join(" "); // last N items concatenated is the value
                        abbrevs[abbrev] = call.toLower();  // add to the table (lower case it first to make it case insensitive
                }
            } else {
//                qDebug() << "Skipping comment: " << line;
            }
        }
        file.close();  // close the file when done reading
    } else {
        qDebug() << "ERROR: COULD NOT OPEN ABBREVS.TXT, EVEN AFTER COPY.";
    }
//    qDebug() << "abbrevs: " << abbrevs;
//    qDebug() << "DONE reading abbreviations.";
}

QString MainWindow::expandAbbreviations(QString s) {
    // expands a string based on the current abbreviations table

//    qDebug() << "expandAbbreviations: " << s;

    static QRegularExpression whitespace("\\s+");
    QStringList tokens = s.replace("[", " [ ").replace("]", " ] ").split(whitespace); // Note special handling for brackets
    QStringList result;
    for (const auto &token : tokens) {
        if (abbrevs.contains(token)) {
            result << abbrevs[token]; // if a match is found, replace token with replacement string
        } else {
            result << token;          // if no match is found, just use the token itself, no replacement
        }
    }

    QString r = result.join(" ").replace(" [ ", " [").replace(" ] ", "] ").simplified();
//    qDebug() << "FINAL RESULT: " << r;

    return(r);
}

void MainWindow::on_actionSave_Current_Dance_As_HTML_triggered()
{
    // Ask me where to save it...
    RecursionGuard dialog_guard(inPreferencesDialog);

    QString nowISO8601 = QDateTime::currentDateTime().toString(Qt::ISODate).replace("-","").replace(":","");
    QString pathToDefaultHTMLOutputFile = (musicRootPath + "/sd/dances/" + frameName + "/" + frameName + "_" + nowISO8601 + "Z.html"); // frameName here is really "dance name"
//    qDebug() << "pathToDefaultHTML: " << pathToDefaultHTMLOutputFile;

    QString outputFilename = QFileDialog::getSaveFileName(this,
                                                            tr("Save Dance As HTML"),
                                                            pathToDefaultHTMLOutputFile,
                                                            tr("HTML (*.html *.htm)"));
    if (!outputFilename.isNull())
    {
        // ====== get the list of filenames to output
        QDirIterator dirIt(musicRootPath + "/sd/dances/" + frameName);
        QStringList files;
        while (dirIt.hasNext()) {
            dirIt.next();
            QFileInfo fileInfo(dirIt.filePath());
            if (fileInfo.isFile() && fileInfo.suffix() == "txt") {
                if (!dirIt.filePath().endsWith("deleted.txt")) { // we do NOT want to output the deleted sequences
                        files << dirIt.filePath();
                }
            }
        }

        files.sort(Qt::CaseInsensitive);
        //    qDebug() << "sorted file list: " << files;

        // ======= now open up the HTML output file
        QFile file(outputFilename);
        if ( !file.open(QIODevice::WriteOnly) ) {
            return;  // could not open the file for writing
        }

        QTextStream stream( &file );

        QString newVersionString = QString("<!-- squaredesk:version = ") + QString(VERSIONSTRING) + QString(" -->");

        stream << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n\n";
        stream << "<HTML>\n";
        stream << "    <HEAD> " + newVersionString + "\n";
        stream << "    <TITLE> " + frameName + "</TITLE>\n";
        stream << "        <STYLE>\n";
        stream << "            body, p, font { font-family: Verdana; font-size: 3.0em; font-weight: Normal; color: #000000; background: #FFFFFF; margin-top: 0px; margin-bottom: 0px;}\n";
        stream << "            .dance        { font-size: 2.0em;     font-weight: 699;    color: #000000;}\n";
        stream << "            .frame        { font-size: 1.5em;     font-weight: Normal; color: #FF0002;}\n";
        stream << "        </STYLE>\n";
        stream << "    </HEAD>\n";
        stream << "    <BODY>\n";
        stream << "        <SPAN class=\"dance\">" << frameName << "</SPAN>\n";

        // ====== FOR EACH OF THE INPUT FILES
        for ( const auto& f : files )
        {
            QFile inFile(f);

            if (inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&inFile);

                QString shortFilename = f.split("/").last().replace(".txt","");

                stream << "            <BR><SPAN class=\"frame\">" << shortFilename << "</SPAN><BR>\n";

                // READ THE ENTIRE FILE
                QString allLines = in.readAll();
                QStringList allLines2 = allLines.split("\n");

                while (allLines2.last().startsWith("@") || allLines2.last() == "") {
                        allLines2.removeLast(); // remove blank lines and last @ line
                }

                // Now iterate over the lines, splitting into sequences in the HTML file
                int sequenceNumber = 0;
                for ( const auto& line : allLines2 )
                {
                    if (line.startsWith("@")) {
                        sequenceNumber++;
                        if (sequenceNumber != 1) {
                            stream << "            <BR>\n";
                        }
                        stream << "\n            # " << sequenceNumber << " ---------------------------------<BR>\n"; // horizontal rule between sequences
                    } else if (line.startsWith("#")) {
                        // do nothing, it's a comment
                    } else {
                        // normal line, just spit it out
                        stream << "            " << line << "<BR>\n";
                    }
                }

                inFile.close();
            } else {
                // Could not open file
            }
        }

        stream << "    </BODY>\n";
        stream << "</HTML>\n";

        stream.flush();
        file.close();
    }
}

// --------------------------------------------------
void MainWindow::on_actionSave_Sequence_triggered()
{
    // SD > Save Sequence (does the same thing as pressing the SAVE button
    on_pushButtonSDSave_clicked();
}

void MainWindow::on_actionPrint_Sequence_triggered()
{
    // SD > Print Sequence...
    QPrinter printer;
    QPrintDialog printDialog(&printer, this);

    printDialog.setWindowTitle("Print SD Sequence");

    if (printDialog.exec() == QDialog::Rejected) {
        return;
    }

    QTextDocument doc;
    QSizeF paperSize;
    paperSize.setWidth(printer.width());
    paperSize.setHeight(printer.height());
    doc.setPageSize(paperSize);

    QString contents(get_current_sd_sequence_as_html(true, true));
    doc.setHtml(contents);
    doc.print(&printer);
}
