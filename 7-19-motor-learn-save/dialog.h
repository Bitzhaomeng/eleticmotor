#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include "serialportthread.h"
#include<QTimer>


const int MOTOR_RESOLUTION = 524000;			//电机分辨率
namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();
    void OpenMotor();

private slots:
    void on_searchButton_clicked();

    void on_moveButton_clicked();

    void on_defaultsetting_clicked();
    void receive_response(const QString &s);

    void on_speed_textChanged(const QString &arg1);

    void on_lineEdit_PA_textChanged(const QString &arg1);

    void on_lineEdit_PR_textChanged(const QString &arg1);

    void on_direction_activated(int index);
    void DemandPX();

    void on_closeButton_clicked();

    void on_checkButton_clicked();
    void portError_OR_timeout();

    void on_openButton_clicked();

    void on_setPX0Button_clicked();

private:
    Ui::Dialog *ui;
    QString portname,Order_str,AC,DC,PR,PA,SP,PX_Origin;
    SerialPortThread thread_port;
    bool Motor_Connected,defaultset,direction,handle_PX,check_PX;
    QTimer *timer1;
};

#endif // DIALOG_H
