#ifndef MUSICWIN_H
#define MUSICWIN_H

#include <QWidget>
#include <QBitmap>
#include <QPainter>
#include<QPushButton>
#include<QFileDialog>
#include<QPlainTextEdit>
#include<QtMultimedia/QMediaPlayer>
#include<QtMultimedia/QMediaPlaylist>
#include<QSlider>

namespace Ui {
class MusicWin;
}

class MusicWin : public QWidget
{
    Q_OBJECT

public:
    explicit MusicWin(QWidget *parent = nullptr);
    ~MusicWin();
    void paintEvent(QPaintEvent *); // Paint event.

private:
    Ui::MusicWin *ui;

    QPushButton *next; // Next-track button.

    QPushButton *last; // Previous-track button.

    QPushButton *pause; // Pause button.

    QPushButton *play; // Play button.

    QPushButton *add; // Add-song button.

    QPlainTextEdit *musicName;// Translated comment.

    QPlainTextEdit *musicList;// Translated comment.

    QMediaPlayer * player;// Translated comment.

    QMediaPlaylist * playlist;// Translated comment.

    QSlider * volume;// Translated comment.

    QStringList fileList;// Translated comment.

    void initBtn();// Translated comment.

    void initMedia();// Translated comment.

    void playBtnPush();// Translated comment.

    void pauseBtnPush();

    void nextBtnPush();

    void lastBtnPush();

    void addBtnPush();

    void musicChangeEvent();// Translated comment.

    void volumeChangeEvent();// Translated comment.
};

#endif // MUSICWIN_H
