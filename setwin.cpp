#include "setwin.h"
#include "ui_setwin.h"

SetWin::SetWin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SetWin)
{
    ui->setupUi(this);

    // Draw the rounded background in paintEvent instead of relying on a stylesheet background.
    // QSS with WA_StyledBackground does not reliably render under the native Windows style
    // for translucent frameless PE_Widget windows. Fusion works, but changing the global
    // style for one panel is unnecessary. This manual approach was already proven stable.
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true); // Keep the area outside rounded corners transparent.

    setStyleSheet(
        "QLabel, QCheckBox {"
        "  color: white;"
        "  font: bold 15px;"
        "  background: transparent;"
        "}"
    );

    this->setWindowIcon(QIcon(":/images/icon/desktop_pet.png")); // Set the window icon.

    initBtn();
}

SetWin::~SetWin()
{
    delete ui;
}

void SetWin::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    QPainterPath path;
    path.addRoundedRect(rect(), 20, 20);
    painter.fillPath(path, QColor(204, 204, 255, 242)); // #ccccff, about 0.95 opacity.

    QWidget::paintEvent(event);
}

void SetWin::setSize(int size)
{
    haroSize = size;

    // Translated comment.
    // Translated comment.
    // Translated comment.
    if (!sizeNum) {
        QFont ft;
        ft.setPointSize(12);
        QPalette pa;
        pa.setColor(QPalette::WindowText, Qt::blue);
        sizeNum = new QLabel(this);
        sizeNum->move(120, 100);
        sizeNum->resize(120, 20);
        sizeNum->setFont(ft);
        sizeNum->setPalette(pa);
        sizeNum->show();
    }
    sizeNum->setNum(haroSize);

    ui->sizeSlider->setValue(haroSize);
}

int SetWin::getSize()
{
    return haroSize;
}

void SetWin::on_sizeSlider_valueChanged(int value)
{
    haroSize = value;
    // Translated comment.
    // Translated comment.
    // Translated comment.
    if (!sizeNum) {
        QFont ft;
        ft.setPointSize(12);
        QPalette pa;
        pa.setColor(QPalette::WindowText, Qt::blue);
        sizeNum = new QLabel(this);
        sizeNum->move(120, 100);
        sizeNum->resize(120, 20);
        sizeNum->setFont(ft);
        sizeNum->setPalette(pa);
        sizeNum->show();
    }
    sizeNum->setNum(haroSize);

    emit sizeChanged(haroSize); // Translated comment.
}
void SetWin::initBtn()
{
    autoStartCheckBox = new QCheckBox("Launch at startup", this);
    autoStartCheckBox->move(40, 60);
    // Translated comment.
    autoStartCheckBox->setChecked(isAutoStartEnabled());

    connect(autoStartCheckBox, &QCheckBox::toggled,
            this, &SetWin::onAutoStartToggled);

}

void SetWin::onAutoStartToggled(bool checked)
{
    setAutoStart(checked);
}

// Translated comment.
bool SetWin::isAutoStartEnabled()
{
    QString appName = "EveApp";
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                       QSettings::NativeFormat);
    return settings.contains(appName);
}

// Translated comment.
void SetWin::setAutoStart(bool enable)
{
    QString appName = "EveApp";
    QString appPath = QCoreApplication::applicationFilePath();

    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                       QSettings::NativeFormat);

    if (enable) {
        settings.setValue(appName, appPath);
    } else {
        settings.remove(appName);
    }
}

void SetWin::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton)
        moveLeftTop = event->globalPos() - frameGeometry().topLeft();
}

void SetWin::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton)
        move(event->globalPos() - moveLeftTop);
}
