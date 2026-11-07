#ifndef EVE_H
#define EVE_H

#include <QMainWindow>
#include<QMouseEvent>
#include<QtGlobal>
#include <QLabel>
#include<QPoint>
#include<QTimer>
#include<QPushButton>
#include<QCalendarWidget>
#include<QFile>
#include<QDataStream>

#include<QSystemTrayIcon>
#include <QHash>
#include <QVariantMap>
#include <QVector>

#include"dresswin.h"
#include"setwin.h"
#include"musicwin.h"
#include "gamewin.h"
#include "healthwin.h"
#include "readingcompanion.h"
#include "timerwin.h"
#include "pawmenuwidget.h"


using namespace std;

QT_BEGIN_NAMESPACE
//namespace Ui { class Eve; }
QT_END_NAMESPACE

class Eve : public QMainWindow
{
    Q_OBJECT
private:
  //  Ui::Eve *ui;

    PawMenuWidget *m_pawMenu = nullptr; // Paged paw-menu widget.

    QPoint m_setWinOffset; // Settings-window offset used when the pet is dragged.

    DressWin *dresswindow; // Dress-up window.

    SetWin *setwindow; // Settings window.

    GameWin *gamewindow;

    HealthWin *healthwindow;

    ReadingCompanion *readingCompanion = nullptr;

    TimerWin *timerwindow;

    QPoint moveLeftTop; // Drag anchor.

    vector<QPixmap> movement; // Expression-frame container.

    vector<QPixmap> spMovement; // Special-animation-frame container.

    vector<int>faceNum; // Frame count and start offset for each expression.

    vector<QPixmap> body,ears,decs; // Image containers for each component.

    QPixmap eyes; // Eye image.

    QLabel *bodyImage,*earsImage,*decsImage,*eyesImage; // Labels for each component.
    vector<QLabel *> outfitLayerImages; // Labels for layered-outfit images.
    QStringList activeOutfitLayers;
    QHash<QString, QVariantMap> activeOutfitAdjustments;
    bool layeredOutfitActive = false;

    // Half-open and closed frames for the three complete portraits.
    QVector<QPixmap> blinkHalfFrames;
    QVector<QPixmap> blinkClosedFrames;
    // Reading mode uses separate open, half-open, and closed frames with the same blink cadence.
    QVector<QPixmap> readingPortraits;
    QVector<QPixmap> readingBlinkHalfFrames;
    QVector<QPixmap> readingBlinkClosedFrames;
    QVector<QPixmap> happyFrames;
    QVector<QPixmap> surprisedFrames;
    QVector<QPixmap> sleepyFrames;
    QTimer *blinkTimer = nullptr;
    QTimer *autoBlinkTimer = nullptr;
    int blinkFrameIndex = 0;
    int expressionIndex = -1;
    bool readingMode = false;

    QTimer *timer; // Timer.

    int size = 0; // Pet size.

    int btnSize; // Button size.

    int bodyNum,earsNum,decNum; // Selected outfit indexes.

    int face; // Expression index.

    int faceSum; // Expression count.

    int spMove; // Special-action index.

    int btnSwitch_1,btnSwitch_2; // Menu-button visibility switches.

    // Legacy menu pointers remain null because the paw menu no longer creates them.
    QPushButton *closeBtn = nullptr; // Close button.
    QPushButton *dressBtn = nullptr; // Dress-up button.
    QPushButton *moreBtn = nullptr; // More button.
    QPushButton *minBtn = nullptr; // Minimize button.
    QPushButton *setBtn = nullptr; // Settings button.
    QPushButton *musicBtn = nullptr; // Music button.
    QPushButton *gameBtn = nullptr; // Game button.
    QPushButton *calenBtn = nullptr; // Calendar button.

    QSystemTrayIcon* pSystemTray = nullptr; // System tray icon.

public:
    Eve(QWidget *parent = nullptr);
    ~Eve();

    void mouseMoveEvent(QMouseEvent *event) override; // Mouse-move event.

    void mousePressEvent(QMouseEvent *event) override; // Mouse-press event.

    void contextMenuEvent(QContextMenuEvent *e) override;

    void eyesMovementLoad(); // Load eye animations.

    void eyesMovement(); // Update eye animation.

    inline void imageSet(QLabel *image,QPixmap map); // Set a component image.
    void showOnlyPortrait(const QPixmap &map); // Show one complete portrait after clearing prior frames.

    void imageLoad(); // Load component images.

    void specialMovementLoad(); // Load special actions.

    void specialMovement(); // Play a special action.

    void saveData(); // Save data.

    void buildMenu(const QPoint &globalPos); // Show the paw menu.

    void systemTrayPush(QSystemTrayIcon::ActivationReason reason);
    void restoreFromTray();

    void initSystemTray();

    void applyPetSize(int newSize); // Apply size for both polling and signal paths.
    void applyLayeredOutfit(const QStringList &resourcePaths, const QHash<QString, QVariantMap> &adjustments = {});
    void playBlink();
    void advanceBlink();
    void cycleExpression();
    void toggleReadingMode();
    QPixmap currentPortrait() const;

private slots:
    void onDressClicked();
    void onTimerClicked();
    void onDressClosed();
    void onGameClicked();
    void onHealthClicked();
    void onRestClicked();
    void onSettingClicked();
    void onQuitClicked();
    void onHideToTrayClicked();

    void onPawAction(const QString &actionId); // Paw-menu action dispatcher.

    void onPetSizeChanged(int newSize); // Respond to real-time SetWin slider changes.
    void onLayeredOutfitChanged(const QStringList &resourcePaths);

};




#endif // EVE_H

