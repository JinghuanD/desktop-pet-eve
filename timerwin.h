#ifndef TimerWin_H
#define TimerWin_H

#include <QDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QBitmap>
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>



//namespace Ui {
//class TimerWin;
//}

class TimerWin : public QDialog
{
    Q_OBJECT

public:
    explicit TimerWin(QWidget *parent = nullptr);
    ~TimerWin();
    void init_btn();


private slots:
    void yesbutton_clicked();

    void nobutton_clicked();

    void closebutton_clicked();


signals:
    void send_data(int data);

private:
   // Ui::TimerWin *ui;
    QRadioButton *yesbutton; // Confirmation button.
    QRadioButton *nobutton;
    QPushButton *closebutton;
    QDialog *drinkwindow;
    QSpinBox *spinBox;
    int spinnum;

};

#endif // TimerWin_H
