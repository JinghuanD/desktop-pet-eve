#include "timerwin.h"
#include "ui_timerwin.h"

TimerWin::TimerWin(QWidget *parent) :
    QDialog(parent)
 {
    setWindowFlags(Qt::FramelessWindowHint|Qt::Tool); // Hide the window frame.

    QBitmap bmp(this->size()); // Create a rounded window mask.
    bmp.fill();
    QPainter p(&bmp);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawRoundedRect(bmp.rect(),30,30);
    setMask(bmp);
    setWindowOpacity(0.95); // Set window opacity.
    setStyleSheet("background-color:#ccccff;color:white;font: bold 12px;");

    Qt::WindowFlags m_flags = windowFlags(); // Preserve window flags.
    setWindowFlags(m_flags|Qt::WindowStaysOnTopHint); // Keep the window on top.


    QLabel* titleLabel = new QLabel("Little Witch Reminder", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleLabel->setFont(titleFont);
    titleLabel->resize(400, 30);
    titleLabel->move(0, 20);

    // Reminder text.
    QLabel* messageLabel = new QLabel("Your scheduled time is up!", this);
    messageLabel->setAlignment(Qt::AlignCenter);
    QFont msgFont = messageLabel->font();
    msgFont.setPointSize(12);
    messageLabel->setFont(msgFont);
    messageLabel->resize(400, 30);
    messageLabel->move(0, 60);


    init_btn();
}

TimerWin::~TimerWin()
{
   // delete ui;

}
void TimerWin::init_btn()
{

    yesbutton = new QRadioButton("I will do it now", this);
    yesbutton->move(40, 110);

    nobutton = new QRadioButton("I need a little more time", this);
    nobutton->move(40, 150);

    spinBox = new QSpinBox(this);
    spinBox->setRange(1, 120);
    spinBox->setValue(0);
    spinBox->resize(50, 25);
    spinBox->move(190, 148);

    QLabel* delayLabel = new QLabel("Remind me again in minutes", this);
    delayLabel->move(250, 150);

    QButtonGroup* buttonGroup = new QButtonGroup(this);
    buttonGroup->addButton(yesbutton);
    buttonGroup->addButton(nobutton);

    QPushButton* closebutton = new QPushButton("Close Timer", this);
    closebutton->resize(120, 30);
    closebutton->move(140, 220);

    connect(closebutton, &QPushButton::clicked, this, &TimerWin::closebutton_clicked);
    connect(yesbutton, &QRadioButton::clicked, this, &TimerWin::yesbutton_clicked);
    connect(nobutton, &QRadioButton::clicked, this, &TimerWin::nobutton_clicked);


}

void TimerWin::yesbutton_clicked()
{

    emit send_data(1);

}


void TimerWin::nobutton_clicked()
{
   spinBox->interpretText();
   emit send_data(spinBox->value());  // Read the current value directly.
//   this->close();
}


void TimerWin::closebutton_clicked()
{

    emit send_data(-1);

}
