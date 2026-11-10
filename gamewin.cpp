#include "gamewin.h"
#include "ui_gamewin.h"

#include <QDir>
#include <QJsonArray>


// ── Accelerator path ────────────────────────────────────────────────────────
static const QString kAccelPath = R"(E:\LieBao.exe)";

// ════════════════════════════════════════════════════════════════════════════
//  Construction / destruction




GameWin::GameWin(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("System Panel");
    setMinimumWidth(540);
    setStyleSheet(R"(
        QDialog             { background: #1a1a28; }
        QLabel              { color: #c8c0e8; font-size: 13px; }
        QPushButton         { background:#3a2fa0; color:#fff; border:none;
                              border-radius:6px; padding:8px 20px;
                              font-size:13px; font-weight:bold; }
        QPushButton:hover   { background:#5040c8; }
        QPushButton:pressed { background:#7060e0; }
        QTableWidget        { background:#12121e; color:#d0c8f0;
                              gridline-color:#2a2a40; border:1px solid #2a2a40;
                              border-radius:4px; font-size:12px; }
        QHeaderView::section{ background:#24243a; color:#a090d0;
                              border:none; padding:4px; font-weight:bold; }
        QTableWidget::item:selected { background:#3a2fa0; }
    )");

    // ── CPU temperature label ───────────────────────────────────────────
    m_cpuTempLabel = new QLabel("CPU Temperature: Reading…");
    m_cpuTempLabel->setStyleSheet("color:#80d0a0; font-size:14px; font-weight:bold;");

    // ── Open accelerator button ─────────────────────────────────────────
    m_accelBtn = new QPushButton("🚀  Open Accelerator");
    m_accelBtn->setFixedHeight(38);
    connect(m_accelBtn, &QPushButton::clicked, this, &GameWin::onOpenAccelerator);

    auto *topLayout = new QHBoxLayout;
    topLayout->addWidget(m_cpuTempLabel, 1);
    topLayout->addWidget(m_accelBtn);

    // ── Process table ───────────────────────────────────────────────────
    m_processTable = new QTableWidget(0, 3);
    m_processTable->setHorizontalHeaderLabels({"Process", "PID", "CPU %"});
    m_processTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_processTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_processTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_processTable->setColumnWidth(1, 75);
    m_processTable->setColumnWidth(2, 75);
    m_processTable->verticalHeader()->setVisible(false);
    m_processTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_processTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_processTable->setAlternatingRowColors(true);
    m_processTable->setStyleSheet(
        "QTableWidget { alternate-background-color: #16162a; }");

    // ── Performance page ────────────────────────────────────────────────
    auto *performancePage = new QWidget(this);
    auto *layout = new QVBoxLayout(performancePage);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    layout->addLayout(topLayout);
    layout->addWidget(new QLabel("Current processes:"));
    layout->addWidget(m_processTable);

    auto *networkPage = new QWidget(this);
    auto *networkLayout = new QVBoxLayout(networkPage);
    m_networkLabel = new QLabel(networkPage);
    m_networkLabel->setWordWrap(true);
    m_networkLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    networkLayout->addWidget(new QLabel("Current network status:", networkPage));
    networkLayout->addWidget(m_networkLabel);
    networkLayout->addStretch();

    auto *hardwarePage = new QWidget(this);
    auto *hardwareLayout = new QVBoxLayout(hardwarePage);
    m_hardwareLabel = new QLabel(hardwarePage);
    m_hardwareLabel->setWordWrap(true);
    m_hardwareLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    hardwareLayout->addWidget(new QLabel("Hardware overview:", hardwarePage));
    hardwareLayout->addWidget(m_hardwareLabel);
    hardwareLayout->addStretch();

    auto *usagePage = new QWidget(this);
    auto *usageLayout = new QVBoxLayout(usagePage);
    m_gameHintLabel = new QLabel("Only foreground games while Eve is running are tracked; past history is not imported.", usagePage);
    m_gameHintLabel->setWordWrap(true);
    m_gameHintLabel->setStyleSheet("color:#a090d0;");
    m_gameTimeTable = new QTableWidget(0, 2, usagePage);
    m_gameTimeTable->setHorizontalHeaderLabels({"Recent games", "Total time"});
    m_gameTimeTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_gameTimeTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_gameTimeTable->verticalHeader()->setVisible(false);
    m_gameTimeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    usageLayout->addWidget(m_gameHintLabel);
    usageLayout->addWidget(m_gameTimeTable);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(performancePage, "Performance");
    tabs->addTab(networkPage, "Network");
    tabs->addTab(hardwarePage, "Hardware");
    tabs->addTab(usagePage, "Game Time");
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(10, 10, 10, 10);
    rootLayout->addWidget(tabs);

    // ── Initialize WMI ──────────────────────────────────────────────────
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    m_wmiReady = initWmi();
    if (!m_wmiReady)
        m_cpuTempLabel->setText("CPU Temperature: WMI initialization failed");

    // ── Periodic refresh (every 2 seconds) ──────────────────────────────
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &GameWin::refreshAll);
    loadGameUsage();
    m_refreshTimer->start(2000);
    refreshAll();
}

GameWin::~GameWin()
{
    m_refreshTimer->stop();
    saveGameUsage();
    cleanupWmi();
    CoUninitialize();
}

// ════════════════════════════════════════════════════════════════════════════
//  WMI initialization
// ════════════════════════════════════════════════════════════════════════════
bool GameWin::initWmi()
{
    HRESULT hr;

    hr = CoInitializeSecurity(
        nullptr, -1, nullptr, nullptr,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_NONE, nullptr);
    // E_ALREADY_INITIALIZED is normal because Qt may have initialized COM.
    if (FAILED(hr) && hr != RPC_E_TOO_LATE) return false;

    hr = CoCreateInstance(
        CLSID_WbemLocator, nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        reinterpret_cast<void**>(&m_wbemLocator));
    if (FAILED(hr)) return false;

    // Connect to the WMI namespace. CPU temperature is in root\WMI (MSAcpi_ThermalZoneTemperature).
    hr = m_wbemLocator->ConnectServer(
   const_cast<BSTR> (L"ROOT\\WMI"),
        nullptr, nullptr, nullptr, 0, nullptr, nullptr,
        &m_wbemServices);
    if (FAILED(hr)) return false;

    hr = CoSetProxyBlanket(
        m_wbemServices,
        RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_NONE);
    if (FAILED(hr)) return false;

    return true;
}

void GameWin::cleanupWmi()
{
    if (m_wbemServices) { m_wbemServices->Release(); m_wbemServices = nullptr; }
    if (m_wbemLocator)  { m_wbemLocator->Release();  m_wbemLocator  = nullptr; }
}

// ════════════════════════════════════════════════════════════════════════════
//  Query CPU temperature through WMI.
//  Source: MSAcpi_ThermalZoneTemperature (Kelvin × 10).
//  Conversion: Celsius = (value - 2732) / 10.
// ════════════════════════════════════════════════════════════════════════════
double GameWin::queryCpuTemp()
{
    if (!m_wmiReady || !m_wbemServices) return -1.0;

    IEnumWbemClassObject *pEnum = nullptr;
    HRESULT hr = m_wbemServices->ExecQuery(
              const_cast<BSTR>( L"WQL") ,
               const_cast<BSTR>(L"SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature"),
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                nullptr, &pEnum);
    if (FAILED(hr) || !pEnum) return -1.0;

    double tempC = -1.0;
    IWbemClassObject *pObj = nullptr;
    ULONG returned = 0;

    // Use the first record, which is usually the CPU thermal zone.
    if (pEnum->Next(WBEM_INFINITE, 1, &pObj, &returned) == S_OK && returned > 0) {
        VARIANT v;
        VariantInit(&v);
        if (SUCCEEDED(pObj->Get(L"CurrentTemperature", 0, &v, nullptr, nullptr))
            && v.vt == VT_I4)
        {
            // Kelvin × 10 → Celsius
            tempC = (v.lVal - 2732) / 10.0;
        }
        VariantClear(&v);
        pObj->Release();
    }
    pEnum->Release();
    return tempC;
}

// ════════════════════════════════════════════════════════════════════════════
//  Refresh temperature and process list.
// ════════════════════════════════════════════════════════════════════════════
void GameWin::refreshAll()
{
    refreshNetworkStatus();
    refreshHardwareStatus();
    updateGameUsage();

    // ── CPU temperature ─────────────────────────────────────────────────
    double temp = queryCpuTemp();
    if (temp >= 0) {
        QString color = temp >= 85 ? "#ff4040"
                      : temp >= 70 ? "#ffaa30"
                      :              "#80d0a0";
        m_cpuTempLabel->setStyleSheet(
            QString("color:%1; font-size:14px; font-weight:bold;").arg(color));
        m_cpuTempLabel->setText(
            QString("CPU Temperature: %1 °C").arg(temp, 0, 'f', 1));
    } else {
        m_cpuTempLabel->setText("CPU Temperature: unavailable (administrator permission may be required)");
    }

    // ── Process list ────────────────────────────────────────────────────
    auto procs = getProcessList();
    std::sort(procs.begin(), procs.end(), [](const ProcInfo &a, const ProcInfo &b){
        return a.cpuPercent > b.cpuPercent;
    });

    m_processTable->setRowCount(procs.size());
    for (int row = 0; row < procs.size(); ++row) {
        const auto &p = procs[row];

        auto *nameItem = new QTableWidgetItem(p.name);
        auto *pidItem  = new QTableWidgetItem(QString::number(p.pid));
        auto *cpuItem  = new QTableWidgetItem(
            p.cpuPercent > 0.05
                ? QString::number(p.cpuPercent, 'f', 1) + " %"
                : "—");
        pidItem->setTextAlignment(Qt::AlignCenter);
        cpuItem->setTextAlignment(Qt::AlignCenter);

        if      (p.cpuPercent >= 10.0) cpuItem->setForeground(QColor("#ff6060"));
        else if (p.cpuPercent >=  2.0) cpuItem->setForeground(QColor("#ffd060"));

        m_processTable->setItem(row, 0, nameItem);
        m_processTable->setItem(row, 1, pidItem);
        m_processTable->setItem(row, 2, cpuItem);
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  Open accelerator
// ════════════════════════════════════════════════════════════════════════════
void GameWin::onOpenAccelerator()
{
    if (!QFileInfo::exists(kAccelPath)) {
        QMessageBox::warning(this, "Error",
            QString("Accelerator not found. Check this path:\n%1").arg(kAccelPath));
        return;
    }
    ShellExecuteW(
        reinterpret_cast<HWND>(winId()),
        L"open",
        kAccelPath.toStdWString().c_str(),
        nullptr, nullptr, SW_SHOWNORMAL);
}

// ════════════════════════════════════════════════════════════════════════════
//  Process list (snapshot plus CPU-time deltas)
// ════════════════════════════════════════════════════════════════════════════
QList<GameWin::ProcInfo> GameWin::getProcessList()
{
    FILETIME now; GetSystemTimeAsFileTime(&now);
    QList<ProcSample> currentSamples;

    // First pass: collect CPU time.
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return {};
    PROCESSENTRY32W pe; pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
            if (!h) continue;
            FILETIME ct, et, kt, ut;
            if (GetProcessTimes(h, &ct, &et, &kt, &ut))
                currentSamples.append({pe.th32ProcessID, kt, ut, now});
            CloseHandle(h);
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);

    // Second pass: build results with process names.
    QList<ProcInfo> result;
    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    pe.dwSize = sizeof(pe);
    if (snap != INVALID_HANDLE_VALUE && Process32FirstW(snap, &pe)) {
        do {
            ProcInfo info;
            info.name = QString::fromWCharArray(pe.szExeFile);
            info.pid  = pe.th32ProcessID;
            info.cpuPercent = 0.0;

            auto itC = std::find_if(currentSamples.begin(), currentSamples.end(),
                [&](const ProcSample &s){ return s.pid == pe.th32ProcessID; });
            auto itP = std::find_if(m_lastSamples.begin(), m_lastSamples.end(),
                [&](const ProcSample &s){ return s.pid == pe.th32ProcessID; });
            if (itC != currentSamples.end() && itP != m_lastSamples.end())
                info.cpuPercent = calcCpuPercent(*itP, *itC);

            result.append(info);
        } while (Process32NextW(snap, &pe));
        CloseHandle(snap);
    }

    m_lastSamples = currentSamples;
    return result;
}

double GameWin::calcCpuPercent(const ProcSample &prev, const ProcSample &curr)
{
    auto ft2u64 = [](const FILETIME &ft) -> quint64 {
        return (quint64(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };
    quint64 cpuDelta  = (ft2u64(curr.kernelTime) + ft2u64(curr.userTime))
                      - (ft2u64(prev.kernelTime) + ft2u64(prev.userTime));
    quint64 timeDelta = ft2u64(curr.sampleTime) - ft2u64(prev.sampleTime);
    if (timeDelta == 0) return 0.0;
    return double(cpuDelta) / double(timeDelta) / QThread::idealThreadCount() * 100.0;
}

void GameWin::refreshNetworkStatus()
{
    QStringList lines;
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        const auto flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) ||
            !(flags & QNetworkInterface::IsRunning) ||
            (flags & QNetworkInterface::IsLoopBack)) {
            continue;
        }
        QStringList addresses;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol &&
                !entry.ip().isNull()) {
                addresses << entry.ip().toString();
            }
        }
        lines << QString("%1\nIPv4：%2\nMAC：%3")
                     .arg(iface.humanReadableName(),
                          addresses.isEmpty() ? "Unassigned" : addresses.join(", "),
                          iface.hardwareAddress());
    }
    m_networkLabel->setText(lines.isEmpty()
        ? "No connected network adapter detected."
        : lines.join("\n\n"));
}

void GameWin::refreshHardwareStatus()
{
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    MEMORYSTATUSEX memoryStatus{};
    memoryStatus.dwLength = sizeof(memoryStatus);
    GlobalMemoryStatusEx(&memoryStatus);

    DISPLAY_DEVICEW displayDevice{};
    displayDevice.cb = sizeof(displayDevice);
    QString gpu = "Not detected";
    if (EnumDisplayDevicesW(nullptr, 0, &displayDevice, 0)) {
        gpu = QString::fromWCharArray(displayDevice.DeviceString);
    }

    const double totalGiB = double(memoryStatus.ullTotalPhys) / 1024.0 / 1024.0 / 1024.0;
    const double availableGiB = double(memoryStatus.ullAvailPhys) / 1024.0 / 1024.0 / 1024.0;
    m_hardwareLabel->setText(QString("CPU logical cores: %1\nMemory: %2 GiB (%3 GiB available)\nGPU: %4")
        .arg(systemInfo.dwNumberOfProcessors)
        .arg(totalGiB, 0, 'f', 1)
        .arg(availableGiB, 0, 'f', 1)
        .arg(gpu));
}

bool GameWin::isGameCandidate(const QString &executableName)
{
    static const QSet<QString> excluded = {
        "explorer.exe", "eve.exe", "code.exe", "devenv.exe", "cmd.exe",
        "powershell.exe", "windowsterminal.exe", "chrome.exe", "msedge.exe",
        "firefox.exe", "discord.exe", "steam.exe", "steamwebhelper.exe"
    };
    return executableName.endsWith(".exe", Qt::CaseInsensitive)
        && !excluded.contains(executableName.toLower());
}

void GameWin::updateGameUsage()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_lastUsageTickMs > 0 && !m_activeProgram.isEmpty()) {
        m_gameSeconds[m_activeProgram] += (now - m_lastUsageTickMs) / 1000;
    }
    m_lastUsageTickMs = now;
    m_activeProgram.clear();

    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    if (pid != 0) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        if (snap != INVALID_HANDLE_VALUE && Process32FirstW(snap, &pe)) {
            do {
                if (pe.th32ProcessID == pid) {
                    const QString name = QString::fromWCharArray(pe.szExeFile);
                    if (isGameCandidate(name)) m_activeProgram = name;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        if (snap != INVALID_HANDLE_VALUE) CloseHandle(snap);
    }

    QList<QPair<QString, qint64>> entries;
    for (auto it = m_gameSeconds.cbegin(); it != m_gameSeconds.cend(); ++it) {
        if (it.value() > 0) entries.append(qMakePair(it.key(), it.value()));
    }
    std::sort(entries.begin(), entries.end(), [](const QPair<QString, qint64> &a,
                                                   const QPair<QString, qint64> &b) {
        return a.second > b.second;
    });

    m_gameTimeTable->setRowCount(entries.size());
    for (int row = 0; row < entries.size(); ++row) {
        const qint64 seconds = entries[row].second;
        const QString duration = QString("%1 hours %2 minutes")
            .arg(seconds / 3600)
            .arg((seconds % 3600) / 60, 2, 10, QLatin1Char('0'));
        m_gameTimeTable->setItem(row, 0, new QTableWidgetItem(entries[row].first));
        m_gameTimeTable->setItem(row, 1, new QTableWidgetItem(duration));
    }
}

void GameWin::loadGameUsage()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(QDir(dir).filePath("game_usage.json"));
    if (!file.open(QIODevice::ReadOnly)) return;
    const QJsonObject values = QJsonDocument::fromJson(file.readAll()).object();
    for (auto it = values.begin(); it != values.end(); ++it) {
        m_gameSeconds.insert(it.key(), static_cast<qint64>(it.value().toDouble()));
    }
}

void GameWin::saveGameUsage() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QJsonObject values;
    for (auto it = m_gameSeconds.cbegin(); it != m_gameSeconds.cend(); ++it) {
        values.insert(it.key(), static_cast<double>(it.value()));
    }
    QFile file(QDir(dir).filePath("game_usage.json"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(values).toJson());
    }
}
