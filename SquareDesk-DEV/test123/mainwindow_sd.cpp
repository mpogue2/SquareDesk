#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "renderarea.h"
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
#include <QCoreApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QClipboard>
#include "common.h"
#include "../sdlib/database.h"

static double dancerGridSize = 20;

static QBrush coupleColorBrushes[4] = { QBrush(COUPLE1COLOR),
                             QBrush(COUPLE2COLOR),
                             QBrush(COUPLE3COLOR),
                             QBrush(COUPLE4COLOR)
};

static QFont dancerLabelFont;

static QGraphicsItemGroup *generateDancer(QGraphicsScene &sdscene, SDDancer &dancer, int number, bool boy)
{
    static QPen pen(Qt::black, 1.5, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);

    static float rectSize = 20;
    static QRectF rect(-rectSize/2, -rectSize/2, rectSize, rectSize);
    static QRectF directionRect(-2,-rectSize / 2 - 4,4,4);

    QGraphicsItem *mainItem = boy ?
        (QGraphicsItem*)(sdscene.addRect(rect, pen, coupleColorBrushes[number]))
        :   (QGraphicsItem*)(sdscene.addEllipse(rect, pen, coupleColorBrushes[number]));
    
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
    items.append(mainItem);
    items.append(directionRectItem);
    items.append(label);
    QGraphicsItemGroup *group = sdscene.createItemGroup(items);
    dancer.graphics = group;
    dancer.mainItem = mainItem;
    dancer.directionRectItem = directionRectItem;
    dancer.label = label;
       
    return group;
}


static void draw_scene(const QStringList &sdformation,
                       QList<SDDancer> &sdpeople)
{
    int coupleNumber = -1;
    int girl = 0;
    double max_y = (double)(sdformation.length());
    qDebug() << "Formation: " << sdformation.length() << " " << max_y;
    for (int i = 0; i < sdformation.length(); ++i)
    {
        qDebug() << "  " << i << ":" << sdformation[i];
    }
        
        
    for (int y = 0; y < sdformation.length(); ++y)
    {
        int dancer_start_x = -1;
        bool draw = false;
        float direction = -1;
            
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
                        sdpeople[dancerNum].x = dancer_start_x;
                        sdpeople[dancerNum].y = y;
                        sdpeople[dancerNum].direction = direction;
                    }
                }
                draw = false;
                direction = -1;
                coupleNumber = -1;
                    
            }
        } /* end of for x */
    } /* end of for y */

    bool factors[4];
    double left_x = -1;
    
    for (int dancerNum = 0; dancerNum < sdpeople.length(); dancerNum++)
    {
        int dancer_start_x = sdpeople[dancerNum].x;
        if (left_x < 0 || dancer_start_x < left_x)
            left_x = dancer_start_x;
    }
    
    for (size_t i = 0; i < sizeof(factors) / sizeof(*factors); ++i)
        factors[i] = true;
    int max_x = -1;

    for (int dancerNum = 0; dancerNum < sdpeople.length(); dancerNum++)
    {
        int dancer_start_x = sdpeople[dancerNum].x - left_x;
        if (dancer_start_x > max_x)
            max_x = dancer_start_x;

        for (size_t i = 0; i < sizeof(factors) / sizeof(*factors); ++i)
        {
            if (0 != (dancer_start_x % (i + 3)))
            {
                factors[i] = false;
            }
        }
    }
    double lowest_factor = -1;
    for (size_t i = 0; i < sizeof(factors) / sizeof(*factors); ++i)
    {
        if (factors[i])
        {
            lowest_factor = i + 3;
            break;
        }
    }
    qDebug() << "max_x " << max_x << " " << lowest_factor;

    max_x /= lowest_factor;
        
    for (int dancerNum = 0; dancerNum < sdpeople.length(); dancerNum++)
    {
        QTransform transform;
        double dancer_start_x = sdpeople[dancerNum].x - left_x;
        double dancer_x = (dancer_start_x / lowest_factor - max_x / 2.0);
        double dancer_y = (sdpeople[dancerNum].y - max_y / 2.0);
        transform.translate( dancer_x * dancerGridSize,
                             dancer_y * dancerGridSize);
        transform.rotate(sdpeople[dancerNum].direction);
        sdpeople[dancerNum].graphics->setTransform(transform);

        // Gyrations to make sure that the text labels are always upright
        QTransform textTransform;
        textTransform.translate( -sdpeople[dancerNum].labelTranslateX,
                                 -sdpeople[dancerNum].labelTranslateY );
        textTransform.rotate( -sdpeople[dancerNum].direction);
        textTransform.translate( sdpeople[dancerNum].labelTranslateX,
                                 sdpeople[dancerNum].labelTranslateY );
        sdpeople[dancerNum].label->setTransform(textTransform); // Rotation(-sdpeople[dancerNum].direction);
    }

}


void MainWindow::initialize_internal_sd_tab()
{
#if defined(Q_OS_MAC)
    dancerLabelFont = QFont("Arial",11);
#elif defined(Q_OS_WIN)
    dancerLabelFont = QFont("Arial",8);
#else
    dancerLabelFont = QFont("Arial",8);
#endif
    ui->listWidgetSDOutput->setIconSize(QSize(128,128));
    
    const double backgroundSize = 120.0;
    QPen backgroundPen(Qt::black);
    QPen gridPen(Qt::black,0.25,Qt::DotLine);
    QBrush backgroundBrush(QColor(240,240,240));
    QRectF backgroundRect(-backgroundSize, -backgroundSize, backgroundSize * 2, backgroundSize * 2);
    sdscene.addRect(backgroundRect, backgroundPen, backgroundBrush);
    graphicsTextItemSDStatusBarText = sdscene.addText("", dancerLabelFont);
    QTransform statusBarTransform;
    statusBarTransform.translate(-backgroundSize, -backgroundSize);
    statusBarTransform.scale(2,2);
    graphicsTextItemSDStatusBarText->setTransform(statusBarTransform);

    const double gridSize = 40;
    for (double x =  -backgroundSize + gridSize; x < backgroundSize; x += gridSize)
    {
        sdscene.addLine(x, -backgroundSize, x, backgroundSize, gridPen);
        sdscene.addLine(-backgroundSize, x, backgroundSize, x, gridPen);
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
    
    ui->graphicsViewSDFormation->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    ui->graphicsViewSDFormation->setScene(&sdscene);

    QStringList initialDancerLocations;
    initialDancerLocations.append("");
    initialDancerLocations.append("  .    3GV   3BV    .");
    initialDancerLocations.append("");
    initialDancerLocations.append(" 4B>    .     .    2G<");
    initialDancerLocations.append("");
    initialDancerLocations.append(" 4G>    .     .    2B<");
    initialDancerLocations.append("");
    initialDancerLocations.append("  .    1B^   1G^    .");
    draw_scene(initialDancerLocations, sdpeople);
    
    QStringList tableHeader;
    tableHeader << "call" << "result";
    ui->tableWidgetCurrentSequence->setHorizontalHeaderLabels(tableHeader);
    ui->tableWidgetCurrentSequence->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->tableWidgetCurrentSequence->horizontalHeader()->setVisible(true);
    ui->tableWidgetCurrentSequence->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    QHeaderView *verticalHeader = ui->tableWidgetCurrentSequence->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader->setDefaultSectionSize(64);
}
       


void MainWindow::on_threadSD_errorString(QString /* str */)
{
//    qDebug() << "on_threadSD_errorString: " <<  str;
}

void MainWindow::on_sd_set_window_title(QString /* str */)
{
//    qDebug() << "on_sd_set_window_title: " << str;
}

void MainWindow::on_sd_update_status_bar(QString str)
{
//    qDebug() << "on_sd_update_status_bar: " << str;
    graphicsTextItemSDStatusBarText->setPlainText(str);

    if (!sdformation.empty())
    {
            draw_scene(sdformation, sdpeople);
            QPixmap image(128,128);
            image.fill();
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing);
            sdscene.render(&painter);

            QListWidgetItem *item = new QListWidgetItem();
            item->setData(Qt::UserRole, graphicsTextItemSDStatusBarText->toPlainText() +
                          "\n" + sdformation.join("\n"));
            item->setIcon(QIcon(image));
            ui->listWidgetSDOutput->addItem(item);
    }
    if (!sdformation.empty() && sdLastLine >= 1)
    { 
        QPixmap image(64,64);
        image.fill();
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        sdscene.render(&painter);
        
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setData(Qt::UserRole, graphicsTextItemSDStatusBarText->toPlainText() +
                      "\n" + sdformation.join("\n"));
        item->setData(Qt::DecorationRole,image);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);

        int row = sdLastLine >= 2 ? (sdLastLine - 2) : 0;
        ui->tableWidgetCurrentSequence->setItem(row, 1, item);
        item = ui->tableWidgetCurrentSequence->item(row,0);
        item->setData(Qt::UserRole, graphicsTextItemSDStatusBarText->toPlainText() +
                      "\n" + sdformation.join("\n"));
        /* ui->listWidgetSDOutput->addItem(sdformation.join("\n")); */
    }
    sdformation.clear();
}


void MainWindow::on_sd_add_new_line(QString str, int drawing_picture)
{
    if (!drawing_picture)
    {
        str = str.trimmed();
        if (str.isEmpty())
            return;
    }

    if (str.startsWith("Output file is \""))
    {
        sdLastLine = 0;
        ui->tableWidgetCurrentSequence->setRowCount(0);
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
            sdLastLine = match.captured(1).toInt();
            if (sdLastLine > 0 && !move.isEmpty())
            {
                if (ui->tableWidgetCurrentSequence->rowCount() < sdLastLine)
                    ui->tableWidgetCurrentSequence->setRowCount(sdLastLine);
                QTableWidgetItem *moveItem = new QTableWidgetItem(match.captured(2));
                moveItem->setFlags(moveItem->flags() & ~Qt::ItemIsEditable);
                
                
                ui->tableWidgetCurrentSequence->setItem(sdLastLine - 1, 0, moveItem);
            }
        }

        // Drawing the people happens over in on_sd_update_status_bar
        ui->listWidgetSDOutput->addItem(str);
    }
}

void MainWindow::on_sd_awaiting_input()
{
    ui->listWidgetSDOutput->scrollToBottom();
    ui->tableWidgetCurrentSequence->setRowCount(sdLastLine > 1 ? sdLastLine - 1 : sdLastLine);
    ui->tableWidgetCurrentSequence->scrollToBottom();
    on_lineEditSDInput_textChanged();
}
    
void MainWindow::on_sd_set_pick_string(QString str)
{

    qDebug() << "on_sd_set_pick_string: " <<  str;
}

void MainWindow::on_sd_dispose_of_abbreviation(QString str)
{
    qDebug() << "on_sd_dispose_of_abbreviation: " <<  str;
}


void MainWindow::on_sd_set_matcher_options(QStringList options, QStringList levels)
{
    ui->listWidgetSDOptions->clear();
    for (int i = 0; i < options.length(); ++i)
    {
        QListWidgetItem *item = new QListWidgetItem(options[i]);
        QVariant v(levels[i].toInt());
        item->setData(Qt::UserRole, v);
        ui->listWidgetSDOptions->addItem(item);
    }
}


void MainWindow::on_tableWidgetCurrentSequence_itemDoubleClicked(QListWidgetItem *item)
{
    QVariant v = item->data(Qt::UserRole);
    if (!v.isNull())
    {
        QString formation(v.toString());
        QStringList formationList = formation.split("\n");
        if (formationList.size() > 0)
        {
            graphicsTextItemSDStatusBarText->setPlainText(formationList[0]);
            formationList.removeFirst();
            draw_scene(formationList, sdpeople);
        }

    }
}



void MainWindow::on_listWidgetSDOutput_itemDoubleClicked(QListWidgetItem *item)
{
    QVariant v = item->data(Qt::UserRole);
    if (!v.isNull())
    {
        QString formation(v.toString());
        QStringList formationList = formation.split("\n");
        if (formationList.size() > 0)
        {
            graphicsTextItemSDStatusBarText->setPlainText(formationList[0]);
            formationList.removeFirst();
            draw_scene(formationList, sdpeople);
        }

    }
}

void MainWindow::on_listWidgetSDOptions_itemDoubleClicked(QListWidgetItem *item)
{
    QString originalText(ui->lineEditSDInput->text().simplified());
    QString longestMatch;
    QString callSearch = sd_strip_leading_selectors(originalText);
    QString prefix = originalText.left(originalText.length() - callSearch.length());

    QString doubleClickedCall = item->text().simplified();
    if (prefix.length() != 0 && !prefix.endsWith(" "))
    {
        prefix = prefix + " ";
    }
    doubleClickedCall = prefix + doubleClickedCall;
    
//    ui->lineEditSDInput->setText(doubleClickedCall);
    emit sdthread->on_user_input(doubleClickedCall);
    ui->lineEditSDInput->clear();
    ui->lineEditSDInput->setFocus();
}

void MainWindow::do_sd_tab_completion()
{
    QString originalText(ui->lineEditSDInput->text().simplified());
    QString longestMatch;
    QString callSearch = sd_strip_leading_selectors(originalText);
    QString prefix = originalText.left(originalText.length() - callSearch.length());

    for (int i = 0; i < ui->listWidgetSDOptions->count(); ++i)
    {
        if (ui->listWidgetSDOptions->item(i)->text().startsWith(callSearch, Qt::CaseInsensitive)
            && !ui->listWidgetSDOptions->isRowHidden(i))
        {
            if (longestMatch.isEmpty())
            {
                longestMatch = ui->listWidgetSDOptions->item(i)->text();
            }
            else
            {
                QString thisItem = ui->listWidgetSDOptions->item(i)->text();
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
    if (longestMatch.length() > callSearch.length())
    {
        ui->lineEditSDInput->setText(prefix + longestMatch);
    }
    
}

void MainWindow::on_lineEditSDInput_returnPressed()
{
//    qDebug() << "Return pressed, command is: " << ui->lineEditSDInput->text();
    QString cmd(ui->lineEditSDInput->text().trimmed());
    if (!cmd.compare("quit", Qt::CaseInsensitive))
    {
        cmd = "abort this sequence";
    }
    emit sdthread->on_user_input(cmd);
    ui->lineEditSDInput->clear();
}

void MainWindow::on_lineEditSDInput_textChanged()
{
    bool showCommands = ui->actionShow_Commands->isChecked();
    bool showConcepts = ui->actionShow_Concepts->isChecked();

    QString s = ui->lineEditSDInput->text();
    if (s.length() > 0 &&
        (s[s.length() - 1] == '!' || s[s.length() - 1] == '?'))
    {
        on_lineEditSDInput_returnPressed();
        return;
    }
    s = sd_strip_leading_selectors(s);
   
    int current_dance_program = INT_MAX;

    if (ui->actionSDDanceProgramMainstream->isChecked())
    {
        current_dance_program = (int)(l_mainstream);
    }
    if (ui->actionSDDanceProgramPlus->isChecked())
    {
        current_dance_program = (int)(l_plus);
    }
    if (ui->actionSDDanceProgramA1->isChecked())
    {
        current_dance_program = (int)(l_a1);
    }
    if (ui->actionSDDanceProgramA2->isChecked())
    {
        current_dance_program = (int)(l_a2);
    }
    if (ui->actionSDDanceProgramC1->isChecked())
    {
        current_dance_program = (int)(l_c1);
    }
    if (ui->actionSDDanceProgramC2->isChecked())
    {
        current_dance_program = (int)(l_c2);
    }
    if (ui->actionSDDanceProgramC3A->isChecked())
    {
        current_dance_program = (int)(l_c3a);
    }
    if (ui->actionSDDanceProgramC3->isChecked())
    {
        current_dance_program = (int)(l_c3);
    }
    if (ui->actionSDDanceProgramC3x->isChecked())
    {
        current_dance_program = (int)(l_c3x);
    }
    if (ui->actionSDDanceProgramC4->isChecked())
    {
        current_dance_program = (int)(l_c4);
    }
    if (ui->actionSDDanceProgramC4x->isChecked())
    {
        current_dance_program = (int)(l_c4x);
    }
    
    for (int i = 0; i < ui->listWidgetSDOptions->count(); ++i)
    {
        bool show = ui->listWidgetSDOptions->item(i)->text().startsWith(s, Qt::CaseInsensitive);
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

void MainWindow::setCurrentCheckerColorScheme(CheckerColorScheme colorScheme)
{
    QAction *actions[] = {
        ui->actionNormal,
        ui->actionColor_only,
        ui->actionMental_image,
        ui->actionSight,
        NULL
    };
    for (int i = 0; actions[i]; ++i)
    {
        bool checked = (i == (int)(colorScheme));
        actions[i]->setChecked(checked);
    }
    // TODO: Actually set color scheme
}

void MainWindow::setCurrentSDDanceProgram(dance_level dance_program)
{
    QAction *actions[] = {
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
        NULL
    };
    
    for (int i = 0; actions[i]; ++i)
    {
        bool checked = (i == (int)(dance_program));
        actions[i]->setChecked(checked);
    }
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

void SDDancer::setColor(const QColor &color)
{
    QGraphicsRectItem* rectItem = dynamic_cast<QGraphicsRectItem*>(mainItem);
    QGraphicsEllipseItem* ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(mainItem);
    if (rectItem) rectItem->setBrush(QBrush(color));
    if (ellipseItem) ellipseItem->setBrush(QBrush(color));
    directionRectItem->setBrush(QBrush(color));
}


void MainWindow::setSDCoupleColoringScheme(const QString &colorScheme)
{
    bool showDancerLabels = !(colorScheme == "Color only");
    for (int dancerNum = 0; dancerNum < sdpeople.length(); ++dancerNum)
    {
        sdpeople[dancerNum].label->setVisible(showDancerLabels);
    }
    
    if (colorScheme == "Normal" || colorScheme == "Color only") {
        sdpeople[COUPLE1 * 2 + 0].setColor(COUPLE1COLOR);
        sdpeople[COUPLE1 * 2 + 1].setColor(COUPLE1COLOR);
        sdpeople[COUPLE2 * 2 + 0].setColor(COUPLE2COLOR);
        sdpeople[COUPLE2 * 2 + 1].setColor(COUPLE2COLOR);
        sdpeople[COUPLE3 * 2 + 0].setColor(COUPLE3COLOR);
        sdpeople[COUPLE3 * 2 + 1].setColor(COUPLE3COLOR);
        sdpeople[COUPLE4 * 2 + 0].setColor(COUPLE4COLOR);
        sdpeople[COUPLE4 * 2 + 1].setColor(COUPLE4COLOR);
    } else if (colorScheme == "Mental image") {
        sdpeople[COUPLE1 * 2 + 0].setColor(COUPLE1COLOR);
        sdpeople[COUPLE1 * 2 + 1].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE2 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE2 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE3 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE3 * 2 + 1].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE4 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE4 * 2 + 1].setColor(GREYCOUPLECOLOR);
    } else {
        // Sight
        sdpeople[COUPLE1 * 2 + 0].setColor(COUPLE1COLOR);
        sdpeople[COUPLE1 * 2 + 1].setColor(COUPLE1COLOR);
        sdpeople[COUPLE2 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE2 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE3 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE3 * 2 + 1].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE4 * 2 + 0].setColor(COUPLE4COLOR);
        sdpeople[COUPLE4 * 2 + 1].setColor(COUPLE4COLOR);
    }
}


// actionNormal
// actionColor_only
// actionMental_image
// actionSight

void MainWindow::copy_selection_from_tableWidgetCurrentSequence()
{
    QString selection;
    for (int row = 0; row < ui->tableWidgetCurrentSequence->rowCount(); ++row)
    {
        bool selected(false);
        for (int col = 0; col < ui->tableWidgetCurrentSequence->columnCount(); ++col)
        {
            QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,col);
            if (item->isSelected())
            {
                selected =true;
            }
        }
        if (selected)
        {
            QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,0);
            selection += item->text() + "\n";
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

void MainWindow::on_tableWidgetCurrentSequence_customContextMenuRequested(const QPoint &pos) 
{
    QMenu contextMenu(tr("Sequence"), this);

    QAction action1("Copy", this);
    connect(&action1, SIGNAL(triggered()), this, SLOT(copy_selection_from_tableWidgetCurrentSequence()));
    contextMenu.addAction(&action1);
    contextMenu.exec(ui->tableWidgetCurrentSequence->mapToGlobal(pos));
}


void MainWindow::on_listWidgetSDOutput_customContextMenuRequested(const QPoint &pos) 
{
    QMenu contextMenu(tr("Sequence"), this);

    QAction action1("Copy", this);
    connect(&action1, SIGNAL(triggered()), this, SLOT(copy_selection_from_listWidgetSDOutput()));
    contextMenu.addAction(&action1);
    contextMenu.exec(ui->listWidgetSDOutput->mapToGlobal(pos));
}
