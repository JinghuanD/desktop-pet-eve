#include "musicwin.h"
#include "ui_musicwin.h"

MusicWin::MusicWin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MusicWin)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    // Set a rounded-corner mask.
    QBitmap bmp(this->size());
    bmp.fill(Qt::color0);  // color0 is transparent.
    QPainter p(&bmp);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(Qt::color1);  // color1 is opaque.
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(bmp.rect(), 30, 30);
    setMask(bmp);

    // Set window opacity.
    setWindowOpacity(0.95);  // Overall opacity.


    this->setWindowIcon(QIcon(":/images/icon/desktop_pet.png")); // Set the window icon.

    initBtn(); // Initialize buttons.
    initMedia(); // Initialize media.
}

MusicWin::~MusicWin()
{
    delete ui;
}

void MusicWin::initBtn()
{
    // Configure button styles.
    setStyleSheet("QPushButton{border: none;"
                  "background-color:rgb(255,255,255);border-radius: 35px;}"
                  "QPushButton::hover{background-color:rgb(200,200,200);}"
                  "QPushButton:pressed{background-color:rgb(160,160,160);}");

    next = new QPushButton(this);
    last = new QPushButton(this);
    pause = new QPushButton(this);
    play = new QPushButton(this);
    add = new QPushButton(this);
    // Load icons.
    next->setIcon(QIcon(":/images/icon/next.png"));
    last->setIcon(QIcon(":/images/icon/last.png"));
    pause->setIcon(QIcon(":/images/icon/pause.png"));
    play->setIcon(QIcon(":/images/icon/play.png"));
    add->setIcon(QIcon(":/images/icon/add.png"));

    // Icon size.
    int btnSize = 80;
    QSize temp(btnSize,btnSize);
    next->setIconSize(temp);
    last->setIconSize(temp);
    pause->setIconSize(temp);
    play->setIconSize(temp);
    add->setIconSize(temp);

    // Button positions.
    int x=60;
    int y=300;
    next->setGeometry(x+180,y,btnSize,btnSize);
    last->setGeometry(x,y,btnSize,btnSize);
    pause->setGeometry(x+85,y-5,btnSize+10,btnSize+10);
    play->setGeometry(x+85,y-5,btnSize+10,btnSize+10);
    add->setGeometry(x+265,y+5,btnSize-10,btnSize-10);
    play->hide();

    // Connect button signals to their slots.
    connect(play,&QPushButton::clicked,this,&MusicWin::playBtnPush);
    connect(pause,&QPushButton::clicked,this,&MusicWin::pauseBtnPush);
    connect(next,&QPushButton::clicked,this,&MusicWin::nextBtnPush);
    connect(last,&QPushButton::clicked,this,&MusicWin::lastBtnPush);
    connect(add,&QPushButton::clicked,this,&MusicWin::addBtnPush);

}

void MusicWin::paintEvent(QPaintEvent *)
{
     QPainter painter(this);
     painter.setRenderHint(QPainter::Antialiasing);

     // Desired background color.
     QColor bgColor("#ccccff");
     bgColor.setAlphaF(1.0);  // Overall transparency is controlled by windowOpacity.
     painter.setBrush(bgColor);
     painter.setPen(Qt::NoPen);
     painter.drawRoundedRect(rect(), 30, 30);
}

void MusicWin::initMedia()
{
    player = new QMediaPlayer(this);
    playlist = new QMediaPlaylist(this);

    playlist->setPlaybackMode(QMediaPlaylist::Loop); // Set loop mode.
    player->setPlaylist(playlist); // Assign the playlist.
    player->setVolume(60); // Set initial volume.

    musicName = new QPlainTextEdit(this);
    musicList = new QPlainTextEdit(this);

    // Make text-edit backgrounds transparent.
    QPalette pl = musicName->palette();
    pl.setBrush(QPalette::Base,QBrush(QColor(0,0,0,0)));
    musicName->setPalette(pl);
    musicList->setPalette(pl);

    musicName->setStyleSheet("border: 1px solid white;");
    musicList->setStyleSheet("border: 1px solid white;");


    musicList->setGeometry(50,50,350,120);
    musicList->setReadOnly(true);
    musicName->setGeometry(50,180,350,100);
    musicName->setReadOnly(true);

    musicList->setPlainText("Playlist:");
    musicName->setPlainText("Now playing:");
    // Update the displayed music name when the media changes.
    connect(playlist,&QMediaPlaylist::currentMediaChanged,this,&MusicWin::musicChangeEvent);

    volume = new QSlider(this);
    volume->setMaximum(100); // Set maximum value.
    volume->setMinimum(0); // Set minimum value.
    volume->setValue(60); // Set initial value.
    volume->setSingleStep(5); // Set step size.
    volume->setOrientation(Qt::Horizontal); // Horizontal orientation.
    volume->setTickPosition(QSlider::TicksAbove); // Tick marks.
    volume->setGeometry(60,400,330,30); // Position and size.
    // Synchronize volume when the slider changes.
    connect(volume,&QSlider::valueChanged,this,&MusicWin::volumeChangeEvent);
}

void MusicWin::playBtnPush()
{
    play->hide();
    pause->show();
    player->pause();
}

void MusicWin::pauseBtnPush()
{
    if(player->state()==QMediaPlayer::PausedState){
        pause->hide();
        play->show();
        player->play();
    }
}

void MusicWin::nextBtnPush()
{
    if(player->state()!=QMediaPlayer::StoppedState)
        playlist->next();

}

void MusicWin::lastBtnPush()
{
    if(player->state()!=QMediaPlayer::StoppedState)
        playlist->previous();

}

void MusicWin::addBtnPush()
{
    // Open a folder picker for audio files.
    QString curPash = "./music";
    QString dlgTitle="Select Music";
    QString filter="Audio files (*.mp3 *.wav *.wma);;MP3 files (*.mp3);;WAV files (*.wav);;WMA files (*.wma);;All files (*.*)";
    QStringList fileListTemp = QFileDialog::getOpenFileNames(this,dlgTitle,curPash,filter);

//    QDir dir( "./music");
//    QStringList nameFilters;
//    nameFilters << "*.mp3";
//    QStringList fileListTemp = dir.entryList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);

        if(fileListTemp.count()<1)
        return;

    for(int i = 0;i<fileListTemp.count();i++){
        QString aFile = fileListTemp.at(i);
        playlist->addMedia(QUrl::fromLocalFile(aFile)); // Add file to the playlist.
        QFileInfo fileInfo(aFile);
        fileList<<aFile; // Add the file path to the file-name list.
        musicList->appendPlainText(QString::number(fileList.count())+"."+(fileListTemp.at(i).section(QRegExp("[/.]"),-2,-2)));
        // Translated comment.
    }

    if(player->state()==QMediaPlayer::StoppedState){
        playlist->setCurrentIndex(0);
        play->show();
        pause->hide();
        player->play();
    }

}

void MusicWin::musicChangeEvent()
{
    musicName->setPlainText("Now playing:");
    musicName->appendPlainText((fileList.at(playlist->currentIndex()).section(QRegExp("[/.]"),-2,-2)));
}

void MusicWin::volumeChangeEvent()
{
    player->setVolume(volume->value());
}
