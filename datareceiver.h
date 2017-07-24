#ifndef DATARECEIVER_H
#define DATARECEIVER_H

#include <QObject>
#include <QDateTime>

class DataReceiver : public QObject
{
    Q_OBJECT
public:
    explicit DataReceiver(QObject *parent = 0);
    void SetStartDT(QDateTime startDT);
    void SetEndDT(QDateTime endDT);


    QDateTime startLoadedDT; //диапазон загруженных данных
    QDateTime endLoadedDT;

signals:

public slots:

};

#endif // DATARECEIVER_H
