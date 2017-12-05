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

static QFont dancerLabelFont;
static QString stringClickableCall("clickable call!");

static QGraphicsItemGroup *generateDancer(QGraphicsScene &sdscene, SDDancer &dancer, int number, bool boy)
{
    static QPen pen(Qt::black, 1.5, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);

    static float rectSize = 16;
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
//    qDebug() << "Formation: " << sdformation.length() << " " << max_y;
    for (int i = 0; i < sdformation.length(); ++i)
    {
//        qDebug() << "  " << i << ":" << sdformation[i];
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
    double lowest_factor = 2; // just default to something that won't suck too badly
    for (size_t i = 0; i < sizeof(factors) / sizeof(*factors); ++i)
    {
        if (factors[i])
        {
            lowest_factor = i + 3;
            break;
        }
    }
//    qDebug() << "max_x " << max_x << " " << lowest_factor;

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
        NULL
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


    
#if defined(Q_OS_MAC)
    dancerLabelFont = QFont("Arial",11);
#elif defined(Q_OS_WIN)
    dancerLabelFont = QFont("Arial",8);
#else
    dancerLabelFont = QFont("Arial",8);
#endif
    ui->listWidgetSDOutput->setIconSize(QSize(128,128));

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

    for (double x =  -halfBackgroundSize + gridSize;
         x < halfBackgroundSize; x += gridSize)
    {
        QPen &pen(x == 0 ? axisPen : gridPen);
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

    ui->graphicsViewSDFormation->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    ui->graphicsViewSDFormation->setScene(&sdscene);

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
    ui->tableWidgetCurrentSequence->horizontalHeader()->setVisible(false);
    ui->tableWidgetCurrentSequence->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableWidgetCurrentSequence->setColumnWidth(1, currentSequenceIconSize + 8);
    QHeaderView *verticalHeader = ui->tableWidgetCurrentSequence->verticalHeader();
    verticalHeader->setSectionResizeMode(QHeaderView::Fixed);
    initialTableWidgetCurrentSequenceDefaultSectionSize =
        verticalHeader->defaultSectionSize();
    verticalHeader->setDefaultSectionSize(currentSequenceIconSize);
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
    if (!str.compare("<startup>"))
    {
        sdformation = initialDancerLocations;
    }
    graphicsTextItemSDStatusBarText->setPlainText(str);
    QString formation(graphicsTextItemSDStatusBarText->toPlainText() +
                      "\n" + sdformation.join("\n"));
    if (!sdformation.empty())
    {
        draw_scene(sdformation, sdpeople);
        QPixmap image(sdListIconSize,sdListIconSize);
        image.fill();
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        sdscene.render(&painter);

        QListWidgetItem *item = new QListWidgetItem();
        item->setData(Qt::UserRole, formation);
        item->setIcon(QIcon(image));
        ui->listWidgetSDOutput->addItem(item);
    }
    if (!sdformation.empty() && sdLastLine >= 1)
    {
        int row = sdLastLine >= 2 ? (sdLastLine - 2) : 0;

        render_current_sd_scene_to_tableWidgetCurrentSequence(row, formation);
        QTableWidgetItem *item = ui->tableWidgetCurrentSequence->item(row,0);
        item->setData(Qt::UserRole, QVariant(formation));
        /* ui->listWidgetSDOutput->addItem(sdformation.join("\n")); */
    }
    sdformation.clear();
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
            call_text += " - " + sdLevelEnumsToStrings[available_call.dance_program];
        }
        else
        {
            call_text += QString(" - (%0)").arg(available_call.dance_program);
        }
        if (available_call.dance_program <= current_dance_program
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

void MainWindow::on_listWidgetSDQuestionMarkComplete_itemDoubleClicked(QListWidgetItem *item)
{
    do_sd_double_click_call_completion(item);
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
//    qInfo() << "setting matcher options " << options << " " << levels;
    ui->listWidgetSDOptions->clear();
    for (int i = 0; i < options.length(); ++i)
    {
        // if it's not "exit the program"
        if (options[i].compare(str_exit_from_the_program))
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

void MainWindow::render_current_sd_scene_to_tableWidgetCurrentSequence(int row, const QString &formation)
{
    QPixmap image(currentSequenceIconSize,currentSequenceIconSize);
    image.fill();
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    sdscene.render(&painter);

    QTableWidgetItem *item = new QTableWidgetItem();
    item->setData(Qt::UserRole, formation);
    item->setData(Qt::DecorationRole,image);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);

    ui->tableWidgetCurrentSequence->setItem(row, 1, item);
    QTableWidgetItem *textItem = ui->tableWidgetCurrentSequence->item(row,0);
//    qDebug() << "Setting row " << row << " to formation " << formation;
    textItem->setData(Qt::UserRole, QVariant(formation));
}

void MainWindow::set_current_sequence_icons_visible(bool visible)
{
    if (visible)
    {
        QHeaderView *verticalHeader = ui->tableWidgetCurrentSequence->verticalHeader();
        verticalHeader->setDefaultSectionSize(currentSequenceIconSize);
    }
    else
    {
        QHeaderView *verticalHeader = ui->tableWidgetCurrentSequence->verticalHeader();
        verticalHeader->setDefaultSectionSize(initialTableWidgetCurrentSequenceDefaultSectionSize);
    }
    ui->tableWidgetCurrentSequence->setColumnHidden(1,!visible);
}

void MainWindow::do_sd_double_click_call_completion(QListWidgetItem *item)
{
    QString call(item->text());
    QStringList calls = call.split(" - ");
    if (calls.size() > 1)
    {
        call = calls[0];
    }
    ui->lineEditSDInput->setText(call);
    qInfo() << "Double click on " << call;
    qInfo() << "   contains: " << (call.contains("<") && call.contains(">")) << " " << call.contains("<") << " " << call.contains(">");
    
    if (!(call.contains("<") && call.contains(">")))
    {
        on_lineEditSDInput_returnPressed();
    }
    else
    {
        int lessThan = call.indexOf("<");
        int greaterThan = call.indexOf(">");
        ui->lineEditSDInput->setSelection(lessThan, greaterThan - lessThan + 1);
        ui->lineEditSDInput->setFocus();
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
                graphicsTextItemSDStatusBarText->setPlainText(formationList[0]);
                formationList.removeFirst();
                draw_scene(formationList, sdpeople);
            }
        }

    }
}

void MainWindow::on_listWidgetSDOptions_itemDoubleClicked(QListWidgetItem *item)
{
    do_sd_double_click_call_completion(item);
}


static QString get_longest_match_from_list_widget(QListWidget *listWidget,
                                                  QString originalText,
                                                  QString callSearch,
                                                  QString longestMatch = QString())
{
    
    for (int i = 0; i < listWidget->count(); ++i)
    {
        if ((listWidget->item(i)->text().startsWith(callSearch,
                                                                 Qt::CaseInsensitive)
             || listWidget->item(i)->text().startsWith(originalText,
                                                                    Qt::CaseInsensitive))
            && !listWidget->isRowHidden(i))
        {
            if (longestMatch.isEmpty())
            {
                longestMatch = listWidget->item(i)->text();
            }
            else
            {
                QString thisItem = listWidget->item(i)->text();
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
    QString callSearch = sd_strip_leading_selectors(originalText);
    QString prefix = originalText.left(originalText.length() - callSearch.length());

    
    QString longestMatch(get_longest_match_from_list_widget(ui->listWidgetSDOptions,
                                                            originalText,
                                                            callSearch));
    longestMatch = get_longest_match_from_list_widget(ui->listWidgetSDQuestionMarkComplete,
                                                      originalText,
                                                      originalText,
                                                      longestMatch);
    
    if (longestMatch.length() > callSearch.length())
    {
        if (prefix.length() != 0 && !prefix.endsWith(" "))
        {
            prefix = prefix + " ";
        }
        QString new_line(prefix + longestMatch);
        if (new_line.contains('<'))
        {
            new_line = new_line.left(new_line.indexOf('<'));
        }
        ui->lineEditSDInput->setText(new_line);
    }

}

void MainWindow::on_lineEditSDInput_returnPressed()
{
//    qDebug() << "Return pressed, command is: " << ui->lineEditSDInput->text();
    QString cmd(ui->lineEditSDInput->text().trimmed());
    ui->lineEditSDInput->clear();

    qInfo() << "Doing user input 0" << cmd;

    if (!cmd.compare("quit", Qt::CaseInsensitive))
    {
        cmd = "abort this sequence";
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
    cmd = cmd.replace(QRegExp("^go "),"").replace(QRegExp("^do a "),"").replace(QRegExp("^do "),"");
    cmd = cmd.replace("DOTHECENTERS", "do the centers");  // special case "do the centers part of load the boat"

    // handle specialized sd spelling of flutter wheel, and specialized wording of reverse flutter wheel
    cmd = cmd.replace("flutterwheel","flutter wheel");
    cmd = cmd.replace("reverse the flutter","reverse flutter wheel");

    // handle specialized sd wording of first go *, next go *
    cmd = cmd.replace(QRegExp("first[a-z ]* go left[a-z ]* next[a-z ]* go right"),"first couple go left, next go right");
    cmd = cmd.replace(QRegExp("first[a-z ]* go right[a-z ]* next[a-z ]* go left"),"first couple go right, next go left");
    cmd = cmd.replace(QRegExp("first[a-z ]* go left[a-z ]* next[a-z ]* go left"),"first couple go left, next go left");
    cmd = cmd.replace(QRegExp("first[a-z ]* go right[a-z ]* next[a-z ]* go right"),"first couple go right, next go right");

    // handle "single circle to an ocean wave" -> "single circle to a wave"
    cmd = cmd.replace("single circle to an ocean wave","single circle to a wave");

    // handle manually-inserted brackets
    cmd = cmd.replace(QRegExp("left bracket\\s+"), "[").replace(QRegExp("\\s+right bracket"),"]");

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
    cmd = cmd.replace(QRegExp("dixie style.*"), "dixie style to a wave");

    // handle the <anything> and roll case
    //   NOTE: don't do anything, if we added manual brackets.  The user is in control in that case.
    if (!cmd.contains("[")) {
        QRegExp andRollCall("(.*) and roll.*");
        if (cmd.indexOf(andRollCall) != -1) {
            cmd = "[" + andRollCall.cap(1) + "] and roll";
        }

        // explode must be handled *after* roll, because explode binds tightly with the call
        // e.g. EXPLODE AND RIGHT AND LEFT THRU AND ROLL must be translated to:
        //      [explode and [right and left thru]] and roll

        // first, handle both: "explode and <anything> and roll"
        //  by the time we're here, it's already "[explode and <anything>] and roll", because
        //  we've already done the roll processing.
        QRegExp explodeAndRollCall("\\[explode and (.*)\\] and roll");
        QRegExp explodeAndNotRollCall("^explode and (.*)");

        if (cmd.indexOf(explodeAndRollCall) != -1) {
            cmd = "[explode and [" + explodeAndRollCall.cap(1).trimmed() + "]] and roll";
        } else if (cmd.indexOf(explodeAndNotRollCall) != -1) {
            // not a roll, for sure.  Must be a naked "explode and <anything>"
            cmd = "explode and [" + explodeAndNotRollCall.cap(1).trimmed() + "]";
        } else {
        }
    }

    // handle <ANYTHING> and spread
    if (!cmd.contains("[")) {
        QRegExp andSpreadCall("(.*) and spread");
        if (cmd.indexOf(andSpreadCall) != -1) {
            cmd = "[" + andSpreadCall.cap(1) + "] and spread";
        }
    }

    // handle "undo [that]" --> "undo last call"
    cmd = cmd.replace("undo that", "undo last call");
    if (cmd == "undo") {
        cmd = "undo last call";
    }

    // handle "peel your top" --> "peel the top"
    cmd = cmd.replace("peel your top", "peel the top");

    // handle "veer (to the) left/right" --> "veer left/right"
    cmd = cmd.replace("veer to the", "veer");

    // handle the <anything> and sweep case
    // FIX: this needs to add center boys, etc, but that messes up the QRegExp

    // <ANYTHING> AND SWEEP (A | ONE) QUARTER [MORE]
    QRegExp andSweepPart(" and sweep.*");
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
    if (cmd == "square thru") {
        cmd = "square thru 4";
    }

    // SD COMMANDS -------
    // square your|the set -> square thru 4
    if (cmd == "square the set" || cmd == "square your set") {
        sdthread->do_user_input("abort this sequence");
        sdthread->do_user_input("y");
    }
    qInfo() << "Doing user input 1" << cmd;
    sdthread->do_user_input(cmd);
}

dance_level MainWindow::get_current_sd_dance_program()
{
    dance_level current_dance_program = (dance_level)(INT_MAX);

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
//    qInfo() << "lineEditSDInput_textChanged()";
    bool showCommands = ui->actionShow_Commands->isChecked();
    bool showConcepts = ui->actionShow_Concepts->isChecked();

    
    QString currentText = ui->lineEditSDInput->text().simplified();
    int currentTextLastChar = currentText.length() - 1;
    
    if (currentTextLastChar >= 0 &&
        (currentText[currentTextLastChar] == '!' ||
         currentText[currentTextLastChar] == '?'))
    {
        on_lineEditSDInput_returnPressed();
        ui->lineEditSDInput->setText(currentText.left(currentTextLastChar));
        sdLineEditSDInputLengthWhenAvailableCallsWasBuilt = currentTextLastChar;
        return;
    }
    else
    {
        if (currentTextLastChar < sdLineEditSDInputLengthWhenAvailableCallsWasBuilt)
            sdAvailableCalls.clear();
    }
    
    QString strippedText = sd_strip_leading_selectors(currentText);

    dance_level current_dance_program = get_current_sd_dance_program();
    
//    qInfo() << "Looking at call options for " << current_dance_program << " showCommands: "
//            << showCommands << " showConcepts " << showConcepts << " currentText " << currentText
//            << " options count " << ui->listWidgetSDOptions->count();

    for (int i = 0; i < ui->listWidgetSDOptions->count(); ++i)
    {
        bool show = ui->listWidgetSDOptions->item(i)->text().startsWith(currentText, Qt::CaseInsensitive) ||
            ui->listWidgetSDOptions->item(i)->text().startsWith(strippedText, Qt::CaseInsensitive);

//        qInfo() << "Showing " << ui->listWidgetSDOptions->item(i)->text() << " currentText " << currentText << " stripped " << strippedText << " / " << ui->listWidgetSDOptions->item(i);
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
                    static QRegExp regexpReplaceableParts("<.*?>");
                    QString callName = ui->listWidgetSDOptions->item(i)->text();
                    QStringList callParts(callName.split(regexpReplaceableParts, QString::SkipEmptyParts));
                    
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
//                    qInfo() << "Call: " << callParts << " inAvailableCalls " << inAvailableCalls;
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

void MainWindow::setCurrentSDDanceProgram(dance_level dance_program)
{
    sdthread->set_dance_program(dance_program);
//    for (int i = 0; actions[i]; ++i)
//    {
//        bool checked = (i == (int)(dance_program));
//        actions[i]->setChecked(checked);
//    }
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
    QGraphicsRectItem* rectItem = dynamic_cast<QGraphicsRectItem*>(mainItem);
    QGraphicsEllipseItem* ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(mainItem);
    if (rectItem) rectItem->setBrush(QBrush(color));
    if (ellipseItem) ellipseItem->setBrush(QBrush(color));
    directionRectItem->setBrush(QBrush(color));
}


void MainWindow::setSDCoupleColoringScheme(const QString &colorScheme)
{
    bool showDancerLabels = (colorScheme == "Normal");
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
        sdpeople[COUPLE2 * 2 + 1].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE3 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE3 * 2 + 1].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE4 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE4 * 2 + 1].setColor(GREYCOUPLECOLOR);
    } else {
        // Sight
        sdpeople[COUPLE1 * 2 + 0].setColor(COUPLE1COLOR);
        sdpeople[COUPLE1 * 2 + 1].setColor(COUPLE1COLOR);
        sdpeople[COUPLE2 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE2 * 2 + 1].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE3 * 2 + 0].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE3 * 2 + 1].setColor(GREYCOUPLECOLOR);
        sdpeople[COUPLE4 * 2 + 0].setColor(COUPLE4COLOR);
        sdpeople[COUPLE4 * 2 + 1].setColor(COUPLE4COLOR);
    }
}


void MainWindow::undo_last_sd_action()
{
    sdthread->do_user_input("undo last call");
}

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
                selected = true;
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
    QAction action2("Undo", this);
    connect(&action2, SIGNAL(triggered()), this, SLOT(undo_last_sd_action()));
    contextMenu.addAction(&action2);
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
    contextMenu.addAction(&action2);
    contextMenu.exec(ui->listWidgetSDOutput->mapToGlobal(pos));
}
