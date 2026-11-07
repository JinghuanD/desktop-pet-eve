#ifndef GAMEWIN_H
#define GAMEWIN_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QThread>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QString>
#include <QMessageBox>
#include <QFileInfo>
#include <QTabWidget>
#include <QFormLayout>
#include <QNetworkInterface>
#include <QHash>
#include <QSet>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStandardPaths>

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <wbemidl.h>
#include <shellapi.h>
#include <algorithm>




class GameWin : public QDialog
{
    Q_OBJECT


private slots:
    void onOpenAccelerator();
    void refreshAll();


public:
    explicit GameWin(QWidget *parent = nullptr);
    ~GameWin();


private:
    void openUU();
    QLabel        *m_cpuTempLabel;
    QLabel        *m_networkLabel;
    QLabel        *m_hardwareLabel;
    QLabel        *m_gameHintLabel;
    QPushButton   *m_accelBtn;
    QTableWidget  *m_processTable;
    QTableWidget  *m_gameTimeTable;
    QTimer        *m_refreshTimer;
    QHash<QString, qint64> m_gameSeconds;
    QString        m_activeProgram;
    qint64         m_lastUsageTickMs = 0;

    // ── WMI ─────────────────────────────────────────────────────────────
    IWbemLocator  *m_wbemLocator  = nullptr;
    IWbemServices *m_wbemServices = nullptr;
    bool           m_wmiReady     = false;

    bool   initWmi();
    void   cleanupWmi();
    double queryCpuTemp();   // Returns Celsius; -1 on failure.

    // ── Processes ──────────────────────────────────────────────────────
    struct ProcInfo  { QString name; DWORD pid; double cpuPercent; };
    struct ProcSample{ DWORD pid; FILETIME kernelTime, userTime, sampleTime; };
    QList<ProcSample> m_lastSamples;

    QList<ProcInfo> getProcessList();
    static double   calcCpuPercent(const ProcSample &prev, const ProcSample &curr);

    void refreshNetworkStatus();
    void refreshHardwareStatus();
    void updateGameUsage();
    void loadGameUsage();
    void saveGameUsage() const;
    static bool isGameCandidate(const QString &executableName);




};

#endif // CALENWIN_H
