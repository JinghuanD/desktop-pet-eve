#ifndef HEALTHWIN_H
#define HEALTHWIN_H

#include <QWidget>
#include <QJsonObject>

class QDateEdit;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QTableWidget;
class QLabel;
class QPushButton;
class QNetworkAccessManager;

class HealthWin : public QWidget
{
    Q_OBJECT

public:
    explicit HealthWin(QWidget *parent = nullptr);

private:
    void buildUi();
    void loadStore();
    void saveStore() const;
    void loadDay();
    void saveDay();
    QString dayKey() const;
    void updateTotal();

    QJsonObject m_store;
    QDateEdit *m_dateEdit = nullptr;
    QComboBox *m_mealBox = nullptr;
    QLineEdit *m_foodEdit = nullptr;
    QDoubleSpinBox *m_calorieSpin = nullptr;
    QTableWidget *m_mealTable = nullptr;
    QLabel *m_totalLabel = nullptr;
    QSpinBox *m_waterSpin = nullptr;
    QSpinBox *m_stepsSpin = nullptr;
    QDoubleSpinBox *m_sleepSpin = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;
};

#endif // HEALTHWIN_H
