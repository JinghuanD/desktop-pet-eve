#ifndef SETWIN_H
#define SETWIN_H

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QLabel>
#include <QString>
#include <QSettings>
#include <QCoreApplication>
#include <QFileInfo>
#include <QCheckBox>
#include <QPoint>
#include <QMouseEvent>



namespace Ui {
class SetWin;
}

class SetWin : public QWidget
{
    Q_OBJECT

public:
    explicit SetWin(QWidget *parent = nullptr);
    ~SetWin();

    void setSize(int); // Set the size value.

    int getSize(); // Retrieve the size value externally.

    void initBtn();

    void onAutoStartToggled(bool checked);

    bool isAutoStartEnabled();

    void setAutoStart(bool enable);

    void mousePressEvent(QMouseEvent *event);

    void mouseMoveEvent(QMouseEvent *event);

    void paintEvent(QPaintEvent *event) override;

    QPoint moveLeftTop;

signals:

    void sizeChanged(int value); // Emitted immediately when the slider changes.

private slots:


    void on_sizeSlider_valueChanged(int value); // Slider slot.

private:

    int haroSize; // Pet-size parameter.

    QLabel *sizeNum = nullptr; // Label showing the size value.

    Ui::SetWin *ui;

    QCheckBox *autoStartCheckBox;
};

#endif // SETWIN_H
