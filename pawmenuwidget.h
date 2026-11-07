#ifndef PAWMENUWIDGET_H
#define PAWMENUWIDGET_H

#include <QWidget>
#include <QVector>
#include <QString>

class QPushButton;
class QLabel;
class QPropertyAnimation;

// Configuration for one paw-button action.
struct PawItem {
    QString iconGlyph;   // Emoji icon matching the original card style.
    QString label;       // Small caption below the button.
    QString actionId;    // Identifier sent to Eve::onPawAction.
};

class PawMenuWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PawMenuWidget(QWidget *parent = nullptr);

    // Display the menu. anchorPos is a screen-space anchor, typically the pet's top center.
    void popupAt(const QPoint &anchorPos);
    void hideWithAnimation();

signals:
    void actionTriggered(const QString &actionId);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void buildPages();
    void showPage(int index);
    void layoutToeButtons(const QVector<PawItem> &items);
    QPushButton *makeToeButton(const PawItem &item, int x, int y, int diameter);

    QVector<QVector<PawItem>> m_pages;
    QVector<QPushButton *> m_currentToeButtons;
    QVector<QLabel *> m_currentToeLabels;
    QPushButton *m_palmButton = nullptr;
    QPushButton *m_prevArrow = nullptr;
    QPushButton *m_nextArrow = nullptr;
    QVector<QLabel *> m_dots;

    int m_currentPage = 0;
    QPropertyAnimation *m_fadeAnim = nullptr;
};

#endif // PAWMENUWIDGET_H
