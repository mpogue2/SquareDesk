#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "renderarea.h"
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
#include <QCoreApplication>

static double dancerGridSize = 20;

static QGraphicsItemGroup *generateDancer(QGraphicsScene &sdscene, int number, bool boy)
{
    static QPen pen(Qt::black, 1.5, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin);
    static QBrush brushes[4] = { QBrush(Qt::red),
                                 QBrush(Qt::green),
                                 QBrush(Qt::blue),
                                 QBrush(Qt::yellow)
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

void MainWindow::initialize_internal_sd_tab()
{
    ui->listWidgetSDOutput->setIconSize(QSize(128,128));
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
    ui->graphicsViewSDFormation->setScene(&sdscene);
                          
}
       


void MainWindow::on_threadSD_errorString(QString str)
{
    qDebug() << "on_threadSD_errorString: " <<  str;
}

void MainWindow::on_sd_set_window_title(QString str)
{
    qDebug() << "on_sd_set_window_title: " << str;
}

void MainWindow::on_sd_update_status_bar(QString str)
{
    qDebug() << "on_sd_update_status_bar: " << str;
    ui->labelSDStatusBar->setText(str);
}

void MainWindow::on_sd_add_new_line(QString str, int drawing_picture)
{
    while (str.length() > 1 && str[str.length() - 1] == '\n')
        str = str.left(str.length() - 1);
    
    if (drawing_picture)
    {
        sdformation.append(str);
    }
    else
    {
        int coupleNumber = -1;
        int girl = 0;
        double max_x = -1;
        double max_y = sdformation.length();
        
        for (int y = 0; y < sdformation.length(); ++y)
        {
            if (sdformation[y].length() > max_x)
                max_x = sdformation[y].length();
        }
        max_x = max_x - 1; // account for newlines
        
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
                            double dancer_x = ((double)(dancer_start_x) - max_x / 2.0) / 3.0;
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
        if (!sdformation.empty())
        {
            QPixmap image(128,128);
            image.fill();
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing);
            sdscene.render(&painter);

            QListWidgetItem *item = new QListWidgetItem(sdformation.join("\n"));
            item->setIcon(QIcon(image));
            ui->listWidgetSDOutput->addItem(item);
            
            /* ui->listWidgetSDOutput->addItem(sdformation.join("\n")); */
        }
        sdformation.clear();
        ui->listWidgetSDOutput->addItem(str);
    }
}

void MainWindow::on_sd_awaiting_input()
{
    ui->listWidgetSDOutput->scrollToBottom();
}
    
void MainWindow::on_sd_set_pick_string(QString str)
{

    qDebug() << "on_sd_set_pick_stringe: " <<  str;
}

void MainWindow::on_sd_dispose_of_abbreviation(QString str)
{
    qDebug() << "on_sd_dispose_of_abbreviation: " <<  str;
}


void MainWindow::on_sd_set_matcher_options(QStringList options)
{
    ui->listWidgetSDOptions->clear();
    for (auto option = options.begin(); option != options.end(); ++option)
    {
        ui->listWidgetSDOptions->addItem(*option);
    }
}

void MainWindow::on_listWidgetSDOptions_itemDoubleClicked(QListWidgetItem *item)
{
    qDebug() << "Double click: " << item->text();
    ui->lineEditSDInput->setText(item->text());
}

void MainWindow::on_lineEditSDInput_returnPressed()
{
    qDebug() << "Return pressed, command is: " << ui->lineEditSDInput->text();
    sdthread->on_user_input(ui->lineEditSDInput->text());
    ui->lineEditSDInput->clear();
}

void MainWindow::on_lineEditSDInput_textChanged()
{
    
}

