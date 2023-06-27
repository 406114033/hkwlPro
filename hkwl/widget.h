#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include<QSerialPort>    //提供访问串口的功能
#include<QSerialPortInfo>    //提供系统中存在的串口的信息
#include<QSqlQuery>
#include<QSqlTableModel>
#include<QChart>
#include<QChartView>
#include<QtCharts>
using namespace QtCharts;
#include<QValueAxis>
#include<QDateTime>
#include<QDateTimeAxis>
#include"QDateTime"
#include<QTimer>


namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

    void paintEvent(QPaintEvent *);  //重写绘图时间
    void initDraw();  //初始化画布


private slots:
    void on_btnOpen_clicked();

    void on_btnSend_clicked();

    void on_btnClear_clicked();

    void Drawline();   //画线



    void on_clear_clicked();

private:
    Ui::Widget *ui;
    //创建串口对象
    QSerialPort serial;
    QSqlQuery query;

    QSqlTableModel *model;

    QChart *m_chart;  //画笔
    QChartView *m_chartview;  //画布
    QTimer *timer;   //计时器
    QSplineSeries *series1;    //温度曲线
    QSplineSeries * series2;   //湿度曲线
    QDateTimeAxis *axisX;   //轴

    QValueAxis *axisY_1;
    QValueAxis *axisY_2;

    uint temp;  //温度数据
    uint hum;   //湿度数据
    uint smoke;   //烟雾数据

};

#endif // WIDGET_H
