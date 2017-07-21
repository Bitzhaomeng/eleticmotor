#include "dialog.h"
#include "ui_dialog.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QComboBox>
#include<QDebug>



Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    handle_PX = false;
    timer1= new QTimer(this);
    connect(timer1,SIGNAL(timeout()),this,SLOT(DemandPX()));
    connect(&thread_port,SIGNAL(response(QString)),this,SLOT(receive_response(QString)));
    connect(&thread_port,SIGNAL(S_PortNotOpen()),this,SLOT(portError_OR_timeout()));
    connect(&thread_port,SIGNAL(timeout()),this,SLOT(portError_OR_timeout()));
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_searchButton_clicked()
{
    portname.clear();
    QSerialPort my_serial;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        my_serial.close();
        my_serial.setPortName(info.portName());
        if(!my_serial.open(QIODevice::ReadWrite))
            return;
        my_serial.setBaudRate(QSerialPort::Baud19200);
        my_serial.setDataBits(QSerialPort::Data8);
        my_serial.setStopBits(QSerialPort::OneStop);
        my_serial.setFlowControl(QSerialPort::NoFlowControl);

        QString test("VR;");
        QByteArray testData = test.toLocal8Bit();
        my_serial.write(testData);
        if(my_serial.waitForBytesWritten(15))
        {
            if(my_serial.waitForReadyRead(30))
            {
                QByteArray testResponse = my_serial.readAll();
                while(my_serial.waitForReadyRead(10))
                    testResponse += my_serial.readAll();
                QString response(testResponse);
                if(response.left(10) == "VR;Whistle")
                {
                    portname = info.portName();
                    ui->port->setText(portname);
                    ui->state->setText("电机连接");
                    break;
                }
            }
        }
    }
    my_serial.close();

    if(portname.left(3) == "COM")
    {
        Motor_Connected = true;								//电机连接
        OpenMotor();										//打开电机
    }
    else
    {
        Motor_Connected = false;							//电机未连接
        ui->state->setText("串口连接失败");
        ui->searchButton->setEnabled(true);	//未检测到串口，按钮设为使能状态
    }

//    emit this->Motot_connect_status(Motor_Connected);		//主界面状态栏显示电机连接状态

}

void Dialog::OpenMotor()
{
    Order_str = "MO=1;";
    thread_port.transaction(portname,Order_str);
}


void Dialog::on_moveButton_clicked()
{
    Order_str = "BG;";
    thread_port.transaction(portname,Order_str);
    timer1->start(100);
    //打开定时器timer1
    handle_PX=false;                 //???
}

void Dialog::on_defaultsetting_clicked()
{
    defaultset=true;
    ui->speed->setText(QString::number(90));		//速度
    ui->acceleratedseed->setText("180");						//加速度
    ui->decelerationspeed->setText("180");						//减速度
    ui->direction->setCurrentIndex(1);
    ui->lineEdit_PR->setText("0");							//相对距离
//    ui->lineEdit_PA->setText("0");							//绝对距离
    Order_str = "AC=1800;";
    thread_port.transaction(portname,Order_str);
}
void Dialog::receive_response(const QString &s)
{
    QString res = s;
    qDebug()<<"RES="<<s;
    if(defaultset)
    {
      if(s.left(2) == "AC")
      {
          Order_str = "DC=1800;";
          thread_port.transaction(portname,Order_str);
      }
      else
      {
          if(s.left(2) == "DC")
          {
              Order_str = "SP=1310000;";
              thread_port.transaction(portname,Order_str);
              defaultset=false;
          }

      }

    }


    if(s.left(2) == "MS")								//获取当前位置值
    {
         qDebug()<<"MS1";
        if(s.left(4) == "MS;0")
         {
             timer1->stop();
             ui->state->setText("电机到达预定位置");
             ui->lineEdit_PR->setEnabled(true);
             ui->lineEdit_PA->setEnabled(true);
             handle_PX=false;
         }

         if(s.left(4) == "MS;3")
            {
             timer1->stop();
             ui->state->setText("电机启动失败");
             handle_PX=false;
            }
         else
            {
             Order_str = "PX;";
             thread_port.transaction(portname,Order_str);
//             ui->state->setText("电机运转");
             qDebug()<<"PX1";
            }
    }
    if(s.left(2) == "PX")
    {
         QString ret = res.split(";").at(1).toLocal8Bit().data();	//PX值
         if(check_PX)
         {
            PX_Origin=ret;
            check_PX=false;
         }
         float positionData = (ret.toFloat()-PX_Origin.toFloat())*360/MOTOR_RESOLUTION;
         if(positionData<0)
         {
//             positionData=-1*positionData;
             while(positionData<0)
                 positionData = positionData + 360;
         }
         while(positionData>360)
             positionData = positionData - 360;
         QString str = QString::number(360-positionData,'f',2)+ "°";
         ui->position->setText(str);
//         ui->position->setText(QString::number(positionData));
         handle_PX=false;
         qDebug()<<"read";
    }
    if(s.left(2) == "MO")
    {
//        qDebug()<<"MO";
        if(s.left(4) == "MO=0")
        {
           ui->state->setText("电机关闭");
           PX_Origin="";
//           qDebug()<<"MO=0";
        }
        else
        {
           ui->state->setText("电机打开");
           check_PX=false;

//           qDebug()<<"MO=1";
        }
    }
//    else
//    {
//       ui->state->setText("电机运转指令发出");
//    }
}

void Dialog::on_speed_textChanged(const QString &arg1)
{
    SP=QString::number((arg1.toInt()*MOTOR_RESOLUTION/360));
    Order_str = "SP="+SP+";";
    thread_port.transaction(portname,Order_str);
}

void Dialog::on_lineEdit_PA_textChanged(const QString &arg1)
{
    PA=QString::number((arg1.toInt()*MOTOR_RESOLUTION/360));
    Order_str = "PA="+PA+";";
    thread_port.transaction(portname,Order_str);
    ui->lineEdit_PR->setEnabled(false);
    qDebug()<<"PA="<<arg1.toInt();
}

void Dialog::on_lineEdit_PR_textChanged(const QString &arg1)
{
    PR=QString::number((arg1.toInt()*MOTOR_RESOLUTION/360));
    if(!direction)
    {
        Order_str = "PR=-"+PR+";";
        qDebug()<<"false ok";
    }

    else
    {
       Order_str = "PR="+PR+";";
    }
    thread_port.transaction(portname,Order_str);
    ui->lineEdit_PA->setEnabled(false);
}


void Dialog::on_direction_activated(int index)
{
    if(index==0)
     {
        direction=false;
        Order_str = "PR=-"+PR+";";
       qDebug()<<"false";
    }
    else
    {
        direction=true;
        Order_str = "PR="+PR+";";
         qDebug()<<"true";
    }
    thread_port.transaction(portname,Order_str);
}

void Dialog::DemandPX()
{
//    qDebug()<<"demand="<<handle_PX;
    if(handle_PX == false)
    {
        handle_PX = true;
        Order_str = "MS;";
        thread_port.transaction(portname,Order_str);
        ui->state->setText("电机运转");
        qDebug()<<"demand ok";
    }
}

void Dialog::on_closeButton_clicked()
{
    Order_str = "MO=0;";
    thread_port.transaction(portname,Order_str);
//    ui->state->setText("电机关闭");
}

void Dialog::on_checkButton_clicked()
{
    Order_str = "PX;";
    thread_port.transaction(portname,Order_str);
}

void Dialog::portError_OR_timeout()
{
     timer1->stop();
     ui->state->setText("串口未打开");
}

void Dialog::on_openButton_clicked()
{
    Order_str = "MO=1;";
    thread_port.transaction(portname,Order_str);
//    ui->state->setText("电机打开");
}

void Dialog::on_setPX0Button_clicked()
{
    Order_str = "PX;";
    thread_port.transaction(portname,Order_str);
    ui->position->setText("0");
    check_PX=true;
}
