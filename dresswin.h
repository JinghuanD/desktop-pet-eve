#ifndef DRESSWIN_H
#define DRESSWIN_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QCloseEvent>
#include <QHash>
#include <QVariantMap>
#include <QStringList>
#include <vector>


class DressWin : public QWidget
{
    Q_OBJECT

public:
    explicit DressWin(QWidget *parent = nullptr);
    void accept(std::vector<QPixmap> &body, std::vector<QPixmap> &ears,
                std::vector<QPixmap> &decs, int bodyNum, int earsNum, int decsNum);

    int getBodyNum() const;
    int getEarsNum() const;
    int getDecsNum() const;
    bool usesLayeredSelection() const;
    QHash<QString, QVariantMap> layerAdjustments() const;

signals:
    void closed();
    // Retained for main-window compatibility; the simplified version does not send layered assets.
    void layeredOutfitChanged(const QStringList &resourcePaths);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void buildOutfitCards();
    void selectOutfit(int index);
    void updatePreview();

    std::vector<QPixmap> m_body;
    std::vector<QPixmap> m_ears;
    std::vector<QPixmap> m_decs;
    int m_bodyNum = 0;
    int m_earsNum = 0;
    int m_decsNum = 0;
    bool m_cardsBuilt = false;

    QLabel *m_previewLabel = nullptr;
    QLabel *m_outfitNameLabel = nullptr;
    QGridLayout *m_cardGrid = nullptr;
    std::vector<QPushButton *> m_outfitCards;
};

#endif // DRESSWIN_H
