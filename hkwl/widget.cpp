#include "widget.h"
#include "ui_widget.h"
#include<QSerialPort>    //提供访问串口的功能
#include<QSerialPortInfo>    //提供系统中存在的串口的信息
#include<QMessageBox>
#include<QDebug>
#include<QSqlError>
#include<QSqlQuery>
#include<QVariantList>
#include<QSqlDatabase>
#include<QPainter>
#include<QColor>
#include<QPixmap>
#include<QtCharts>
using namespace QtCharts;
#include<QAbstractSeries>
#include<QSplineSeries>
#include<QDateTime>
#include<QDateTimeAxis>
#include<QTime>
#include<QTimer>


Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/iconfinder_weather_43_2682808.png"));
//    ui->tableView->setStyleSheet("background-image:url(:/res/Frame.jpg)");
   //setFixedSize(850,600);
    //ui->tabWidget->setFixedSize(700,700);
    //打印Qt支持的数据库驱动
  // qDebug()<<QSqlDatabase::drivers();

    //利用Qlabel显示图片
    ui->portlabel->setPixmap(QPixmap(":/open.bmp"));
    ui->wenshiLabel->setPixmap(QPixmap(":/iconfinder_Artboard_10_6045706.png"));
    ui->wenshiLabel->setScaledContents(true);

    ui->yanwuLabel->setPixmap(QPixmap(":/gas_normal.bmp"));
    ui->yanwuLabel->setScaledContents(true);
    //检测串口
    ui->PortBox->clear();
    //通过QSerialportInfo查找可用串口
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
      ui->PortBox->addItem(info.portName());
    }
    //添加SqlLite数据库
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    //设置数据库
    db.setDatabaseName("../fire.db");


    //打开数据库
    if(!db.open())    //数据库打开失败
    {
        qDebug()<<"连接失败";
        QMessageBox::warning(this,"错误",db.lastError().text());
        return;

    }
    else
    {
        qDebug()<<"连接成功";
    }
   //当串口的接收缓冲区有数据时，QSerilaPort对象会发出一个readyRead()的信号 连接信号
    connect(&serial,&QSerialPort::readyRead,this,[=](){
        //从接收缓冲区中读取数据
        QByteArray buffer = serial.readAll();
        QString wendu = QString(buffer.mid(0,2));
        QString shidu = QString(buffer.mid(4,2));
        QString yanwu = QString(buffer.mid(8,1));

        //温度曲线图数据
        temp = wendu.toInt();
        //湿度曲线图数据
        hum = shidu.toInt();

        smoke = yanwu.toInt();

        //设置当前信息中的数据
        ui->wendu->setText(wendu);
        ui->shidu->setText(shidu);


        if(yanwu.toInt()==1)
        {
            ui->yanwuLabel->setPixmap(QPixmap(":/gas_normal.bmp"));
            ui->yanwuLabel->setScaledContents(true);

        }
        else {
           ui->yanwuLabel->setPixmap(QPixmap(":/gas_abnormal.bmp"));
           ui->yanwuLabel->setScaledContents(true);

           QDateTime current_date_time = QDateTime::currentDateTime();
           QString current_date = current_date_time.toString("yyyy-MM-dd");
           QString current_time = current_date_time.toString("hh:mm:ss");

           QString rec = QString("气体异常,%1,%2").arg(current_date).arg(current_time);
           ui->errorEdit->append(rec);
          // ui->errorEdit->append("气体异常");
        }

        query = QSqlQuery(db);
        //创建表
        query.exec("create table fire(id int primary key, wen int, shi int, yan int)");
//        qDebug()<<wendu<<shidu<<yanwu;
        query.prepare("insert into fire(wen,shi,yan) values(?,?,?)");

        //给字段设置内容list
        QVariantList wenlist;
        wenlist <<temp;
        QVariantList shiList;
        shiList<<hum;
        QVariantList yanlist;
        yanlist<<smoke;

        query.addBindValue(wenlist);
        query.addBindValue(shiList);
        query.addBindValue(yanlist);

    //    query.exec("update fire set wen='"+wendu+"' where shi='"+shidu+"'and yan='"+yanwu+"'");

        //执行预处理命令
        query.execBatch();

        //从界面中读取以前收到的数据
        QString recv = ui->writeEdit->toPlainText();
        recv += QString(buffer);

        //清空以前的显示
        ui->writeEdit->clear();
        //重新显示
        ui->writeEdit->append(recv);
    });
    //发送按键失能
    ui->btnSend->setEnabled(false);
    //波特率默认选择下拉第一项 ：9600
    ui->BaudBox->setCurrentIndex(0);
    ui->DataBox->setCurrentIndex(3);

    //设置模型
    model = new QSqlTableModel(this);
    model->setTable("fire");

    ui->tableView->setModel(model);
    //显示model里的数据
    model->select();
    //修改列表名
   // model->setHeaderData(0,Qt::Horizontal,"温度");

    //设置model的编辑模式 ，手动提交修改
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    //设置view中的数据库不允许修改
    //ui->tableView->setEditTriggers(QAbstractItemView::NoEditTrigger);


    initDraw();   //画布
    timer = new QTimer(this);   //创建定时器
    connect(timer,&QTimer::timeout,this,[=](){
       Drawline();
    });
}

Widget::~Widget()
{
    delete ui;
}


//打开串口按钮的槽函数
void Widget::on_btnOpen_clicked()
{
    if(ui->btnOpen->text() == QString("打开串口"))
    {
        timer->start();
        timer->setInterval(3000);  //设置定时周期

        //设置串口名
        serial.setPortName(ui->PortBox->currentText());
        //设置波特率
        serial.setBaudRate(ui->BaudBox->currentText().toInt());
        //设置数据位数
        switch (ui->DataBox->currentIndex()) {
        case 8:
           serial.setDataBits(QSerialPort::Data8);
            break;
        default:
            break;
        }
        //设置校验
        switch (ui->ParityBox->currentIndex()) {
        case 0:
            serial.setParity(QSerialPort::NoParity);
            break;
        default:
            break;
        }
        //设置停止位
        switch (ui->StopBox->currentIndex()) {
        case 1:
           serial.setStopBits(QSerialPort::OneStop);
            break;
        case 2:
            serial.setStopBits(QSerialPort::TwoStop);
            break;
        default:
            break;
        }

        //打开串口
        if(!serial.open(QIODevice::ReadWrite))
        {
         QMessageBox::about(NULL,"提示","无法打开串口");
         return;
        }
        //下拉菜单控件失能
        ui->PortBox->setEnabled(false);
        ui->BaudBox->setEnabled(false);
        ui->DataBox->setEnabled(false);
        ui->ParityBox->setEnabled(false);
        ui->StopBox->setEnabled(false);
        ui->btnOpen->setText(QString("关闭串口"));
        ui->portlabel->setPixmap(QPixmap(":/close.bmp"));

        //发送按键使能
        ui->btnSend->setEnabled(true);

    }
    else
    {
       //关闭串口
        serial.close();
        //下拉菜单控件使能
        ui->PortBox->setEnabled(true);
        ui->BaudBox->setEnabled(true);
        ui->DataBox->setEnabled(true);
        ui->ParityBox->setEnabled(true);
        ui->StopBox->setEnabled(true);
        ui->btnOpen->setText(QString("打开串口"));
        //利用Qlabel显示图片
        ui->portlabel->setPixmap(QPixmap(":/open.bmp"));
        //发送按键使能
        ui->btnSend->setEnabled(false);

    }


}
//重写paintEvent事件 画背景图
void Widget::paintEvent(QPaintEvent *)
{
      QPainter painter(this);

      QPixmap pix;
      pix.load(":/res/Frame.jpg");

      painter.drawPixmap(0,0,this->width(),this->height(),pix);


}

void Widget::on_btnSend_clicked()
{
    //获取界面上的数据并转换成utf-8格式的字节流
    QByteArray data = ui->sendEdit->toPlainText().toUtf8();
    serial.write(data);
}

//清空按钮
void Widget::on_btnClear_clicked()
{
    ui->writeEdit->clear();
}

//初始化画布
void Widget::initDraw()
{
    QPen penY1(Qt::darkBlue,3,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
    QPen penY2(Qt::darkGreen,3,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
    //绘制动态曲线
    m_chart = new QChart();
    m_chartview = new QChartView(m_chart,ui->w_widget); //画布
    m_chart = m_chartview->chart();  //画笔
    series1 = new QSplineSeries;
    series2 = new QSplineSeries;

    m_chartview->setRubberBand(QChartView::NoRubberBand); //矩形缩放
    m_chartview->setRenderHint(QPainter::Antialiasing);   //设置抗锯齿

    m_chartview->resize(720,500);
    m_chartview->setContentsMargins(0,0,0,0);
    m_chartview->show();

    //m_chart->legend()->hide();  //隐藏图例
    m_chart->addSeries(series1);   //把线添加到chart
    m_chart->addSeries(series2);
    //设置X坐标轴

    axisX = new QDateTimeAxis;
    axisX->setFormat("hh:mm:ss");   //设置时间显示格式
    axisX->setGridLineVisible(true);   //网格
    axisX->setTickCount(8);   //设置坐标轴格数
    axisX->setTitleText("实时时间");  //标题
    axisX->setLinePen(penY1);

    //设置Y坐标轴
    axisY_1 = new QValueAxis;
    axisY_1->setRange(0,40);   //范围
    axisY_1->setLabelFormat("%d");
    axisY_1->setLinePenColor(QColor(Qt::darkBlue));  //设置坐标轴颜色样式
    axisY_1->setGridLineColor(QColor(Qt::darkBlue));
    axisY_1->setGridLineVisible(false);  //设置Y轴网格不显示
    axisY_1->setTickCount(10);  //轴上有多个标记项目
    axisY_1->setMinorTickCount(5);  //主要刻度之间有多少网格线
    axisY_1->setTitleText("温度/(℃)");
    axisY_1->setLinePen(penY1);

    axisY_2 = new QValueAxis;
    axisY_2->setRange(0,100);   //范围
    axisY_2->setLabelFormat("%d");
    axisY_2->setLinePenColor(QColor(Qt::darkGreen));  //设置坐标轴颜色样式
    axisY_2->setGridLineColor(QColor(Qt::darkGreen));
    axisY_2->setGridLineVisible(false);  //设置Y轴网格不显示
    axisY_2->setTickCount(10);  //轴上有多个标记项目
    axisY_2->setMinorTickCount(5);  //主要刻度之间有多少网格线
    axisY_2->setTitleText("湿度/(%))");
    axisY_2->setLinePen(penY2);



    axisX->setMin(QDateTime::currentDateTime().addSecs(-60*1));
    axisX->setMax(QDateTime::currentDateTime().addSecs(0));


    m_chart->addAxis(axisX,Qt::AlignBottom);  //将坐标轴加到chart上  居下
    m_chart->addAxis(axisY_1,Qt::AlignLeft);  //居左
    m_chart->addAxis(axisY_2,Qt::AlignRight);  //居右

    series1->attachAxis(axisX);   //把数据添加到坐标轴上
    series1->attachAxis(axisY_1);

    series2->attachAxis(axisX);   //把数据添加到坐标轴上
    series2->attachAxis(axisY_2);

    series1->setName("温度");
    series2->setName("湿度");

    //m_chart->setTitle("温湿度曲线");
    //m_chart->legend()->setVisible(true);  //设置图例可见
    m_chart->legend()->setLayoutDirection(Qt::LeftToRight);
    m_chart->legend()->setAlignment(Qt::AlignTop);


}
void Widget::Drawline()
{
     QDateTime currentTime = QDateTime::currentDateTime();

     //设置坐标轴显示范围
     m_chart->axisX()->setMin(QDateTime::currentDateTime().addSecs(-60*1));  //系统当前时间的前一秒
     m_chart->axisX()->setMax(QDateTime::currentDateTime().addSecs(0));     //系统当前时间
     //生成随机数做测试
     qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

     //增加新的点到曲线末端
      series1->append(currentTime.toMSecsSinceEpoch(),temp);
      //增加新的点到曲线末端
      series2->append(currentTime.toMSecsSinceEpoch(),hum);
}
//异常记录清空
void Widget::on_clear_clicked()
{
    ui->errorEdit->clear();
}
