#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "renderarea.h"
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
#include <QCoreApplication>
#include <QInputDialog>
#include <QLineEdit>
#include "common.h"
#include "../sdlib/database.h"

static double dancerGridSize = 20;



static QGraphicsItemGroup *generateDancer(QGraphicsScene &sdscene, int number, bool boy)
{
    static QPen pen(Qt::black, 1.5, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);
    static QBrush brushes[4] = { QBrush(COUPLE1COLOR),
                                 QBrush(COUPLE2COLOR),
                                 QBrush(COUPLE3COLOR),
                                 QBrush(COUPLE4COLOR)
    };

    static float rectSize = 20;
    static QRectF rect(-rectSize/2, -rectSize/2, rectSize, rectSize);
    static QRectF directionRect(-2,-rectSize / 2 - 4,4,4);

    QGraphicsItem *mainItem = boy ?
        (QGraphicsItem*)(sdscene.addRect(rect, pen, brushes[number]))
        :   (QGraphicsItem*)(sdscene.addEllipse(rect, pen, brushes[number]));
    
    QGraphicsRectItem *directionRectItem = sdscene.addRect(directionRect, pen, brushes[number]);
    QGraphicsTextItem *label = sdscene.addText(QString("%1").arg(number + 1));

    QRectF labelBounds(label->boundingRect());
    QTransform labelTransform;
    labelTransform.translate(-(labelBounds.left() + labelBounds.width() / 2),
                             -(labelBounds.top() + labelBounds.height() / 2));
    label->setTransform(labelTransform);

    QList<QGraphicsItem*> items;
    items.append(mainItem);
    items.append(directionRectItem);
    items.append(label);
    QGraphicsItemGroup *group = sdscene.createItemGroup(items);
    return group;
}


static void draw_scene(const QStringList &sdformation,
                  QList<QGraphicsItemGroup*> &sdpeople)
{
    int coupleNumber = -1;
    int girl = 0;
    double max_x = -1;
    double max_y = sdformation.length() - 1;
        
    for (int y = 0; y < sdformation.length(); ++y)
    {
        if (sdformation[y].length() > max_x)
            max_x = sdformation[y].length();
    }
    max_x = max_x - 4; // account for trailing newlines, leading
                       // spaces & 2 for center of dancer
        
    for (int y = 0; y < sdformation.length(); ++y)
    {
        double dancer_start_x = -1.0;
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
                dancer_start_x = (double)(x);
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
                        QTransform transform;
//                        qDebug() << "Dancer " << coupleNumber << " " << (girl ? "girl" : "boy") << " at " << dancer_start_x << " max " << max_x;
                        double dancer_x = ((double)(dancer_start_x - 1) - max_x / 2.0) / 3.0;
                        double dancer_y = ((double)(y) - max_y / 2.0);
//                            qDebug() << "Couple " << coupleNumber << " " << (girl ? "Girl" : "Boy") << " at "
//                                     << dancer_x << "," << dancer_y << ", direction " << direction
//                                     << " x " << dancer_start_x << " max_x " << max_x;
                        transform.translate( dancer_x * dancerGridSize,
                                             dancer_y * dancerGridSize);
                        transform.rotate(direction);
//                            qDebug() << "Transform: " << transform;
                        sdpeople[dancerNum]->setTransform(transform);
                    }
                }
                draw = false;
                direction = -1;
                coupleNumber = -1;
                    
            }
        }
    }
}


void MainWindow::initialize_internal_sd_tab()
{
    ui->listWidgetSDOutput->setIconSize(QSize(128,128));
    
    const double backgroundSize = 140.0;
    QPen backgroundPen(Qt::black);
    QPen gridPen(Qt::black,0.25,Qt::DotLine);
    QBrush backgroundBrush(QColor(240,240,240));
    QRectF backgroundRect(-backgroundSize, -backgroundSize, backgroundSize * 2, backgroundSize * 2);
    sdscene.addRect(backgroundRect, backgroundPen, backgroundBrush);
    graphicsTextItemSDStatusBarText = sdscene.addText("");
    QTransform statusBarTransform;
    statusBarTransform.translate(-backgroundSize, -backgroundSize);
    graphicsTextItemSDStatusBarText->setTransform(statusBarTransform);
    
    for (double x =  -backgroundSize + dancerGridSize; x < backgroundSize; x += dancerGridSize)
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
        

        QGraphicsItemGroup *boyGroup = generateDancer(sdscene, i, true);
        QGraphicsItemGroup *girlGroup = generateDancer(sdscene, i, false);

        boyGroup->setTransform(boyTransform);
        girlGroup->setTransform(girlTransform);
        
        sdpeople.append(boyGroup);
        sdpeople.append(girlGroup);
    }
    
    ui->graphicsViewSDFormation->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    ui->graphicsViewSDFormation->setScene(&sdscene);

    QStringList initialDancerLocations;
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
        sdformation.append(str);
    }
    else
    {
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
    
        draw_scene(sdformation, sdpeople);
        if (!sdformation.empty())
        { 
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
            /* ui->listWidgetSDOutput->addItem(sdformation.join("\n")); */
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

    qDebug() << "on_sd_set_pick_stringe: " <<  str;
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
    qDebug() << "Double click: " << item->text();
    ui->lineEditSDInput->setText(item->text());
    emit sdthread->on_user_input(item->text());
}

void MainWindow::do_sd_tab_completion()
{
    QString originalText(ui->lineEditSDInput->text().trimmed());
    QString longestMatch;
    
    for (int i = 0; i < ui->listWidgetSDOptions->count(); ++i)
    {
        if (ui->listWidgetSDOptions->item(i)->text().startsWith(originalText, Qt::CaseInsensitive)
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
    if (longestMatch.length() > originalText.length())
    {
        ui->lineEditSDInput->setText(longestMatch);
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

void MainWindow::on_actionNormal_triggered()
{
    setCurrentCheckerColorScheme(CheckerColorSchemeNormal);
}
void MainWindow::on_actionColor_only_triggered()
{
    setCurrentCheckerColorScheme(CheckerColorSchemeColorOnly);
}
void MainWindow::on_actionMental_image_triggered()
{
    setCurrentCheckerColorScheme(CheckerColorSchemeMentalImage);
}
void MainWindow::on_actionSight_triggered()
{
    setCurrentCheckerColorScheme(CheckerColorSchemeSight);
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
