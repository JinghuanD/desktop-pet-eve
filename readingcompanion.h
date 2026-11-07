#ifndef READINGCOMPANION_H
#define READINGCOMPANION_H

#include <QElapsedTimer>
#include <QRect>
#include <QString>
#include <QTimer>
#include <QWidget>

// A reading-status card displayed above the pet. It never takes focus; it observes the
// foreground Word or Edge document and visualizes its available file name, page, and duration.
class ReadingCompanion : public QWidget
{
    Q_OBJECT
public:
    explicit ReadingCompanion(QWidget *parent = nullptr);
    ~ReadingCompanion() override;

    void followPet(const QRect &petRect);
    void beginTracking();
    void stopTracking();

signals:
    void exitRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void refreshDocument();

private:
    void setWaitingState();
    void updateDocument(const QString &fileName, int page, int total, quintptr windowId);
    void pauseClock();
    void resumeClock();

    QTimer m_refreshTimer;
    QElapsedTimer m_readingTimer;
    QString m_fileName;
    int m_page = 0;
    int m_totalPages = 0;
    quintptr m_documentWindow = 0;
    bool m_tracking = false;
    bool m_clockRunning = false;
    bool m_comInitialized = false;
    qint64 m_elapsedMs = 0;
};

#endif // READINGCOMPANION_H
