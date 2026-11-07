#include "dresswin.h"

#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace {
const QStringList kOutfitNames = {
    QString::fromUtf8("Mid Autumn Set"),
    QString::fromUtf8("Rabbit Pajama"),
    QString::fromUtf8("Kitty Pajama")
};
}

DressWin::DressWin(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowStaysOnTopHint);
    setWindowTitle(QString::fromUtf8("Change"));
    resize(960, 680);
    setMinimumSize(820, 580);
    buildUi();
}

void DressWin::buildUi()
{
    setStyleSheet(R"(
        QWidget { background:#fff7fb; color:#765277; font-family:"Microsoft YaHei"; }
        QLabel#title { color:#9b4c7f; font-size:38px; font-weight:800; }
        QLabel#hint { color:#a4698b; font-size:16px; font-weight:700; }
        QLabel#preview { background:qradialgradient(cx:0.5, cy:0.42, radius:0.7, stop:0 #fff9ff, stop:0.7 #f5dff3, stop:1 #e7c8e8); border:3px solid #e6a9cb; border-radius:28px; }
        QLabel#outfitName { color:#9a4e7c; font-size:22px; font-weight:700; }
        QPushButton#card { background:#fffaff; border:2px solid #efd6e4; border-radius:18px; color:#855676; font-size:16px; font-weight:700; padding:10px; }
        QPushButton#card:hover { border-color:#ec8cb9; background:#fff1f7; }
        QPushButton#card:checked { border:3px solid #ef74ad; background:#ffeaf4; }
        QPushButton#close { background:#fff0f6; color:#aa4f81; border:2px solid #e9a8c7; border-radius:20px; font-size:26px; font-weight:800; }
        QPushButton#close:hover { background:#f19ac1; color:white; }
    )");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(26, 16, 26, 26);
    root->setSpacing(12);
    auto *header = new QHBoxLayout;
    auto *title = new QLabel(QString::fromUtf8("✿  Change"), this);
    title->setObjectName("title");
    header->addWidget(title);
    header->addStretch();
    auto *closeButton = new QPushButton(QString::fromUtf8("×"), this);
    closeButton->setObjectName("close");
    closeButton->setFixedSize(42, 42);
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
    header->addWidget(closeButton);
    root->addLayout(header);


    auto *content = new QHBoxLayout;
    content->setSpacing(24);
    m_previewLabel = new QLabel(this);
    m_previewLabel->setObjectName("preview");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumSize(410, 500);
    content->addWidget(m_previewLabel, 5);

    auto *choicePanel = new QVBoxLayout;
    auto *choiceTitle = new QLabel(QString::fromUtf8("Select Set"), this);
    choiceTitle->setObjectName("hint");
    choiceTitle->setAlignment(Qt::AlignCenter);
    choicePanel->addWidget(choiceTitle);
    auto *gridHost = new QWidget(this);
    m_cardGrid = new QGridLayout(gridHost);
    m_cardGrid->setContentsMargins(0, 0, 0, 0);
    m_cardGrid->setSpacing(14);
    choicePanel->addWidget(gridHost, 1);
    content->addLayout(choicePanel, 5);
    root->addLayout(content, 1);

    m_outfitNameLabel = new QLabel(this);
    m_outfitNameLabel->setObjectName("outfitName");
    m_outfitNameLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_outfitNameLabel);
}

void DressWin::accept(std::vector<QPixmap> &body, std::vector<QPixmap> &ears,
                      std::vector<QPixmap> &decs, int bodyNum, int earsNum, int decsNum)
{
    m_body = body;
    m_ears = ears;
    m_decs = decs;
    m_bodyNum = qBound(0, bodyNum, qMax(0, static_cast<int>(m_body.size()) - 1));
    m_earsNum = qBound(0, earsNum, qMax(0, static_cast<int>(m_ears.size()) - 1));
    m_decsNum = qBound(0, decsNum, qMax(0, static_cast<int>(m_decs.size()) - 1));
    if (!m_cardsBuilt) buildOutfitCards();
    selectOutfit(m_bodyNum);
}

void DressWin::buildOutfitCards()
{
    if (!m_cardGrid) return;
    m_cardsBuilt = true;
    for (int index = 0; index < static_cast<int>(m_body.size()); ++index) {
        auto *card = new QPushButton(this);
        card->setObjectName("card");
        card->setCheckable(true);
        card->setMinimumSize(205, 160);
        card->setText(index < kOutfitNames.size() ? kOutfitNames[index] : QString::fromUtf8("Complete Set"));
        card->setIcon(QIcon(m_body[index]));
        card->setIconSize(QSize(150, 124));
        connect(card, &QPushButton::clicked, this, [this, index] { selectOutfit(index); });
        m_cardGrid->addWidget(card, index, 0);
        m_outfitCards.push_back(card);
    }
}

void DressWin::selectOutfit(int index)
{
    if (index < 0 || index >= static_cast<int>(m_body.size())) return;
    m_bodyNum = m_earsNum = m_decsNum = index;
    for (int i = 0; i < static_cast<int>(m_outfitCards.size()); ++i)
        m_outfitCards[i]->setChecked(i == index);
    updatePreview();
}

void DressWin::updatePreview()
{
    if (m_body.empty() || !m_previewLabel) return;
    const QSize previewSize = m_previewLabel->size() - QSize(26, 26);
    m_previewLabel->setPixmap(m_body[m_bodyNum].scaled(previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_outfitNameLabel->setText(m_bodyNum < kOutfitNames.size() ? kOutfitNames[m_bodyNum] : QString::fromUtf8("Complete Set"));
}

int DressWin::getBodyNum() const { return m_bodyNum; }
int DressWin::getEarsNum() const { return m_earsNum; }
int DressWin::getDecsNum() const { return m_decsNum; }
bool DressWin::usesLayeredSelection() const { return false; }
QHash<QString, QVariantMap> DressWin::layerAdjustments() const { return {}; }

void DressWin::closeEvent(QCloseEvent *event)
{
    emit closed();
    event->accept();
}
