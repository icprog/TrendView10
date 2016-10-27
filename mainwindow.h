#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qcustomplot.h>
#include <QDebug>
#include <QVector>
#include <QDate>

#include "autostopthread.h"


namespace Ui {
class MainWindow;
}

class MainWindow;

//class CheckNeededBackDaysAndLoadThread;


class Trend : public QObject
{
    Q_OBJECT
public:
    explicit Trend(QCustomPlot *customPlot)
    {
        plot=customPlot;
        connect(&checkBox,SIGNAL(toggled(bool)),this,SLOT(checkBoxChanged(bool)));
    }

    virtual ~Trend()
    {
        yData.clear();
        delete yAxis;
        delete tracer;
    }

    QCustomPlot *plot;

    QString tagName;
    QString fileName;
    float min;
    float max;
    QString desc;
    QCPAxis *yAxis;
    QCPGraph *graph;
    QCPItemTracer *tracer;      // Трасировщик по точкам графика
    QVector<double> yData;

    QColor color;
    QLabel labelColorBox;
    QCheckBox checkBox;
    QLabel labelName;
    QLineEdit lineEditValue;
    QLineEdit lineEditScale;

    QLabel labelColorBoxMax;
    /*QLineEdit*/ QDoubleSpinBox lineEditScaleMax;
    QLabel labelColorBoxMin;
    /*QLineEdit*/ QDoubleSpinBox lineEditScaleMin;


    QLabel labelDesc;

public slots:
    void checkBoxChanged(bool newValue){graph->setVisible(newValue);plot->replot();}

};

class CheckNeededBackDaysAndLoadThread : public AutoStopThread
{
    Q_OBJECT
public:
    CheckNeededBackDaysAndLoadThread(){}
    virtual ~CheckNeededBackDaysAndLoadThread(){}
private:
    QDate m_dateFrom;
    QMutex m_mtx;
public:
    MainWindow *mw;
    virtual void run();

    void SetDateFrom(QDate dateFrom)
    {
        m_mtx.lock();
        m_dateFrom=dateFrom;
        m_mtx.unlock();
    }

signals:
    void backDayLoad();


};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void RecalcGridInterval();
    void OnlineRepaint();

//private:
    Ui::MainWindow *ui;

    QString sTitle;
    QString sTrendFilesPath;

    QCustomPlot *wGraphic;      // Объявляем объект QCustomPlot
    QCPCurve *vizirLine;     // Объявляем объект для вертикальной линии
    QCPItemText *vizirLabel;
    //double mouse_coordX_prev;
    QVector<double> xData;

    QDateTime startLoadedDT; //диапазон загруженных данных
    QDateTime endLoadedDT;

    QDateTime startDisplayedDT; //диапазон показываемых данных
    QDateTime endDisplayedDT;

    uint displayedInterval_sec;

    bool onlineTrend;

    double coordX_prev;

    QVector<Trend *> trends;

    CheckNeededBackDaysAndLoadThread loadThread;
    bool useThread;  //if this set to true - using thread to load data

    void TrendAddFullDay(QString name, QDate date, QVector<double> *pyData);
    void TrendAddFromToDay(QString name, QDate date, QTime timeFrom, QTime timeTo, QVector<double> *pyData);

    void XAxisAddFullDay(QDate date, QVector<double> *pxData);
    void XAxisAddFromToDay(QDate date, QTime timeFrom, QTime timeTo, QVector<double> *pxData);

    void AllTrendsAddFullDay(QDate date);
    void AllTrendsAddFromToDay(QDate date, QTime timeFrom, QTime timeTo);

    void CheckNeededBackDaysAndLoad(QDate dateFrom);


private slots:
    void slotMousePress(QMouseEvent * event);
    void slotMouseMove(QMouseEvent * event);


    //void TrendClicked();
    void ButtonPrint();
    void ButtonSaveToFile();
    void ButtonCollapse();
    void ButtonExpand();

    void Button100Forward();
    void Button100Back();
    void Button50Forward();
    void Button50Back();
    void StartEndDateTimeChanged();
    void GraphicXAxisRangeChanged(QCPRange newRange);

    void ComboBoxThemeChanged(int newThemeIndex);

    void ThreadLoadDay();

};




#endif // MAINWINDOW_H
