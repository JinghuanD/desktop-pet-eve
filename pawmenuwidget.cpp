#include "pawmenuwidget.h"

#include <QApplication>
#include <QPushButton>
#include <QLabel>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QScreen>
#include <QFont>

namespace {
constexpr int kMenuWidth  = 280;
constexpr int kMenuHeight = 250;
}

PawMenuWidget::PawMenuWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(kMenuWidth, kMenuHeight);

    buildPages();

    m_prevArrow = new QPushButton("<", this);
    m_nextArrow = new QPushButton(">", this);
    for (QPushButton *arrow : {m_prevArrow, m_nextArrow}) {
        arrow->setFixedSize(24, 24);
        arrow->setStyleSheet(
            "QPushButton {"
            "  border-radius: 12px;"
            "  background: rgba(52,48,120,200);"
            "  border: 1px solid rgba(150,140,255,120);"
            "  color: #E7DFFF;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover { background: rgba(190,60,90,180); }"
        );
    }
    m_prevArrow->move(2, 190);
    m_nextArrow->move(kMenuWidth - 26, 190);
    connect(m_prevArrow, &QPushButton::clicked, this, [this]() {
        if (m_currentPage > 0) showPage(m_currentPage - 1);
    });
    connect(m_nextArrow, &QPushButton::clicked, this, [this]() {
        if (m_currentPage < m_pages.size() - 1) showPage(m_currentPage + 1);
    });

    // Palm pad: branded circular button that cycles pages and can act as an avatar display.
    m_palmButton = new QPushButton(QString::fromUtf8("\xF0\x9F\x90\xBE"), this); // 🐾
    // Keep the original palm size and only move it up to reduce the gap to the toes.
    m_palmButton->setFixedSize(88, 70);
    m_palmButton->move((kMenuWidth - 88) / 2, 138);
    {
        QFont palmFont(m_palmButton->font());
        palmFont.setFamilies({"Segoe UI Emoji", "Noto Color Emoji", palmFont.family()});
        palmFont.setPointSize(18);
        m_palmButton->setFont(palmFont);
    }
    m_palmButton->setStyleSheet(
        "QPushButton {"
            "  border-radius: 35px;"
        "  background: rgba(128,101,232,220);"
        "  border: 1px solid rgba(150,140,255,120);"
        "  font-size: 22px;"
        "}"
        "QPushButton:hover { background: rgba(150,120,240,230); }"
    );
    connect(m_palmButton, &QPushButton::clicked, this, [this]() {
        int next = (m_currentPage + 1) % m_pages.size();
        showPage(next);
    });

    for (int i = 0; i < m_pages.size(); ++i) {
        auto *dot = new QLabel(this);
        dot->setFixedSize(6, 6);
        dot->move(kMenuWidth / 2 - (m_pages.size() * 8) / 2 + i * 8, 232);
        m_dots.push_back(dot);
    }

    showPage(0);
}

void PawMenuWidget::buildPages()
{
    // First page: frequently used actions.
    QVector<PawItem> page1 = {
        {QString::fromUtf8("\xF0\x9F\x91\x95"), "Dress", "dress"},
        {QString::fromUtf8("\xF0\x9F\x8E\xAE"), "Games", "game"},
        {QString::fromUtf8("\xF0\x9F\x93\x9A"), "Reading", "reading"},
        {QString::fromUtf8("\xF0\x9F\xA5\x97"), "Health", "health"},
    };
    // Second page: secondary actions and exit.
    QVector<PawItem> page2 = {
        {QString::fromUtf8("\xE2\x8F\xB0"), "Timer", "timer"},
        {QString::fromUtf8("\xE2\x9A\x99"),     "Settings", "settings"},
        {QString::fromUtf8("\xF0\x9F\x93\x8C"), "Hide", "hide_tray"},
        {QString::fromUtf8("\xE2\x87\xA5"),     "Quit", "quit"},
    };
    m_pages.push_back(page1);
    m_pages.push_back(page2);
}

QPushButton *PawMenuWidget::makeToeButton(const PawItem &item, int x, int y, int diameter)
{
    auto *btn = new QPushButton(this);
    btn->setFixedSize(diameter, diameter);
    btn->move(x, y);
    btn->setText(item.iconGlyph);
    btn->setCursor(Qt::PointingHandCursor);

    // Explicitly select a color-emoji font. Windows' default font may not include all emoji glyphs.
    // "Segoe UI Emoji" is available on Windows; Qt falls back to a suitable installed font.
    QFont emojiFont(btn->font());
    emojiFont.setFamilies({"Segoe UI Emoji", "Noto Color Emoji", emojiFont.family()});
    emojiFont.setPointSize(14);
    btn->setFont(emojiFont);

    bool isDanger = (item.actionId == "quit");
    QString bg     = isDanger ? "rgba(150,45,70,180)"  : "rgba(52,48,120,180)";
    QString border = isDanger ? "rgba(255,100,130,120)" : "rgba(150,140,255,90)";
    QString hover  = isDanger ? "rgba(190,60,90,210)"   : "rgba(80,72,170,200)";

    btn->setStyleSheet(QString(
        "QPushButton {"
        "  border-radius: %1px;"
        "  background: %2;"
        "  border: 1px solid %3;"
        "  font-size: 20px;"
        "}"
        "QPushButton:hover { background: %4; }"
    ).arg(diameter / 2).arg(bg, border, hover));

    connect(btn, &QPushButton::clicked, this, [this, item]() {
        emit actionTriggered(item.actionId);
        hideWithAnimation();
    });

    return btn;
}

void PawMenuWidget::layoutToeButtons(const QVector<PawItem> &items)
{
    for (auto *btn : m_currentToeButtons)  btn->deleteLater();
    for (auto *lbl : m_currentToeLabels)   lbl->deleteLater();
    m_currentToeButtons.clear();
    m_currentToeLabels.clear();

    if (items.isEmpty()) return;

    struct Pos { int x, y, d; };
    // Toe positions form a cat-paw arc and were tuned for the 280x250 canvas.
    // Recheck their bounds if kMenuWidth or kMenuHeight changes.
    const Pos positions[4] = {
        {16,  78, 52},
        {68,  32, 56},
        {132, 28, 56},
        {196, 70, 52},
    };

    constexpr int labelWidth  = 76;
    constexpr int labelHeight = 16;
    constexpr int labelGap    = 6; // Gap between the button bottom and label top.

    const int buttonCount = qMin(items.size(), 4);
    for (int i = 0; i < buttonCount; ++i) {
        auto *btn = makeToeButton(items[i], positions[i].x, positions[i].y, positions[i].d);
        btn->show();
        m_currentToeButtons.push_back(btn);

        // Center each label directly under its button instead of maintaining a separate coordinate table.
        // That avoids labels drifting out of alignment after a button-position change.
        int labelX = positions[i].x + positions[i].d / 2 - labelWidth / 2;
        int labelY = positions[i].y + positions[i].d + labelGap;

        auto *lbl = new QLabel(items[i].label, this);
        lbl->setGeometry(labelX, labelY, labelWidth, labelHeight);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color:#DCD5F5; font-size:11px;");
        lbl->show();
        m_currentToeLabels.push_back(lbl);
    }
}

void PawMenuWidget::showPage(int index)
{
    m_currentPage = index;
    layoutToeButtons(m_pages[index]);

    m_prevArrow->setVisible(index > 0);
    m_nextArrow->setVisible(index < m_pages.size() - 1);

    for (int i = 0; i < m_dots.size(); ++i) {
        m_dots[i]->setStyleSheet(
            i == index ? "background:#8065E8; border-radius:3px;"
                        : "background:#37336E; border-radius:3px;"
        );
    }
}

void PawMenuWidget::popupAt(const QPoint &anchorPos)
{
    QPoint target(anchorPos.x() - width() / 2, anchorPos.y() - height() - 12);

    if (QScreen *screen = QApplication::primaryScreen()) {
        QRect avail = screen->availableGeometry();
        target.setX(qBound(avail.left(), target.x(), avail.right() - width()));
        target.setY(qBound(avail.top(),  target.y(), avail.bottom() - height()));
    }

    move(target);
    showPage(0);
    setWindowOpacity(0.0);
    show();
    raise();
    qApp->installEventFilter(this);

    if (m_fadeAnim) m_fadeAnim->stop();
    m_fadeAnim = new QPropertyAnimation(this, "windowOpacity", this);
    m_fadeAnim->setDuration(160);
    m_fadeAnim->setStartValue(0.0);
    m_fadeAnim->setEndValue(1.0);
    m_fadeAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_fadeAnim->start();
}

void PawMenuWidget::hideWithAnimation()
{
    qApp->removeEventFilter(this);

    if (m_fadeAnim) m_fadeAnim->stop();
    m_fadeAnim = new QPropertyAnimation(this, "windowOpacity", this);
    m_fadeAnim->setDuration(120);
    m_fadeAnim->setStartValue(windowOpacity());
    m_fadeAnim->setEndValue(0.0);
    connect(m_fadeAnim, &QPropertyAnimation::finished, this, [this]() {
        hide();
    });
    m_fadeAnim->start();
}

bool PawMenuWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        QPoint globalPos = mouseEvent->globalPos();
        if (!geometry().contains(globalPos)) {
            hideWithAnimation();
        }
    }
    return QWidget::eventFilter(obj, event);
}
