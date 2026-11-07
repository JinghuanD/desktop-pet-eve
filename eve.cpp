#include "eve.h"
#include "ui_eve.h"
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QCoreApplication>
#include <QMenu>
#include <QAction>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

namespace {
struct AnchorLayer {
    QPointF anchor = QPointF(250, 250);
    int z = 30;
};

struct OutfitAnchorData {
    QHash<QString, QPointF> anchors;
    QHash<QString, QHash<QString, AnchorLayer>> outfits;
};

const OutfitAnchorData &outfitAnchorData()
{
    static const OutfitAnchorData data = [] {
        OutfitAnchorData result;
        QFile file(":/images/appearance/outfit_layouts.json");
        if (!file.open(QIODevice::ReadOnly)) return result;
        const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        const QJsonObject anchors = root.value("anchors").toObject();
        for (auto it = anchors.begin(); it != anchors.end(); ++it) {
            const QJsonArray point = it.value().toArray();
            if (point.size() == 2) result.anchors.insert(it.key(), QPointF(point[0].toDouble(), point[1].toDouble()));
        }
        const QJsonObject outfits = root.value("outfits").toObject();
        for (auto outfitIt = outfits.begin(); outfitIt != outfits.end(); ++outfitIt) {
            QHash<QString, AnchorLayer> layers;
            const QJsonObject layerObject = outfitIt.value().toObject();
            for (auto layerIt = layerObject.begin(); layerIt != layerObject.end(); ++layerIt) {
                const QJsonObject item = layerIt.value().toObject();
                AnchorLayer layer;
                layer.anchor = result.anchors.value(item.value("anchor").toString(), QPointF(250, 250));
                layer.z = item.value("z").toInt(30);
                layers.insert(layerIt.key(), layer);
            }
            result.outfits.insert(outfitIt.key(), layers);
        }
        return result;
    }();
    return data;
}

QString outfitIdForPath(const QString &path)
{
    if (path.contains("/cat_pajama/")) return "cat_pajama";
    if (path.contains("/mid_autumn/")) return "mid_autumn";
    if (path.contains("/pajama_bunny/")) return "pajama_bunny";
    return QString();
}

AnchorLayer layerAnchor(const QString &path)
{
    const OutfitAnchorData &data = outfitAnchorData();
    const QString outfit = outfitIdForPath(path);
    const QString slot = path.section('/', -1).left(2);
    return data.outfits.value(outfit).value(slot, AnchorLayer());
}
}

Eve::Eve(QWidget *parent)
    : QMainWindow(parent)

{

    setWindowFlags(Qt::FramelessWindowHint|Qt::Tool); // Hide the title bar.
    setAttribute(Qt::WA_TranslucentBackground); // Enable a transparent background.
    Qt::WindowFlags m_flags = windowFlags(); // Preserve the current flags.
    setWindowFlags(m_flags|Qt::WindowStaysOnTopHint); // Keep the pet on top.

    int coordX,coordY; // Desktop coordinates.
    QFile file("./file.ini");


    file.open(QIODevice::ReadOnly);


    QTextStream in(&file);
    if (file.isOpen()) {
        in >> size >> bodyNum >> earsNum >> decNum >> coordX >> coordY;
        qDebug() << "size =" << size;
    } else {
        qDebug() << "File could not be opened; using defaults.";
        size   = 300;
        bodyNum = earsNum = decNum = 0;
        coordX = x();
        coordY = y();
    }
    file.close();
    move(coordX,coordY);

//    timer = new QTimer;
// Translated comment.
// Translated comment.


    imageLoad(); // Load component images.
    bodyNum = qBound(0, bodyNum, static_cast<int>(body.size()) - 1);
    earsNum = qBound(0, earsNum, static_cast<int>(ears.size()) - 1);
    decNum = qBound(0, decNum, static_cast<int>(decs.size()) - 1);
// Translated comment.
// Translated comment.

    bodyImage = new QLabel(this); // Body image label.
    eyesImage = new QLabel(this); // Eye image label.
    earsImage = new QLabel(this); // Ear image label.
    decsImage = new QLabel(this); // Decoration image label.

    blinkTimer = new QTimer(this);
    blinkTimer->setInterval(110);
    connect(blinkTimer, &QTimer::timeout, this, &Eve::advanceBlink);
    autoBlinkTimer = new QTimer(this);
    autoBlinkTimer->setInterval(2000);
    connect(autoBlinkTimer, &QTimer::timeout, this, [this] {
        if (expressionIndex < 0) playBlink();
    });
    autoBlinkTimer->start();

    imageSet(bodyImage,body[bodyNum]);

    // Complete portraits already contain facial features, so no legacy eye layer is added.
    imageSet(eyesImage,eyes);
    eyesImage->hide();

    imageSet(earsImage,ears[earsNum]);

    imageSet(decsImage,decs[decNum]);


    dresswindow  = new DressWin(nullptr);
    dresswindow->accept(body, ears, decs, bodyNum, earsNum, decNum);
    dresswindow->hide();
    connect(dresswindow, &DressWin::closed, this, &Eve::onDressClosed);
    connect(dresswindow, &DressWin::layeredOutfitChanged, this, &Eve::onLayeredOutfitChanged);
    timerwindow  = new TimerWin(this);
    timerwindow->hide();
    gamewindow   = new GameWin(this);
    gamewindow->hide();
    healthwindow = new HealthWin(nullptr);
    healthwindow->hide();
    readingCompanion = new ReadingCompanion(nullptr);
    readingCompanion->hide();
    connect(readingCompanion, &ReadingCompanion::exitRequested, this, [this] {
        if (readingMode) toggleReadingMode();
    });
    setwindow    = new SetWin(nullptr);
    setwindow->hide();

    m_pawMenu = new PawMenuWidget(nullptr); // Independent top-level paged paw menu.
    connect(m_pawMenu, &PawMenuWidget::actionTriggered, this, &Eve::onPawAction);

    // Apply slider changes immediately instead of waiting for polling.
    connect(setwindow, &SetWin::sizeChanged, this, &Eve::onPetSizeChanged);

    initSystemTray(); // Initialize the system tray.
}

Eve::~Eve()
{
    // Clean up allocated pointers.

    // Remove the tray icon first so Windows does not briefly retain an invalid icon after exit.
    if (pSystemTray) pSystemTray->hide();

    delete earsImage;
    delete bodyImage;
    delete eyesImage;
    delete decsImage;

    delete closeBtn;
    delete dressBtn;
    delete moreBtn;
    delete minBtn;
    delete setBtn;
    delete musicBtn;
    delete gameBtn;
    delete calenBtn;

    delete dresswindow;
    delete setwindow;
    delete healthwindow;
    delete readingCompanion;
    delete m_pawMenu;
    //delete musicWindow;
}

void Eve::imageSet(QLabel *image,QPixmap map)
{
    // Set image size and position from the current pet size.
    image->setPixmap(map.scaled(size,size)); // Scale the image to avoid blurred resizing.
    image->setScaledContents(true);
    image->resize(size,size);
//    image->move(this->frameGeometry().width()/2 - size/2,
//               this->frameGeometry().height()/2 - size/2);
    image->move(0,0);
}

void Eve::showOnlyPortrait(const QPixmap &map)
{
    if (!bodyImage || map.isNull()) return;

    // Reading blinks must display only one complete portrait. Hide old layers and clear the
    // previous pixels before setting the new PNG to prevent visual remnants in transparent areas.
    if (eyesImage) eyesImage->hide();
    if (earsImage) earsImage->hide();
    if (decsImage) decsImage->hide();
    for (QLabel *layer : outfitLayerImages) {
        if (layer) layer->hide();
    }
    bodyImage->clear();
    imageSet(bodyImage, map);
    bodyImage->show();
    bodyImage->raise();
}

void Eve::applyLayeredOutfit(const QStringList &resourcePaths, const QHash<QString, QVariantMap> &adjustments)
{
    activeOutfitLayers = resourcePaths;
    std::stable_sort(activeOutfitLayers.begin(), activeOutfitLayers.end(),
                     [](const QString &left, const QString &right) {
                         return layerAnchor(left).z < layerAnchor(right).z;
                     });
    activeOutfitAdjustments = adjustments;
    layeredOutfitActive = !activeOutfitLayers.isEmpty();
    bodyImage->hide();
    earsImage->hide();
    decsImage->hide();
    eyesImage->hide();

    while (outfitLayerImages.size() < activeOutfitLayers.size()) {
        outfitLayerImages.push_back(new QLabel(this));
    }
    for (int i = 0; i < static_cast<int>(outfitLayerImages.size()); ++i) {
        if (i < activeOutfitLayers.size()) {
            const QVariantMap adjustment = activeOutfitAdjustments.value(activeOutfitLayers[i]);
            const qreal scale = adjustment.value("scale", 100).toInt() / 100.0;
            const QPixmap source(activeOutfitLayers[i]);
            const AnchorLayer layout = layerAnchor(activeOutfitLayers[i]);
            const QSize layerSize(qRound(source.width() * scale * size / 500.0),
                                  qRound(source.height() * scale * size / 500.0));
            // Current assets use a complete 500×500 canvas. Scale around the same anchor and
            // compensate to the canvas center so confirmed visual parameters remain unchanged.
            const QPointF canvasCenter(250, 250);
            QPointF logicalOrigin = layout.anchor - layout.anchor * scale;
            logicalOrigin += (canvasCenter - layout.anchor) * (1.0 - scale);
            logicalOrigin += QPointF(adjustment.value("x", 0).toInt(), adjustment.value("y", 0).toInt());
            outfitLayerImages[i]->setPixmap(source.scaled(layerSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            outfitLayerImages[i]->setScaledContents(true);
            outfitLayerImages[i]->resize(layerSize);
            outfitLayerImages[i]->move(qRound(logicalOrigin.x() * size / 500.0),
                                       qRound(logicalOrigin.y() * size / 500.0));
            outfitLayerImages[i]->show();
            outfitLayerImages[i]->raise();
        } else {
            outfitLayerImages[i]->hide();
        }
    }
}

void Eve::imageLoad()
{
    // Use complete PNGs as pet portraits without loading or composing individual parts.
    body.push_back(QPixmap(":/images/appearance/stand_sets/mid_autumn_complete.png"));
    body.push_back(QPixmap(":/images/appearance/stand_sets/bunny_pajama_complete.png"));
    body.push_back(QPixmap(":/images/appearance/stand_sets/cat_pajama_complete.png"));

    blinkHalfFrames = {
        QPixmap(":/images/appearance/stand_blink/mid_autumn_half.png"),
        QPixmap(":/images/appearance/stand_blink/bunny_pajama_half.png"),
        QPixmap(":/images/appearance/stand_blink/cat_pajama_half.png")
    };
    blinkClosedFrames = {
        QPixmap(":/images/appearance/stand_blink/mid_autumn_closed.png"),
        QPixmap(":/images/appearance/stand_blink/bunny_pajama_closed.png"),
        QPixmap(":/images/appearance/stand_blink/cat_pajama_closed.png")
    };
    readingPortraits = {
        QPixmap(":/images/appearance/reading_expressions/mid_autumn_open.png"),
        QPixmap(":/images/appearance/reading_expressions/bunny_pajama_open.png"),
        QPixmap(":/images/appearance/reading_expressions/cat_pajama_open.png")
    };
    readingBlinkHalfFrames = {
        QPixmap(":/images/appearance/reading_expressions/mid_autumn_half.png"),
        QPixmap(":/images/appearance/reading_expressions/bunny_pajama_half.png"),
        QPixmap(":/images/appearance/reading_expressions/cat_pajama_half.png")
    };
    readingBlinkClosedFrames = {
        QPixmap(":/images/appearance/reading_expressions/mid_autumn_closed.png"),
        QPixmap(":/images/appearance/reading_expressions/bunny_pajama_closed.png"),
        QPixmap(":/images/appearance/reading_expressions/cat_pajama_closed.png")
    };
    happyFrames = {
        QPixmap(":/images/appearance/stand_expressions/mid_autumn_happy.png"),
        QPixmap(":/images/appearance/stand_expressions/bunny_pajama_happy.png"),
        QPixmap(":/images/appearance/stand_expressions/cat_pajama_happy.png")
    };
    surprisedFrames = {
        QPixmap(":/images/appearance/stand_expressions/mid_autumn_surprised.png"),
        QPixmap(":/images/appearance/stand_expressions/bunny_pajama_surprised.png"),
        QPixmap(":/images/appearance/stand_expressions/cat_pajama_surprised.png")
    };
    sleepyFrames = {
        QPixmap(":/images/appearance/stand_expressions/mid_autumn_sleepy.png"),
        QPixmap(":/images/appearance/stand_expressions/bunny_pajama_sleepy.png"),
        QPixmap(":/images/appearance/stand_expressions/cat_pajama_sleepy.png")
    };
    for (int i = 0; i < 3; ++i) {
        QPixmap transparentLayer(500, 500);
        transparentLayer.fill(Qt::transparent);
        ears.push_back(transparentLayer);
        decs.push_back(transparentLayer);
    }

    resize(size, size);
}

void Eve::playBlink()
{
    // The simplified version animates complete portraits only; legacy layers cannot cover a blink.
    const QVector<QPixmap> &halfFrames = readingMode ? readingBlinkHalfFrames : blinkHalfFrames;
    const QVector<QPixmap> &closedFrames = readingMode ? readingBlinkClosedFrames : blinkClosedFrames;
    if ((!readingMode && expressionIndex >= 0) || layeredOutfitActive || !bodyImage || bodyNum < 0
        || bodyNum >= halfFrames.size() || bodyNum >= closedFrames.size()) return;

    // Both click and timer triggers restart at frame one to prevent overlapping animations.
    blinkTimer->stop();
    blinkFrameIndex = 0;
    blinkTimer->start();
    advanceBlink();
}

void Eve::advanceBlink()
{
    if (!bodyImage || bodyNum < 0 || bodyNum >= static_cast<int>(body.size())) return;
    const QVector<QPixmap> &halfFrames = readingMode ? readingBlinkHalfFrames : blinkHalfFrames;
    const QVector<QPixmap> &closedFrames = readingMode ? readingBlinkClosedFrames : blinkClosedFrames;
    if (bodyNum >= halfFrames.size() || bodyNum >= closedFrames.size()) return;

    // Show half-open, closed, closed, half-open, then restore the selected open portrait.
    if (blinkFrameIndex == 0 || blinkFrameIndex == 3) {
        showOnlyPortrait(halfFrames[bodyNum]);
    } else if (blinkFrameIndex == 1 || blinkFrameIndex == 2) {
        showOnlyPortrait(closedFrames[bodyNum]);
    } else {
        blinkTimer->stop();
        showOnlyPortrait(currentPortrait());
        return;
    }
    ++blinkFrameIndex;
}

QPixmap Eve::currentPortrait() const
{
    if (bodyNum < 0 || bodyNum >= static_cast<int>(body.size())) return QPixmap();
    if (readingMode && bodyNum < readingPortraits.size() && !readingPortraits.at(bodyNum).isNull()) {
        return readingPortraits.at(bodyNum);
    }
    const QVector<QPixmap> *frames = nullptr;
    if (expressionIndex == 0) frames = &happyFrames;
    else if (expressionIndex == 1) frames = &surprisedFrames;
    else if (expressionIndex == 2) frames = &sleepyFrames;
    if (frames && bodyNum < frames->size() && !frames->at(bodyNum).isNull()) return frames->at(bodyNum);
    return body[bodyNum];
}

void Eve::cycleExpression()
{
    if (bodyNum < 0 || bodyNum >= static_cast<int>(body.size())) return;
    // Keep the reading pose while reading; left-click expression cycling applies only to standing.
    if (readingMode) return;
    // The default expression is part of the cycle: default → happy → surprised → sleepy → default.
    expressionIndex = expressionIndex >= 2 ? -1 : expressionIndex + 1;
    if (blinkTimer) blinkTimer->stop();
    imageSet(bodyImage, currentPortrait());
}

void Eve::toggleReadingMode()
{
    if (bodyNum < 0 || bodyNum >= static_cast<int>(body.size())) return;
    const bool hasReadingPortrait = bodyNum < readingPortraits.size()
        && !readingPortraits.at(bodyNum).isNull();
    if (!hasReadingPortrait) return;

    readingMode = !readingMode;
    expressionIndex = -1;
    if (blinkTimer) blinkTimer->stop();
    showOnlyPortrait(currentPortrait());
    if (readingCompanion) {
        if (readingMode) {
            readingCompanion->followPet(frameGeometry());
            readingCompanion->beginTracking();
        } else {
            readingCompanion->stopTracking();
        }
    }
}




void Eve::initSystemTray()
{
    pSystemTray = new QSystemTrayIcon(this);
    pSystemTray->setIcon(QIcon(":/images/icon/desktop_pet.png"));
    pSystemTray->setToolTip("Hello, I'm Eve.");

    auto *trayMenu = new QMenu(this);
    auto *restoreAction = trayMenu->addAction("Restore / Show Pet");
    auto *quitAction = trayMenu->addAction("Quit");
    connect(restoreAction, &QAction::triggered, this, &Eve::restoreFromTray);
    connect(quitAction, &QAction::triggered, this, &Eve::onQuitClicked);
    pSystemTray->setContextMenu(trayMenu);
    pSystemTray->show();
    connect(pSystemTray,&QSystemTrayIcon::activated,this,&Eve::systemTrayPush);
}

void Eve::systemTrayPush(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        restoreFromTray();
    }
}

void Eve::restoreFromTray()
{
    // Restoring from the tray requires more than show(): the window may still be minimized or off-screen.
    if (m_pawMenu) m_pawMenu->hide();
    if (dresswindow) dresswindow->hide();
    if (setwindow) setwindow->hide();
    if (gamewindow) gamewindow->hide();
    if (healthwindow) healthwindow->hide();
    if (timerwindow) timerwindow->hide();
    if (readingCompanion) readingCompanion->stopTracking();

    setWindowState(Qt::WindowNoState);
    if (QScreen *screen = QApplication::primaryScreen()) {
        // Preserve the prior position and move back only if the entire pet lies outside the available screen.
        const QRect available = screen->availableGeometry();
        if (!available.intersects(frameGeometry())) {
            move(available.center() - QPoint(width() / 2, height() / 2));
        }
    }

    if (layeredOutfitActive) {
        for (QLabel *layer : outfitLayerImages) layer->show();
    } else {
        if (bodyImage) bodyImage->show();
        if (earsImage) earsImage->show();
        if (decsImage) decsImage->show();
        if (eyesImage) eyesImage->hide();
    }
    show();
    raise();
    activateWindow();
    if (readingMode && readingCompanion) {
        readingCompanion->followPet(frameGeometry());
        readingCompanion->beginTracking();
    }
}



void Eve::mouseMoveEvent(QMouseEvent *event)
{

    if(event->buttons() & Qt::LeftButton) // Drag the window while the left mouse button is held.
    {
        this->move(event->globalPos()-moveLeftTop);

        // The settings window is an independent top-level window, so move it using its saved offset.
        if (setwindow && setwindow->isVisible()) {
            setwindow->move(this->frameGeometry().topLeft() + m_setWinOffset);
        }
        if (readingMode && readingCompanion && readingCompanion->isVisible()) {
            readingCompanion->followPet(frameGeometry());
        }

        saveData();
    }
}

void Eve::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) { // Left-click event.
        moveLeftTop = event->pos();
        cycleExpression();
    }
 else if(event->button() == Qt::RightButton) { // Right-click event.


    // menu.exec(event->globalPos);

 }
}

void Eve::contextMenuEvent(QContextMenuEvent *e)
{
     buildMenu(e->globalPos());
}

void Eve::buildMenu(const QPoint &globalPos)
{
    // Translated comment.
    // Translated comment.
    // Translated comment.
    if (!m_pawMenu) return;

    Q_UNUSED(globalPos);

    QPoint anchor = frameGeometry().center();
    anchor.setY(frameGeometry().top());
    m_pawMenu->popupAt(anchor);
}

void Eve::onDressClicked()
{
    qDebug() << "Open dress window";
    if(!dresswindow) return;
    if(dresswindow->isHidden())
    {
        timerwindow->hide();
        gamewindow->hide();
        setwindow->hide();
        healthwindow->hide();
        dresswindow->show();
        dresswindow->raise();
        dresswindow->activateWindow();

    }
    else
     {
        dresswindow->hide();
    }


}

void Eve::onTimerClicked()
{
    qDebug() << "Open timer";
}

void Eve::onDressClosed()
{
    if (!dresswindow) return;

    if (dresswindow->usesLayeredSelection()) {
        show();
        raise();
        saveData();
        return;
    }

    layeredOutfitActive = false;
    for (QLabel *layer : outfitLayerImages) layer->hide();

    bodyNum = dresswindow->getBodyNum();
    earsNum = dresswindow->getEarsNum();
    decNum = dresswindow->getDecsNum();
    imageSet(bodyImage, currentPortrait());
    imageSet(earsImage, ears[earsNum]);
    imageSet(decsImage, decs[decNum]);
    // Translated comment.
    bodyImage->show();
    earsImage->show();
    decsImage->show();
    eyesImage->hide();
    show();
    raise();
    saveData();
}

void Eve::onLayeredOutfitChanged(const QStringList &resourcePaths)
{
    applyLayeredOutfit(resourcePaths, dresswindow->layerAdjustments());
}

void Eve::onGameClicked()
{
    qDebug() << "Enter game mode";
    if (!gamewindow) return;

    timerwindow->hide();
    dresswindow->hide();
    setwindow->hide();
    healthwindow->hide();
    gamewindow->show();
    gamewindow->raise();
    gamewindow->activateWindow();
}

void Eve::onHealthClicked()
{
    if (!healthwindow) return;
    timerwindow->hide();
    gamewindow->hide();
    dresswindow->hide();
    setwindow->hide();
    // Translated comment.
    // Translated comment.
    QPoint target = frameGeometry().topRight() + QPoint(12, 0);
    QScreen *screen = QGuiApplication::screenAt(frameGeometry().center());
    if (!screen) screen = QApplication::primaryScreen();
    if (screen) {
        const QRect available = screen->availableGeometry();
        target.setX(qBound(available.left(), target.x(), available.right() - healthwindow->width()));
        target.setY(qBound(available.top(), target.y(), available.bottom() - healthwindow->height()));
    }
    // Translated comment.
    // Translated comment.
    healthwindow->hide();
    QTimer::singleShot(0, this, [this, target] {
        if (!healthwindow) return;
        healthwindow->setWindowState(Qt::WindowNoState);
        healthwindow->move(target);
        healthwindow->showNormal();
        healthwindow->move(target);
        healthwindow->raise();
        healthwindow->activateWindow();
    });
}

void Eve::onRestClicked()
{
    qDebug() << "Enter rest mode";
}

void Eve::onSettingClicked()
{
    qDebug() << "Open settings";
    if(setwindow->isHidden())
    {
        timerwindow->hide();
        gamewindow->hide();
        dresswindow->hide();
        healthwindow->hide();

        // Translated comment.
        QRect petRect = frameGeometry();
        int targetX = petRect.right() + 8;
        int targetY = petRect.top() + (petRect.height() - setwindow->height()) / 2;

        // Translated comment.
        // Translated comment.
        if (QScreen *screen = QApplication::primaryScreen()) {
            QRect avail = screen->availableGeometry();
            if (targetX + setwindow->width() > avail.right()) {
                targetX = petRect.left() - setwindow->width() - 8;
            }
            targetY = qBound(avail.top(), targetY, avail.bottom() - setwindow->height());
        }

        QPoint targetPos(targetX, targetY);


        setwindow->setSize(size);

        setwindow->move(targetPos);
        setwindow->show();
        setwindow->move(targetPos); // Translated comment.
        setwindow->raise();
        setwindow->activateWindow();

        // Translated comment.
        m_setWinOffset = setwindow->pos() - petRect.topLeft();

    }
    else
     {
        setwindow->hide();
    }

}

void Eve::onQuitClicked(){
    if (pSystemTray) pSystemTray->hide();
    QCoreApplication::quit();
}

void Eve::onHideToTrayClicked()
{
    qDebug() << "Hide to tray";
    // Translated comment.
    // Translated comment.
    if (m_pawMenu) m_pawMenu->hide();
    if (setwindow) setwindow->hide();
    if (dresswindow) dresswindow->hide();
    if (gamewindow) gamewindow->hide();
    if (healthwindow) healthwindow->hide();
    if (timerwindow) timerwindow->hide();
    if (readingCompanion) readingCompanion->stopTracking();
    hide();
    pSystemTray->showMessage("Eve", "The pet is hidden in the system tray. Click the tray icon to restore it.",
                             QSystemTrayIcon::Information, 2500);
}

void Eve::onPawAction(const QString &actionId)
{
    // Translated comment.
    // Translated comment.
    if (actionId == "dress") {
        onDressClicked();
    } else if (actionId == "game") {
        onGameClicked();
    } else if (actionId == "health") {
        // Translated comment.
        // Translated comment.
        QTimer::singleShot(180, this, &Eve::onHealthClicked);
    } else if (actionId == "reading") {
        // Translated comment.
        QTimer::singleShot(180, this, &Eve::toggleReadingMode);
    } else if (actionId == "settings") {
        onSettingClicked();
    } else if (actionId == "timer") {
        onTimerClicked();
    } else if (actionId == "rest") {
        onRestClicked();
    } else if (actionId == "hide_tray") {
        onHideToTrayClicked();
    } else if (actionId == "quit") {
        onQuitClicked();
    } else {
        qDebug() << "Unknown paw-menu action:" << actionId;
    }
}


void Eve::eyesMovementLoad()
{
    faceNum.push_back(9);// Translated comment.
    faceNum.push_back(0);// Translated comment.
    for(int i = 1; i<=faceNum[0]; i++)// Translated comment.
        movement.push_back(QPixmap(QString(":/images/movement/blink/%1.png").arg(i)));
    faceNum.push_back(12);
    faceNum.push_back(9);
    for(int i = 1; i<=faceNum[2]; i++)// Translated comment.
        movement.push_back(QPixmap(QString(":/images/movement/heart/%1.png").arg(i)));
    faceNum.push_back(16);
    faceNum.push_back(21);
    for(int i = 1; i<=faceNum[4]; i++)// Translated comment.
        movement.push_back(QPixmap(QString(":/images/movement/question/%1.png").arg(i)));
    faceNum.push_back(15);
    faceNum.push_back(37);
    for(int i = 1; i<=faceNum[6]; i++)// Translated comment.
        movement.push_back(QPixmap(QString(":/images/movement/closeEyes/%1.png").arg(i)));
    faceNum.push_back(9);
    faceNum.push_back(52);
    for(int i = 1; i<=faceNum[8]; i++)// Translated comment.
        movement.push_back(QPixmap(QString(":/images/movement/wink/%1.png").arg(i)));

    face = -1;// Translated comment.
    faceSum = 5;// Translated comment.
}

void Eve::eyesMovement()
{
    // Translated comment.
    static int flag = 0,second1 = 0,second2 = 0,earSwitch = 1;
    int valve = qrand()%200;
    if(face<0 && spMove<0){// Translated comment.
        second1++;
        if(second1>=valve && valve>100){
            face = 0;
            second1 = 0;
        }
    }

    second2++;// Translated comment.
    if(second2>40 && earSwitch){
        earsImage->setPixmap(ears[earsNum].scaled(size,size));
        earSwitch = 0;
        second2 = 0;
    }
    else if(second2>10 && !earSwitch){
        earsImage->setPixmap(ears[earsNum].scaled(size,size));
        earSwitch = 1;
        second2 = 0;
    }

    if(face!=-1){// Translated comment.
    int num = faceNum[face*2],start = faceNum[face*2+1];
        flag++;
        if(flag<num)
            eyesImage->setPixmap(
                    movement[start+flag].scaled(size,size));
        else
            eyesImage->setPixmap(
                    movement[start-flag+(num-1)*2].scaled(size,size));

        if(flag>=(num-1)*2){
            flag = 0;
            face = -1;
            eyesImage->setPixmap(eyes.scaled(size,size));
        }
    }
    if(!dresswindow->isHidden()){// Translated comment.
        if(bodyNum!=dresswindow->getBodyNum()){
            bodyNum = dresswindow->getBodyNum();
            bodyImage->setPixmap(body[bodyNum].scaled(size,size));
            saveData();
        }
        if(earsNum!=dresswindow->getEarsNum()){
            earsNum = dresswindow->getEarsNum();
            earsImage->setPixmap(ears[earsNum].scaled(size,size));
            saveData();
        }
    }

    if(!setwindow->isHidden()){// Translated comment.
        if(size!=setwindow->getSize()){
            applyPetSize(setwindow->getSize());
        }
    }
    if(spMove>-1)// Translated comment.
        specialMovement();
}


void Eve::specialMovementLoad()
{
    for(int i = 1;i<=11;i++)
        spMovement.push_back(QPixmap(QString(":/images/movement/error/%1.png").arg(i)));
    for(int i = 1;i<=22;i++)
        spMovement.push_back(QPixmap(QString(":/images/movement/fly/%1.png").arg(i)));

    spMove = -1;
}

void Eve::specialMovement(){
    static int flag = 0;

    if(spMove == 0){// Translated comment.
        if(flag%20==0 && flag<=200)
            eyesImage->setPixmap(spMovement[flag/20].scaled(size,size));
        else if(flag>300){
            eyesImage->setPixmap(eyes.scaled(size,size));
            flag=0;
            spMove=-1;
            return ;
        }

    }


    flag++;
}

void Eve::saveData()
{
    QFile file("./file.ini");
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);
    out << size <<" "<< bodyNum<<" " << earsNum <<" "<< decNum <<" "<< x() <<" "<< y();
    file.close();
    qDebug() << "Saved successfully";

}

void Eve::applyPetSize(int newSize)
{
    // Translated comment.
    // Translated comment.
    // Translated comment.
    // Translated comment.
    // Translated comment.
    newSize = qBound(60, newSize, 1600);
    if (size == newSize) return;
    size = newSize;

    // Translated comment.
    resize(size, size);
    if (layeredOutfitActive) {
        for (int i = 0; i < activeOutfitLayers.size() && i < static_cast<int>(outfitLayerImages.size()); ++i) {
            const QVariantMap adjustment = activeOutfitAdjustments.value(activeOutfitLayers[i]);
            const qreal scale = adjustment.value("scale", 100).toInt() / 100.0;
            const int layerSize = qRound(size * scale);
            const int offsetX = qRound(adjustment.value("x", 0).toInt() * size / 500.0);
            const int offsetY = qRound(adjustment.value("y", 0).toInt() * size / 500.0);
            outfitLayerImages[i]->setPixmap(QPixmap(activeOutfitLayers[i]).scaled(layerSize, layerSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            outfitLayerImages[i]->resize(layerSize, layerSize);
            outfitLayerImages[i]->move((size - layerSize) / 2 + offsetX, (size - layerSize) / 2 + offsetY);
        }
        saveData();
        return;
    }
    imageSet(bodyImage, currentPortrait());
    eyesImage->hide();
    imageSet(decsImage, decs[decNum]);
    imageSet(earsImage, ears[earsNum]);

    if (readingMode && readingCompanion && readingCompanion->isVisible()) {
        readingCompanion->followPet(frameGeometry());
    }

    saveData();
}

void Eve::onPetSizeChanged(int newSize)
{
    // Translated comment.
    // Translated comment.
    applyPetSize(newSize);
}





