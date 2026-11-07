#include "menu.h"
#include "ui_menu.h"


#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QDebug>

Menu::Menu(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(260, 360);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    QWidget *panel = new QWidget(this);
    panel->setObjectName("panel");

    QVBoxLayout *mainLayout = new QVBoxLayout(panel);
    mainLayout->setContentsMargins(18, 12, 18, 16);
    mainLayout->setSpacing(14);

    QLabel *title = new QLabel("Features");
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName("title");

    QGridLayout *grid = new QGridLayout;
    grid->setSpacing(16);

    QPushButton *dressBtn = createMenuButton("👕", "Dress");
    QPushButton *timerBtn = createMenuButton("⏰", "Timer");
    QPushButton *gameBtn  = createMenuButton("🎮", "Games");
    QPushButton *restBtn  = createMenuButton("🌙", "Rest Mode");
    QPushButton *setBtn   = createMenuButton("⚙", "Settings");

    grid->addWidget(dressBtn, 0, 0);
    grid->addWidget(timerBtn, 0, 1);
    grid->addWidget(gameBtn,  1, 0);
    grid->addWidget(restBtn,  1, 1);
    grid->addWidget(setBtn,   1, 2);

    QLabel *quickTitle = new QLabel("Quick Actions");
    quickTitle->setAlignment(Qt::AlignCenter);
    quickTitle->setObjectName("quickTitle");

    QHBoxLayout *quickLayout = new QHBoxLayout;
    quickLayout->setSpacing(15);

    QLabel *topLabel = new QLabel("📌 Always on Top");
    QLabel *clickLabel = new QLabel("🖱 Click Through");

    topLabel->setObjectName("quickText");
    clickLabel->setObjectName("quickText");

    QCheckBox *topSwitch = createSwitch();
    QCheckBox *clickSwitch = createSwitch();

    quickLayout->addWidget(topLabel);
    quickLayout->addWidget(topSwitch);
    quickLayout->addStretch();
    quickLayout->addWidget(clickLabel);
    quickLayout->addWidget(clickSwitch);

    mainLayout->addWidget(title);
    mainLayout->addLayout(grid);
    mainLayout->addWidget(quickTitle);
    mainLayout->addLayout(quickLayout);

    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(panel);

    connect(dressBtn, &QPushButton::clicked, this, &Menu::onDressClicked);
    connect(timerBtn, &QPushButton::clicked, this, &Menu::onTimerClicked);
    connect(gameBtn,  &QPushButton::clicked, this, &Menu::onGameClicked);
    connect(restBtn,  &QPushButton::clicked, this, &Menu::onRestClicked);
    connect(setBtn,   &QPushButton::clicked, this, &Menu::onSettingClicked);

    setStyleSheet(R"(
        QWidget#panel {
            background-color: rgba(20, 22, 45, 235);
            border: 1px solid rgba(140, 130, 255, 160);
            border-radius: 16px;
        }

        QLabel#title {
            color: #E6E1FF;
            background-color: rgba(110, 100, 190, 120);
            border-radius: 12px;
            padding: 4px 30px;
            font-size: 14px;
            font-weight: bold;
        }

        QLabel#quickTitle {
            color: #E6E1FF;
            background-color: rgba(110, 100, 190, 120);
            border-radius: 12px;
            padding: 4px 30px;
            font-size: 13px;
            font-weight: bold;
        }

        QLabel#quickText {
            color: #DCD8FF;
            font-size: 13px;
        }

        QPushButton {
            color: #F0EEFF;
            background-color: rgba(40, 43, 80, 190);
            border: 1px solid rgba(130, 120, 255, 150);
            border-radius: 14px;
            font-size: 13px;
            padding: 8px;
        }

        QPushButton:hover {
            background-color: rgba(90, 85, 170, 210);
            border: 1px solid rgba(180, 170, 255, 230);
        }

        QPushButton:pressed {
            background-color: rgba(120, 110, 210, 230);
        }

        QCheckBox {
            spacing: 0px;
        }

        QCheckBox::indicator {
            width: 42px;
            height: 22px;
            border-radius: 11px;
            background-color: rgba(55, 58, 95, 220);
            border: 1px solid rgba(150, 145, 255, 150);
        }

        QCheckBox::indicator:checked {
            background-color: #8D82FF;
        }
    )");
}

QPushButton* Menu::createMenuButton(const QString& icon, const QString& text)
{
    QPushButton *btn = new QPushButton;
    btn->setText(icon + "\n" + text);
    btn->setFixedSize(65, 70);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

QCheckBox* Menu::createSwitch()
{
    QCheckBox *box = new QCheckBox;
    box->setCursor(Qt::PointingHandCursor);
    return box;
}

void Menu::onDressClicked()
{
    qDebug() << "Open dress window";
}

void Menu::onTimerClicked()
{
    qDebug() << "Open timer";
}

void Menu::onGameClicked()
{
    qDebug() << "Enter game mode";
}

void Menu::onRestClicked()
{
    qDebug() << "Enter rest mode";
}

void Menu::onSettingClicked()
{
    qDebug() << "Open settings";
}




