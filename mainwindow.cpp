#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QFile>
//#include <QSettings>
#include <QTextCodec>
#include <QProxyStyle>
#include <QStyleFactory>
#include <windows.h> //to use ini files without QSettings, because there is not way to read "path" values (with backslashes)

const float min_float=-3.4028234663852886e+38;

//=======================================================================================
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    onlineTrend=true;

    connect(ui->buttonPrint,SIGNAL(clicked()),this,SLOT(ButtonPrint()));
    connect(ui->buttonSaveToFile,SIGNAL(clicked()),this,SLOT(ButtonSaveToFile()));

    connect(ui->buttonCollapse,SIGNAL(clicked()),this,SLOT(ButtonCollapse()));
    connect(ui->buttonExpand,SIGNAL(clicked()),this,SLOT(ButtonExpand()));

    connect(ui->button50Back,SIGNAL(clicked()),this,SLOT(Button50Back()));
    connect(ui->button100Back,SIGNAL(clicked()),this,SLOT(Button100Back()));
    connect(ui->button50Forward,SIGNAL(clicked()),this,SLOT(Button50Forward()));
    connect(ui->button100Forward,SIGNAL(clicked()),this,SLOT(Button100Forward()));

    connect(ui->comboBoxTheme,SIGNAL(currentIndexChanged(int)),this,SLOT(ComboBoxThemeChanged(int)));

    connect(ui->buttonClose,SIGNAL(clicked()),this,SLOT(close()));

    useThread=true;
    loadThread.mw=this;
    connect(&loadThread,SIGNAL(backDayLoad()),this,SLOT(ThreadLoadDay()));

    ui->buttonPrint->setVisible(false);
    ui->buttonSaveToFile->setVisible(false);
  //  this->setGeometry(300,100,640,480);

    // Инициализируем объект полотна для графика ...
    wGraphic = new QCustomPlot();

    // Инициализируем вертикальную линию - визир, порядок добавление соответствует порядку отрисовки
    // т.е. если визир и лэйбл добавлен после трендов, они будет поверх, иначе под трендами
    vizirLine = new QCPCurve(wGraphic->xAxis, wGraphic->yAxis);


    //читаем из ини файла
    QString iniName=qApp->arguments().last();


    if (iniName.size()>2)
    {
        if (!((iniName[0]=='/' && iniName[1]=='/') ||   //для совместимости с предыдущ версией
            (iniName[0]=='\\' && iniName[1]=='\\')))    //not a absolute path, create absolute path
        {
            iniName=qApp->applicationDirPath()+"\\"+iniName;
        }

    }
    else
    {
        QMessageBox::critical(this,"Load error!!!","Ini file load error");
    }


    ui->verticalLayoutGraphic->addWidget(wGraphic);


    //шкалы слева и справа
    wGraphic->yAxis->setRange(0,10);
    wGraphic->yAxis->setAutoTickStep(false);
    wGraphic->yAxis->setTickStep(2.5);
    wGraphic->yAxis->setTickLabelColor(Qt::transparent);
    wGraphic->yAxis->setVisible(true);

    wGraphic->yAxis2->setRange(0,10);
    wGraphic->yAxis2->setAutoTickStep(false);
    wGraphic->yAxis2->setTickStep(2.5);
    wGraphic->yAxis2->setTickLabelColor(Qt::transparent);
    wGraphic->yAxis2->setVisible(true);


    //wGraphic->yAxis->setVisible(false); //уберем шкалу используемую для визира

    // Подключаем сигналы событий мыши от полотна графика к слотам для их обработки
    connect(wGraphic, &QCustomPlot::mousePress, this, &MainWindow::slotMousePress);
    connect(wGraphic, &QCustomPlot::mouseMove, this, &MainWindow::slotMouseMove);

    wGraphic->addPlottable(vizirLine);   // Добавляем линию на полотно

    wGraphic->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    //wGraphic->xAxis->setDateTimeFormat("hh:mm:ss");
    wGraphic->xAxis->setAutoTickStep(false);
    //wGraphic->xAxis->setTickStep(7200);
    //wGraphic->xAxis->setDateTimeSpec(Qt::LocalTime); // to set ticks starts from 00:00:00, not from timezone ( +03:00:00)


    /*
    QSettings settings(qApp->applicationDirPath()+"/"+iniName,QSettings::IniFormat);

    settings.setIniCodec("Windows-1251");
    settings.beginGroup("Main");




    sTitle=settings.value("Title").toString();
    sTrendFilesPath=settings.value("TrendFilesPath");

    qDebug() << sTrendFilesPath;
    */

    //приходится испольовать WinAPI потому как QSettings не поддерживает чтение windows-путей из ини файла
    //(бэкслэши) решения проблемы под Qt нет
    char  temp[512];
    QTextCodec *codec=QTextCodec::codecForName("Windows-1251");

    GetPrivateProfileStringA("Main","Title","",temp,512,iniName.toStdString().c_str());
    sTitle=codec->toUnicode(temp);
    ui->labelTitle->setText(sTitle);

    GetPrivateProfileStringA("Main","TrendFilesPath","",temp,512,iniName.toStdString().c_str());
    sTrendFilesPath=codec->toUnicode(temp);

    //GetPrivateProfileStringA("Main","Fullscreen","0",temp,512,QString(iniName).toStdString().c_str());
    //if (codec->toUnicode(temp)=="1") {showMaximized();}

    //xData.reserve(1728000);
    for(uint i=0;;++i)
    {
        QString section;
        section = "Tag"+QString::number(i+1);
        GetPrivateProfileStringA(section.toStdString().c_str(),"FileName","",temp,512,iniName.toStdString().c_str());
        QString tmp_fileName=codec->toUnicode(temp);
        GetPrivateProfileStringA(section.toStdString().c_str(),"TagName","",temp,512,iniName.toStdString().c_str());
        QString tmp_tagName=codec->toUnicode(temp);

        if (tmp_fileName=="" || tmp_tagName=="") break;

        //создаем объект тренда и добавляем в вектор
        Trend *tr=new Trend(wGraphic);
        //tr->yData.reserve(1728000);  //two month
        tr->fileName=tmp_fileName;
        tr->tagName=tmp_tagName;

        GetPrivateProfileStringA(section.toStdString().c_str(),"Min","",temp,512,iniName.toStdString().c_str());
        sscanf(temp,"%f",&(tr->min));
        GetPrivateProfileStringA(section.toStdString().c_str(),"Max","",temp,512,iniName.toStdString().c_str());
        sscanf(temp,"%f",&(tr->max));

        GetPrivateProfileStringA(section.toStdString().c_str(),"Desc","",temp,512,iniName.toStdString().c_str());
        tr->desc=codec->toUnicode(temp);

        GetPrivateProfileStringA(section.toStdString().c_str(),"Color","",temp,512,iniName.toStdString().c_str());
        bool bOk;
        tr->color=QColor(QRgb(codec->toUnicode(temp).toInt(&bOk,16)));

//QString::number(,16)
        // Добавляем графики на полотно
        tr->yAxis=new QCPAxis(wGraphic->axisRect(),QCPAxis::atLeft);

        tr->yAxis->setRange(tr->min, tr->max);
        wGraphic->addGraph(wGraphic->xAxis,tr->yAxis);

        tr->graph=wGraphic->graph(i);
        tr->graph->setAntialiased(false);



        if(tr->color==QColor(QRgb(0)))  //если цвет не задан, создадим цвета по умолчанию для совместимости с предыдущей версией, rgb оттуда
        {
            if (i==0) tr->color=QColor(255,0,0);
            if (i==1) tr->color=QColor(255,255,0);  //желтый, было в оригинале 255,255,0, сделал темнее
            if (i==2) tr->color=QColor(0,0,255);
            if (i==3) tr->color=QColor(0,255,0);
            if (i==4) tr->color=QColor(255,0,255);;
            if (i==5) tr->color=QColor(228,120,51);
            if (i>5)  tr->color=Qt::gray;
        }

        tr->graph->setPen(QPen(tr->color));

        // Инициализируем трассировщик 1
        tr->tracer = new QCPItemTracer(wGraphic);
        tr->tracer->setGraph(wGraphic->graph(i));   // Трассировщик будет работать с графиком

        tr->tracer->setStyle(QCPItemTracer::tsNone);
        //tracer->setInterpolating(true);  - интерполяция, в противном случае отображается ближ. значение

        tr->labelName.setText(tr->tagName);
        tr->labelDesc.setText(tr->desc);

        tr->labelColorBox.setFixedWidth(15);
        tr->labelColorBox.setFixedHeight(15);
        ui->gridLayoutTrendsInfo->addWidget(&(tr->labelColorBox),i,0);
        QPalette palette=tr->labelColorBox.palette();
        palette.setColor(tr->labelColorBox.backgroundRole(),tr->color);
        tr->labelColorBox.setAutoFillBackground(true);
        tr->labelColorBox.setPalette(palette);


        ui->gridLayoutTrendsInfo->addWidget(&(tr->checkBox),i,1);
        tr->checkBox.setChecked(true);
        ui->gridLayoutTrendsInfo->addWidget(&(tr->labelName),i,2);
        ui->gridLayoutTrendsInfo->addWidget(&(tr->lineEditValue),i,3);
        tr->lineEditValue.setFixedWidth(100);
        tr->lineEditValue.setAlignment(Qt::AlignHCenter);
        ui->gridLayoutTrendsInfo->addWidget(&(tr->labelDesc),i,4);



        trends.push_back(tr);
    }


    vizirLabel = new QCPItemText(wGraphic);


    // start load data - previous 2 hours
    QDateTime dt_current=QDateTime::currentDateTime();
    startLoadedDT=QDateTime::currentDateTime();

    AllTrendsAddFromToDay(dt_current.date(), QTime(0,0,0), dt_current.time());


    wGraphic->xAxis->setRange(dt_current.addSecs(-7200).toTime_t(),dt_current.toTime_t());

    //загрузим данные если нужны за прошлый день
    CheckNeededBackDaysAndLoad(QDateTime::fromTime_t(wGraphic->xAxis->range().lower).date());

    displayedInterval_sec=7200;

    //покажем последние данные, как онлайн
    foreach(Trend *tr,trends)
    {
        tr->tracer->setGraphKey(dt_current.toTime_t());
    }

    RecalcGridInterval();
    wGraphic->replot();

    foreach(Trend *tr,trends)
    {
        tr->lineEditValue.setText(QString::number(tr->tracer->position->value(),'f',3));
    }




    //Theme select
    ui->comboBoxTheme->addItem("BlackJack Theme");
    ui->comboBoxTheme->addItem("Jack White Theme");
    ui->comboBoxTheme->addItem("Dorian Gray Theme");



    //showFullScreen();
    //showMaximized();

}
//============================================================
MainWindow::~MainWindow()
{
    xData.clear();

    foreach(Trend *tr, trends)
    {
        delete tr;
    }

    //delete vizirLine;
    delete wGraphic;
    delete ui;
}
//=========================================================================================================
//=========================================================================================================
void MainWindow::slotMousePress(QMouseEvent *event)
{

    // Определяем координату X на графике, где был произведён клик мышью
    double coordX = wGraphic->xAxis->pixelToCoord(event->pos().x());


    if (event->buttons() & Qt::LeftButton)
    {


    // Подготавливаем координаты по оси X для переноса вертикальной линии
    static QVector<double> x(2), y(2);
    x[0] = coordX;
    y[0] = 0;
    x[1] = coordX;
    y[1] = 100;

    // Устанавливаем новые координаты
    vizirLine->setData(x, y);
    //date and time over vizir

    vizirLabel->position->setType(QCPItemPosition::ptPlotCoords);
    vizirLabel->setPositionAlignment(Qt::AlignBottom|Qt::AlignHCenter);
    //vizirLabel->setPositionAlignment(Qt::AlignVCenter|Qt::AlignLeft);
    vizirLabel->position->setAxes(wGraphic->xAxis,wGraphic->yAxis);

    vizirLabel->position->setCoords(wGraphic->xAxis->pixelToCoord(event->pos().x()), wGraphic->yAxis->pixelToCoord(event->pos().y()) /*event->pos().y()*/); // place position at center/top of axis rect
    vizirLabel->setText(QDateTime::fromTime_t( wGraphic->xAxis->pixelToCoord(event->pos().x())).toString(" dd.MM.yy \n hh:mm:ss "));
    //textLabel->setFont(QFont(font().family(), 16)); // make font a bit larger
    vizirLabel->setBrush(QBrush(Qt::yellow));
    vizirLabel->setPen(QPen(Qt::black)); // show black border around text



    // По координате X клика мыши определим ближайшие координаты для трассировщика
    foreach(Trend *tr,trends)
    {
        tr->tracer->setGraphKey(coordX);
    }

    //перерисовывать обязательно перед считыванием позиций трасера, в противном случае выведет предыдущее значение
    wGraphic->replot(); // Перерисовываем содержимое полотна графика

    foreach(Trend *tr,trends)
    {
        tr->lineEditValue.setText(QString::number(tr->tracer->position->value(),'f',3));
    }


    // Выводим координаты точки графика, где установился трассировщик, в lineEdit
    /*
    ui->lineEdit->setText("x: " + QDateTime().fromTime_t(trends[0]->tracer->position->key()).toString("hh:mm:ss dd.MM.yyyy") +
                          " y1: " + QString::number(trends[0]->tracer->position->value())+
                          //" y2: " + QString::number(tr2_tracer->position->value())+
                          " x_orig:"+QString::number(event->pos().x()));
    */

    }

    if(event->buttons() & Qt::RightButton) coordX_prev=coordX;


}
//==========================================================================================================
void MainWindow::slotMouseMove(QMouseEvent *event)
{
    /* Если при передвижении мыши, ей кнопки нажаты,
     * то вызываем отработку координат мыши
     * через слот клика
     * */
    if(event->buttons() & Qt::LeftButton) slotMousePress(event);



    if(event->buttons() & Qt::RightButton)
    {

    double coordX = wGraphic->xAxis->pixelToCoord(event->pos().x());


    displayedInterval_sec=wGraphic->xAxis->range().upper-wGraphic->xAxis->range().lower;
    QDateTime currDT=QDateTime::currentDateTime();

    if (currDT < QDateTime::fromTime_t(wGraphic->xAxis->range().upper -coordX+coordX_prev))
    {

        wGraphic->xAxis->setRange(currDT.toTime_t()-displayedInterval_sec,
                                  currDT.toTime_t());
        coordX_prev=coordX;
    }
    else
    {
        wGraphic->xAxis->setRange(wGraphic->xAxis->range().lower -coordX+coordX_prev ,wGraphic->xAxis->range().upper -coordX+coordX_prev);
    }




        startDisplayedDT=startDisplayedDT.fromTime_t(wGraphic->xAxis->range().lower);
        endDisplayedDT=endDisplayedDT.fromTime_t(wGraphic->xAxis->range().upper);

        //qDebug() << startDisplayedDT;
        //qDebug() << endDisplayedDT;
        //qDebug() << startLoadedDT;

        if (!useThread)
        {
            CheckNeededBackDaysAndLoad(startDisplayedDT.date());
        }
        else
        {
            loadThread.SetDateFrom(QDateTime::fromTime_t(wGraphic->xAxis->range().lower).date());
            loadThread.start();
        }



        /*
        ui->lineEdit->setText("x: " + QString::number(coordX) +
                              " x_prev: " + QString::number(coordX_prev)+
                              " disb: " + QString::number(-coordX_prev+coordX));

        */

        qDebug() << "x: " + QString::number(coordX) +
                    " x_prev: " + QString::number(coordX_prev)+
                    " disb: " + QString::number(-coordX_prev+coordX);


        wGraphic->replot(); // Перерисовываем содержимое полотна графика

        //coordX_prev=coordX;
    }


}
//===========================================================================================================
void MainWindow::ButtonPrint()
{

}
//====================================================================
void MainWindow::OnlineRepaint()
{


}
//=================================================================================
void MainWindow::ButtonSaveToFile()
{


}
//=================================================================================
void MainWindow::TrendAddFullDay(QString name, QDate date, QVector<double> *pyData)
{
    static float file_buff[17280];
    static QVector<double> tmp_vect(17280);

    QString filename;

    filename.sprintf("%s%s_%.2u_%.2u_%.4u.trn",sTrendFilesPath.toStdString().c_str(),name.toStdString().c_str(),date.day(),date.month(),date.year());

    QFile trend(filename);

   // buffer_position=(paint_trend_info->endHour*60*60+paint_trend_info->endMinute*60+paint_trend_info->endSecond-
   //     paint_trend_info->startHour*60*60-paint_trend_info->startMinute*60-paint_trend_info->startSecond)/5;

    if (!trend.open(QIODevice::ReadOnly))
    {
        //for (int j=0;j<17280;j++) tmp_vect[j]=min_float;
        tmp_vect.fill(qQNaN());
    }
    else
    {
        if (trend.read((char *)file_buff,17280*4))
        {
            for (int j=0;j<17280;j++)
            {
                if (file_buff[j]!=min_float)
                {
                    tmp_vect[j]=file_buff[j];
                }
                else
                {
                    tmp_vect[j]=qQNaN();
                }
            }
        }
        else
        {
            //for (int j=0;j<17280;j++) tmp_vect[j]=min_float;
            tmp_vect.fill(qQNaN());
        }
        trend.close();
    }
    if (startLoadedDT > QDateTime(date)) startLoadedDT=QDateTime(date);
    *pyData << tmp_vect;
}
//==================================================================================
void MainWindow::TrendAddFromToDay(QString name, QDate date, QTime timeFrom, QTime timeTo, QVector<double> *pyData)
{
    static float file_buff[17280];
    static QVector<double> tmp_vect;//(17280);

    QString filename;

    filename.sprintf("%s%s_%.2u_%.2u_%.4u.trn",sTrendFilesPath.toStdString().c_str(),name.toStdString().c_str(),date.day(),date.month(),date.year());

    uint data_offset=(timeFrom.hour()*60*60 + timeFrom.minute()*60 + timeFrom.second()) / 5;

    //количество 5-секундных срезов, помещающихся в интервал (+1 )
    uint data_length=(timeTo.hour()*60*60 + timeTo.minute()*60 + timeTo.second())/5 - (timeFrom.hour()*60*60 + timeFrom.minute()*60 + timeFrom.second())/5 + 1;

    tmp_vect.resize(data_length);

    QFile trend(filename);

   // buffer_position=(paint_trend_info->endHour*60*60+paint_trend_info->endMinute*60+paint_trend_info->endSecond-
   //     paint_trend_info->startHour*60*60-paint_trend_info->startMinute*60-paint_trend_info->startSecond)/5;

    if (!trend.open(QIODevice::ReadOnly))
    {
        //for (int j=0;j<17280;j++) tmp_vect[j]=min_float;
        tmp_vect.fill(qQNaN());
    }
    else
    {
        if (trend.seek(data_offset*4))
        {
            if (trend.read((char *)file_buff,data_length*4))
            {
                for (uint j=0;j<data_length;j++)
                {
                    if (file_buff[j]!=min_float)
                    {
                        tmp_vect[j]=file_buff[j];
                    }
                    else
                    {
                        tmp_vect[j]=qQNaN();
                    }
                }
            }
            else
            {
                //for (int j=0;j<17280;j++) tmp_vect[j]=min_float;
                tmp_vect.fill(qQNaN());
            }
        }
        else
        {
            //for (int j=0;j<17280;j++) tmp_vect[j]=min_float;
            tmp_vect.fill(qQNaN());
        }
        trend.close();
    }
    if (startLoadedDT > QDateTime(date)) startLoadedDT=QDateTime(date);
    *pyData << tmp_vect;
}
//==================================================================================
void MainWindow::XAxisAddFullDay(QDate date, QVector<double> *pxData)
{
    static QVector<double> tmp_vect(17280);
    QDateTime tmp_dt(date);

    for(int j=0;j<17280;++j)
    {
        tmp_vect[j]=tmp_dt.toTime_t();
        tmp_dt=tmp_dt.addSecs(5);
    }
    *pxData << tmp_vect;
}
//==================================================================================
void MainWindow::XAxisAddFromToDay(QDate date, QTime timeFrom, QTime timeTo, QVector<double> *pxData)
{
    static QVector<double> tmp_vect(17280);
    QDateTime tmp_dt(date,timeFrom);

    uint data_offset=(timeFrom.hour()*60*60 + timeFrom.minute()*60 + timeFrom.second()) / 5;

    //количество 5-секундных срезов, помещающихся в интервал (+1 )
    uint data_length=(timeTo.hour()*60*60 + timeTo.minute()*60 + timeTo.second())/5 - (timeFrom.hour()*60*60 + timeFrom.minute()*60 + timeFrom.second())/5 + 1;

    tmp_vect.resize(data_length);


    for(uint j=0;j<data_length;++j)
    {
        tmp_vect[j]=tmp_dt.toTime_t();
        tmp_dt=tmp_dt.addSecs(5);
    }
    *pxData << tmp_vect;
}

//==================================================================================
void MainWindow::AllTrendsAddFullDay(QDate date)
{
    XAxisAddFullDay(date,&xData);
    foreach(Trend *tr,trends)
    {
        TrendAddFullDay(tr->fileName,date,&(tr->yData));
        tr->graph->setData(xData,tr->yData);
    }
}
//==================================================================================
void MainWindow::AllTrendsAddFromToDay(QDate date, QTime timeFrom, QTime timeTo)
{
    XAxisAddFromToDay(date, timeFrom, timeTo, &xData);

    foreach(Trend *tr,trends)
    {
        TrendAddFromToDay(tr->fileName, date, timeFrom, timeTo, &(tr->yData));
        tr->graph->setData(xData,tr->yData);
    }
}
//==================================================================================

void MainWindow::CheckNeededBackDaysAndLoad(QDate dateFrom)
{
    static QMutex mtx;
    mtx.lock();

    if (startLoadedDT.date() > dateFrom)
    {
        for(QDate dtToLoad=startLoadedDT.date().addDays(-1);dtToLoad>=dateFrom;dtToLoad=dtToLoad.addDays(-1))
        {
            AllTrendsAddFullDay(dtToLoad);
        }

    }

    mtx.unlock();
}
//==================================================================================
void CheckNeededBackDaysAndLoadThread::run()
{
    if (mw->startLoadedDT.date() > m_dateFrom)
    {

        for(QDate dtToLoad=mw->startLoadedDT.date().addDays(-1);dtToLoad>=m_dateFrom;dtToLoad=dtToLoad.addDays(-1))
        {
            mw->AllTrendsAddFullDay(dtToLoad);
            if (CheckThreadStop()) return;

            emit backDayLoad();
        }

    }
}
//==================================================================================
void MainWindow::ThreadLoadDay()
{
    qDebug() << "load1";
    RecalcGridInterval();
    wGraphic->replot();
        qDebug() << "load22222";
}
//==================================================================================
void MainWindow::RecalcGridInterval()
{
    //                                          10m 20m  30m  1h   2h   3h    6h    8h    12h   1d     2d     3d     4d     5d     10d    20d
    QVector<uint> steps={1,2,5,10,30,60,120,300,600,1200,1800,3600,7200,14400,21600,28800,43200,86400, 172800,259200,345600,432000,864000,1728000 };

    uint tickStep=displayedInterval_sec/16;

    //выберем ближайшее большее или равное значение из steps
    for(uint i=0;i<steps.size()-1;++i)
    {
        if (tickStep<=steps[i])
        {
            tickStep=steps[i+1];
            break;
        }
    }

    wGraphic->xAxis->setTickStep(tickStep);
}
//=================================================================================
void MainWindow::ButtonCollapse()
{
    displayedInterval_sec=wGraphic->xAxis->range().upper-wGraphic->xAxis->range().lower;

    //displayedInterval_sec=displayedInterval_sec*2;
    //if ((displayedInterval_sec % 5) != 0) displayedInterval_sec=((displayedInterval_sec/5)+1)*5;  //выравниваем по границе в 5 с.

    QDateTime currDT=QDateTime::currentDateTime();

    if (currDT < QDateTime::fromTime_t(wGraphic->xAxis->range().upper+displayedInterval_sec))
    {

        wGraphic->xAxis->setRange(currDT.toTime_t()-displayedInterval_sec*2,
                                  currDT.toTime_t());
    }
    else
    {
        wGraphic->xAxis->setRange(wGraphic->xAxis->range().lower-displayedInterval_sec/2,
                                  wGraphic->xAxis->range().upper+displayedInterval_sec/2);
    }




    displayedInterval_sec=wGraphic->xAxis->range().upper-wGraphic->xAxis->range().lower;

    if (!useThread)
    {

    CheckNeededBackDaysAndLoad(QDateTime::fromTime_t(wGraphic->xAxis->range().lower).date());
    RecalcGridInterval();
    wGraphic->replot();
    qDebug() << displayedInterval_sec;
    }
    else
    {
        loadThread.SetDateFrom(QDateTime::fromTime_t(wGraphic->xAxis->range().lower).date());
        loadThread.start();
        RecalcGridInterval();
        wGraphic->replot();
    }


    ui->buttonExpand->setEnabled(true);

}
//==================================================================================
void MainWindow::ButtonExpand()
{
    displayedInterval_sec=wGraphic->xAxis->range().upper-wGraphic->xAxis->range().lower;

    //displayedInterval_sec=displayedInterval_sec/2;

    wGraphic->xAxis->setRange(wGraphic->xAxis->range().lower+displayedInterval_sec/4,
                              wGraphic->xAxis->range().upper-displayedInterval_sec/4);

    displayedInterval_sec=wGraphic->xAxis->range().upper-wGraphic->xAxis->range().lower;

    RecalcGridInterval();
    wGraphic->replot();

    if (displayedInterval_sec<60) ui->buttonExpand->setEnabled(false);

    qDebug() << displayedInterval_sec;


    ui->buttonCollapse->setEnabled(true);
}
//==================================================================================
void MainWindow::Button100Forward()
{
    displayedInterval_sec=wGraphic->xAxis->range().upper-wGraphic->xAxis->range().lower;
    QDateTime currDT=QDateTime::currentDateTime();

    if (currDT < QDateTime::fromTime_t(wGraphic->xAxis->range().upper+displayedInterval_sec))
    {

        wGraphic->xAxis->setRange(currDT.toTime_t()-displayedInterval_sec,
                                  currDT.toTime_t());
    }
    else
    {
        wGraphic->xAxis->setRange(wGraphic->xAxis->range().lower+displayedInterval_sec,
                                  wGraphic->xAxis->range().upper+displayedInterval_sec);
    }

    wGraphic->replot();
}
//==================================================================================
void MainWindow::Button100Back()
{
    displayedInterval_sec=wGraphic->xAxis->range().upper-wGraphic->xAxis->range().lower;

    wGraphic->xAxis->setRange(wGraphic->xAxis->range().lower-displayedInterval_sec,
                              wGraphic->xAxis->range().upper-displayedInterval_sec);


    if (!useThread)
    {
        CheckNeededBackDaysAndLoad(QDateTime::fromTime_t(wGraphic->xAxis->range().lower).date());
    }
    else
    {
        loadThread.SetDateFrom(QDateTime::fromTime_t(wGraphic->xAxis->range().lower).date());
        loadThread.start();
    }

    wGraphic->replot();
}
//==================================================================================
void MainWindow::Button50Forward()
{
    displayedInterval_sec=wGraphic->xAxis->range().upper-wGraphic->xAxis->range().lower;
    QDateTime currDT=QDateTime::currentDateTime();

    if (currDT < QDateTime::fromTime_t(wGraphic->xAxis->range().upper+displayedInterval_sec/2))
    {

        wGraphic->xAxis->setRange(currDT.toTime_t()-displayedInterval_sec,
                                  currDT.toTime_t());
    }
    else
    {
        wGraphic->xAxis->setRange(wGraphic->xAxis->range().lower+displayedInterval_sec/2,
                                  wGraphic->xAxis->range().upper+displayedInterval_sec/2);
    }

    wGraphic->replot();
}
//==================================================================================
void MainWindow::Button50Back()
{
    displayedInterval_sec=wGraphic->xAxis->range().upper-wGraphic->xAxis->range().lower;
    wGraphic->xAxis->setRange(wGraphic->xAxis->range().lower-displayedInterval_sec/2,
                              wGraphic->xAxis->range().upper-displayedInterval_sec/2);


    if (!useThread)
    {
        CheckNeededBackDaysAndLoad(QDateTime::fromTime_t(wGraphic->xAxis->range().lower).date());
    }
    else
    {
        loadThread.SetDateFrom(QDateTime::fromTime_t(wGraphic->xAxis->range().lower).date());
        loadThread.start();
    }

    wGraphic->replot();
}
//==================================================================================
void MainWindow::ComboBoxThemeChanged(int newThemeIndex)
{
    static QPalette standartPalette=qApp->palette();

    //"windows", "motif", "cde", "plastique" and "cleanlooks" "fusion"

    //QStyle *style = new QProxyStyle(QStyleFactory::create("plastique"));
    //a.setStyle(style);



    // dark fusion
    qApp->setStyle(QStyleFactory::create("fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(15,15,15));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);

    darkPalette.setColor(QPalette::Highlight, QColor(142,45,197).lighter());
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);


    QPalette grayPalette;
    grayPalette.setColor(QPalette::Window, Qt::gray);
    grayPalette.setColor(QPalette::WindowText, Qt::black);
    grayPalette.setColor(QPalette::Base, Qt::gray);
    grayPalette.setColor(QPalette::AlternateBase, Qt::black);
    grayPalette.setColor(QPalette::ToolTipBase, Qt::gray);
    grayPalette.setColor(QPalette::ToolTipText, Qt::black);
    grayPalette.setColor(QPalette::Text, Qt::black);
    grayPalette.setColor(QPalette::Button, Qt::gray);
    grayPalette.setColor(QPalette::ButtonText, Qt::black);
    grayPalette.setColor(QPalette::BrightText, Qt::red);

    grayPalette.setColor(QPalette::Highlight, QColor(142,45,197).lighter());
    grayPalette.setColor(QPalette::HighlightedText, Qt::black);
    grayPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    grayPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);


    switch (newThemeIndex)
    {
    case 0:  //black
        wGraphic->setBackground(QBrush(Qt::black));  //- to black theme
        wGraphic->xAxis->setBasePen(QPen(Qt::white));
        wGraphic->xAxis->setTickPen(QPen(Qt::white));
        wGraphic->xAxis->setSubTickPen(QPen(Qt::white));
        wGraphic->xAxis->setTickLabelColor(Qt::white);
        wGraphic->yAxis->setBasePen(QPen(Qt::white));
        wGraphic->yAxis->setTickPen(QPen(Qt::white));
        wGraphic->yAxis->setSubTickPen(QPen(Qt::white));
        wGraphic->yAxis2->setBasePen(QPen(Qt::white));
        wGraphic->yAxis2->setTickPen(QPen(Qt::white));
        wGraphic->yAxis2->setSubTickPen(QPen(Qt::white));
        vizirLine->setPen(QPen(Qt::white));
        /*   - изменение ширины пера в момент загрузки данных другим потоком приводит к зависанию
        foreach (Trend *tr, trends)
        {
            QPen pen(tr->graph->pen());
            pen.setWidth(1);
            tr->graph->setPen(pen);
        }
        */
        qApp->setPalette(darkPalette);
        break;
    case 1:  //white
        wGraphic->setBackground(QBrush(Qt::white));  //- to white theme
        wGraphic->xAxis->setBasePen(QPen(Qt::black));
        wGraphic->xAxis->setTickPen(QPen(Qt::black));
        wGraphic->xAxis->setSubTickPen(QPen(Qt::black));
        wGraphic->xAxis->setTickLabelColor(Qt::black);
        wGraphic->yAxis->setBasePen(QPen(Qt::black));
        wGraphic->yAxis->setTickPen(QPen(Qt::black));
        wGraphic->yAxis->setSubTickPen(QPen(Qt::black));
        wGraphic->yAxis2->setBasePen(QPen(Qt::black));
        wGraphic->yAxis2->setTickPen(QPen(Qt::black));
        wGraphic->yAxis2->setSubTickPen(QPen(Qt::black));
        vizirLine->setPen(QPen(QColor(100,100,100)));  //-white theme, gray vizir
        /*
        foreach (Trend *tr, trends)
        {
            QPen pen(tr->graph->pen());
            pen.setWidth(2);
            tr->graph->setPen(pen);
        }
        */
        qApp->setPalette(standartPalette);
        break;
    case 2:  //Gray
        wGraphic->setBackground(QBrush(Qt::gray));  //- to gray theme
        wGraphic->xAxis->setBasePen(QPen(Qt::black));
        wGraphic->xAxis->setTickPen(QPen(Qt::black));
        wGraphic->xAxis->setSubTickPen(QPen(Qt::black));
        wGraphic->xAxis->setTickLabelColor(Qt::black);
        wGraphic->yAxis->setBasePen(QPen(Qt::black));
        wGraphic->yAxis->setTickPen(QPen(Qt::black));
        wGraphic->yAxis->setSubTickPen(QPen(Qt::black));
        wGraphic->yAxis2->setBasePen(QPen(Qt::black));
        wGraphic->yAxis2->setTickPen(QPen(Qt::black));
        wGraphic->yAxis2->setSubTickPen(QPen(Qt::black));
        vizirLine->setPen(QPen(Qt::black));
        /*
        foreach (Trend *tr, trends)
        {
            QPen pen(tr->graph->pen());
            pen.setWidth(2);
            tr->graph->setPen(pen);
        }
        */
        qApp->setPalette(grayPalette);

        break;
    }

    if (!loadThread.isRunning())  wGraphic->replot();

}
//==================================================================================
