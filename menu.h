#ifndef MENU_H
#define MENU_H


#include <QWidget>
#include <QPushButton>
#include <QCheckBox>

class Menu : public QWidget
{
    Q_OBJECT

public:
    explicit Menu(QWidget *parent = nullptr);

private:
    QPushButton* createMenuButton(const QString& icon, const QString& text);
    QCheckBox* createSwitch();

private slots:
    void onDressClicked();
    void onTimerClicked();
    void onGameClicked();
    void onRestClicked();
    void onSettingClicked();
};



#endif // Menu_H

