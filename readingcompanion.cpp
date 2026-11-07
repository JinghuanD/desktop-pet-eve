#include "readingcompanion.h"

#include <QApplication>
#include <QFontMetrics>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QScreen>

#ifdef Q_OS_WIN
#include <windows.h>
#include <oleacc.h>
#include <psapi.h>
#include <oaidl.h>
#endif

namespace {
QString compactTitle(QString title, const QString &application)
{
    title.remove(QChar(0x200B)); // Edge inserts a zero-width space in “Microsoft Edge”.
    title = title.trimmed();
    const int pdfEnd = title.indexOf(QStringLiteral(".pdf"), 0, Qt::CaseInsensitive);
    if (pdfEnd >= 0) return title.left(pdfEnd + 4).trimmed();
    const QRegularExpression suffix(QStringLiteral("\\s*-\\s*")
                                    + QRegularExpression::escape(application)
                                    + QStringLiteral(".*$"), QRegularExpression::CaseInsensitiveOption);
    title.remove(suffix);
    return title.trimmed();
}

#ifdef Q_OS_WIN
QString foregroundProcessName(HWND window)
{
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    // Eve is a 32-bit Qt process while Edge is often 64-bit. GetModuleBaseNameW can fail
    // across bitness boundaries; GetProcessImageFileNameW does not enumerate remote modules.
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!process) return QString();
    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetProcessImageFileNameW(process, path, MAX_PATH);
    CloseHandle(process);
    if (length == 0) return QString();
    return QFileInfo(QString::fromWCharArray(path, int(length))).fileName().toLower();
}

QString nativeWindowTitle(HWND window)
{
    const int size = GetWindowTextLengthW(window);
    if (size <= 0) return QString();
    QVector<wchar_t> text(size + 1);
    GetWindowTextW(window, text.data(), size + 1);
    return QString::fromWCharArray(text.data());
}

void collectAccessibleNames(IAccessible *accessible, QStringList *names, int depth)
{
    if (!accessible || !names || depth > 3 || names->size() >= 100) return;
    VARIANT self;
    VariantInit(&self);
    self.vt = VT_I4;
    self.lVal = CHILDID_SELF;
    BSTR name = nullptr;
    if (SUCCEEDED(accessible->get_accName(self, &name)) && name) {
        names->append(QString::fromWCharArray(name));
        SysFreeString(name);
    }

    long childCount = 0;
    if (FAILED(accessible->get_accChildCount(&childCount)) || childCount <= 0) return;
    childCount = qMin<long>(childCount, 40);
    QVector<VARIANT> children;
    children.resize(int(childCount));
    for (int i = 0; i < children.size(); ++i) VariantInit(&children[i]);
    long obtained = 0;
    if (FAILED(AccessibleChildren(accessible, 0, childCount, children.data(), &obtained))) return;
    for (long i = 0; i < obtained && names->size() < 100; ++i) {
        VARIANT &child = children[int(i)];
        if (child.vt == VT_DISPATCH && child.pdispVal) {
            IAccessible *childAccessible = nullptr;
            if (SUCCEEDED(child.pdispVal->QueryInterface(IID_IAccessible, reinterpret_cast<void **>(&childAccessible)))) {
                collectAccessibleNames(childAccessible, names, depth + 1);
                childAccessible->Release();
            }
        } else {
            BSTR childName = nullptr;
            if (SUCCEEDED(accessible->get_accName(child, &childName)) && childName) {
                names->append(QString::fromWCharArray(childName));
                SysFreeString(childName);
            }
        }
        VariantClear(&child);
    }
}

bool readAccessiblePage(HWND window, int *page, int *total)
{
    IAccessible *accessible = nullptr;
    if (FAILED(AccessibleObjectFromWindow(window, OBJID_CLIENT, IID_IAccessible,
                                           reinterpret_cast<void **>(&accessible))) || !accessible) return false;
    QStringList names;
    collectAccessibleNames(accessible, &names, 0);
    accessible->Release();

    const QRegularExpression pagePattern(
        QStringLiteral("(?:page)?\\s*(\\d+)\\s*(?:of|/)\\s*(\\d+)"),
        QRegularExpression::CaseInsensitiveOption);
    for (const QString &name : names) {
        const QRegularExpressionMatch match = pagePattern.match(name);
        if (match.hasMatch()) {
            *page = match.captured(1).toInt();
            *total = match.captured(2).toInt();
            return *page > 0 && *total >= *page;
        }
    }
    return false;
}

bool dispatchGet(IDispatch *object, const wchar_t *property, VARIANT *result, VARIANT *argument = nullptr)
{
    if (!object || !result) return false;
    DISPID id = DISPID_UNKNOWN;
    LPOLESTR name = const_cast<LPOLESTR>(property);
    if (FAILED(object->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &id))) return false;
    DISPPARAMS params = {};
    if (argument) {
        params.rgvarg = argument;
        params.cArgs = 1;
    }
    VariantInit(result);
    return SUCCEEDED(object->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
                                    &params, result, nullptr, nullptr));
}

bool dispatchMethod(IDispatch *object, const wchar_t *method, VARIANT *result, VARIANT *argument = nullptr)
{
    if (!object || !result) return false;
    DISPID id = DISPID_UNKNOWN;
    LPOLESTR name = const_cast<LPOLESTR>(method);
    if (FAILED(object->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &id))) return false;
    DISPPARAMS params = {};
    if (argument) {
        params.rgvarg = argument;
        params.cArgs = 1;
    }
    VariantInit(result);
    return SUCCEEDED(object->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
                                    &params, result, nullptr, nullptr));
}

IDispatch *variantDispatch(VARIANT *value)
{
    if (!value || value->vt != VT_DISPATCH || !value->pdispVal) return nullptr;
    IDispatch *result = value->pdispVal;
    result->AddRef();
    return result;
}

bool readWordProgress(QString *name, int *page, int *total)
{
    CLSID clsid;
    if (FAILED(CLSIDFromProgID(L"Word.Application", &clsid))) return false;
    IUnknown *unknown = nullptr;
    if (FAILED(GetActiveObject(clsid, nullptr, &unknown)) || !unknown) return false;
    IDispatch *application = nullptr;
    const HRESULT query = unknown->QueryInterface(IID_IDispatch, reinterpret_cast<void **>(&application));
    unknown->Release();
    if (FAILED(query) || !application) return false;

    bool success = false;
    VARIANT documentValue;
    if (dispatchGet(application, L"ActiveDocument", &documentValue)) {
        IDispatch *document = variantDispatch(&documentValue);
        if (document) {
            VARIANT value;
            if (dispatchGet(document, L"Name", &value) && value.vt == VT_BSTR) *name = QString::fromWCharArray(value.bstrVal);
            VariantClear(&value);

            VARIANT statistic;
            VariantInit(&statistic);
            statistic.vt = VT_I4;
            statistic.lVal = 2; // WdStatistic::wdStatisticPages
            if (dispatchMethod(document, L"ComputeStatistics", &value, &statistic)) {
                if (value.vt == VT_I4) *total = value.lVal;
                VariantClear(&value);
            }
            document->Release();
            success = !name->isEmpty();
        }
        VariantClear(&documentValue);
    }

    VARIANT windowValue;
    if (dispatchGet(application, L"ActiveWindow", &windowValue)) {
        IDispatch *activeWindow = variantDispatch(&windowValue);
        if (activeWindow) {
            VARIANT selectionValue;
            if (dispatchGet(activeWindow, L"Selection", &selectionValue)) {
                IDispatch *selection = variantDispatch(&selectionValue);
                if (selection) {
                    VARIANT information;
                    VariantInit(&information);
                    information.vt = VT_I4;
                    information.lVal = 3; // WdInformation::wdActiveEndPageNumber
                    VARIANT value;
                    if (dispatchGet(selection, L"Information", &value, &information) && value.vt == VT_I4) *page = value.lVal;
                    VariantClear(&value);
                    selection->Release();
                }
                VariantClear(&selectionValue);
            }
            activeWindow->Release();
        }
        VariantClear(&windowValue);
    }
    application->Release();
    return success;
}
#endif
}

ReadingCompanion::ReadingCompanion(QWidget *parent) : QWidget(parent)
{
#ifdef Q_OS_WIN
    m_comInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
#endif
    setFixedSize(370, 142);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    m_refreshTimer.setInterval(1000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &ReadingCompanion::refreshDocument);
}

ReadingCompanion::~ReadingCompanion()
{
#ifdef Q_OS_WIN
    if (m_comInitialized) CoUninitialize();
#endif
}

void ReadingCompanion::followPet(const QRect &petRect)
{
    QScreen *screen = QGuiApplication::screenAt(petRect.center());
    if (!screen) screen = QApplication::primaryScreen();
    if (!screen) return;
    const QRect available = screen->availableGeometry();
    int x = petRect.center().x() - width() / 2;
    int y = petRect.top() - height() - 12;
    x = qBound(available.left(), x, available.right() - width() + 1);
    y = qMax(available.top(), y);
    move(x, y);
}

void ReadingCompanion::beginTracking()
{
    m_tracking = true;
    refreshDocument();
    m_refreshTimer.start();
    show();
    raise();
}

void ReadingCompanion::stopTracking()
{
    m_tracking = false;
    m_refreshTimer.stop();
    pauseClock();
    hide();
}

void ReadingCompanion::pauseClock()
{
    if (!m_clockRunning) return;
    if (m_readingTimer.isValid()) m_elapsedMs += m_readingTimer.elapsed();
    m_readingTimer.invalidate();
    m_clockRunning = false;
}

void ReadingCompanion::resumeClock()
{
    if (m_clockRunning) return;
    m_readingTimer.restart();
    m_clockRunning = true;
}

void ReadingCompanion::setWaitingState()
{
    // Pause when leaving PDF or Word; resume the same document session when returning.
    pauseClock();
    if (m_fileName.isEmpty()) {
        m_fileName = "Waiting for a PDF or Word document";
        m_page = 0;
        m_totalPages = 0;
    }
    update();
}

void ReadingCompanion::updateDocument(const QString &fileName, int page, int total, quintptr windowId)
{
    // Do not reset duration when the same document receives a new window handle.
    // Start a new reading session only after a different file is detected.
    if (m_fileName != fileName) {
        m_documentWindow = windowId;
        m_fileName = fileName;
        m_elapsedMs = 0;
        m_readingTimer.invalidate();
    }
    resumeClock();
    if (page > 0 && total > 0) {
        m_page = page;
        m_totalPages = total;
    }
    update();
}

void ReadingCompanion::refreshDocument()
{
#ifdef Q_OS_WIN
    HWND window = GetForegroundWindow();
    if (!window || reinterpret_cast<quintptr>(window) == reinterpret_cast<quintptr>(winId())) return;
    const QString process = foregroundProcessName(window);
    QString title = nativeWindowTitle(window);
    const QString plainTitle = QString(title).remove(QChar(0x200B));
    const bool isWord = process == QStringLiteral("winword.exe")
        || plainTitle.contains(QStringLiteral("Microsoft Word"), Qt::CaseInsensitive);
    const bool isEdge = process == QStringLiteral("msedge.exe")
        || (plainTitle.contains(QStringLiteral("Microsoft"), Qt::CaseInsensitive)
            && plainTitle.contains(QStringLiteral("Edge"), Qt::CaseInsensitive));
    if (!isWord && !isEdge) {
        setWaitingState();
        return;
    }
    int page = 0;
    int total = 0;
    QString fileName;
    if (isWord) {
        readWordProgress(&fileName, &page, &total);
        if (fileName.isEmpty()) fileName = compactTitle(title, QStringLiteral("Word"));
    } else {
        // Distinguish standard Edge web pages from documents.
        // Only actual PDF tabs create or switch reading sessions.
        if (plainTitle.indexOf(QStringLiteral(".pdf"), 0, Qt::CaseInsensitive) < 0) {
            setWaitingState();
            return;
        }
        fileName = compactTitle(title, QStringLiteral("Microsoft Edge"));
    }
    if (fileName.isEmpty()) {
        setWaitingState();
        return;
    }
    if (page <= 0 || total <= 0) readAccessiblePage(window, &page, &total);
    updateDocument(fileName, page, total, reinterpret_cast<quintptr>(window));
#else
    setWaitingState();
#endif
}

void ReadingCompanion::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRect card = rect().adjusted(5, 6, -5, -7);
    painter.setPen(QPen(QColor(181, 141, 82, 120), 1));
    painter.setBrush(QColor(255, 240, 207, 246));
    painter.drawRoundedRect(card, 16, 16);

    painter.setPen(QColor(103, 77, 46));
    QFont titleFont(QStringLiteral("Microsoft YaHei UI"), 10);
    painter.setFont(titleFont);
    painter.drawText(QRect(23, 18, 200, 28), Qt::AlignVCenter, "▱  Reading Mode");
    const QRect exitButton(width() - 72, 17, 48, 27);
    painter.setPen(QPen(QColor(170, 126, 67), 1));
    painter.setBrush(QColor(255, 231, 186));
    painter.drawRoundedRect(exitButton, 9, 9);
    painter.setPen(QColor(116, 80, 42));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 8));
    painter.drawText(exitButton, Qt::AlignCenter, "Exit");

    QFont fileFont(QStringLiteral("Microsoft YaHei UI"), 8);
    painter.setFont(fileFont);
    painter.setPen(QColor(137, 105, 65));
    const QString displayName = QFontMetrics(fileFont).elidedText(m_fileName, Qt::ElideRight, 310);
    painter.drawText(QRect(23, 48, 314, 20), Qt::AlignVCenter, displayName);

    QFont infoFont(QStringLiteral("Microsoft YaHei UI"), 8);
    painter.setFont(infoFont);
    painter.setPen(QColor(151, 109, 60));
    painter.drawText(QRect(23, 76, 82, 25), Qt::AlignVCenter, "Current page");
    QFont pageFont(QStringLiteral("Georgia"), 10, QFont::Bold);
    painter.setFont(pageFont);
    painter.setPen(QColor(107, 67, 29));
    const QString pageText = m_page > 0 && m_totalPages > 0
        ? QStringLiteral("%1 / %2").arg(m_page).arg(m_totalPages)
        : "Loading";
    painter.drawText(QRect(108, 74, 118, 28), Qt::AlignVCenter, pageText);

    const qint64 elapsedMs = m_elapsedMs + (m_clockRunning && m_readingTimer.isValid()
        ? m_readingTimer.elapsed() : 0);
    const qint64 seconds = elapsedMs / 1000;
    const QString elapsed = QStringLiteral("%1:%2:%3")
        .arg(seconds / 3600, 2, 10, QLatin1Char('0'))
        .arg((seconds / 60) % 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
    painter.drawText(QRect(width() - 118, 74, 92, 28), Qt::AlignRight | Qt::AlignVCenter, elapsed);

    const QRect track(23, 112, width() - 46, 8);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(216, 191, 143));
    painter.drawRoundedRect(track, 4, 4);
    const qreal fraction = m_totalPages > 0 ? qBound(0.0, qreal(m_page) / m_totalPages, 1.0) : 0.0;
    painter.setBrush(QColor(181, 139, 77));
    painter.drawRoundedRect(QRect(track.left(), track.top(), qRound(track.width() * fraction), track.height()), 4, 4);
    painter.setPen(QColor(125, 91, 49));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 8));
    const QString percent = m_totalPages > 0 ? QStringLiteral("%1%").arg(qRound(fraction * 100)) : QStringLiteral("--");
    painter.drawText(QRect(track.right() - 38, 116, 36, 18), Qt::AlignRight, percent);
}

void ReadingCompanion::mousePressEvent(QMouseEvent *event)
{
    const QRect exitButton(width() - 72, 17, 48, 27);
    if (event->button() == Qt::LeftButton && exitButton.contains(event->pos())) {
        emit exitRequested();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}
