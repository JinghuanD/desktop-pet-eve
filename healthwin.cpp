#include "healthwin.h"

#include <QDateEdit>
#include <QDate>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QCoreApplication>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QRegularExpression>
#include <QInputDialog>
#include <QUrlQuery>
#include <QtMath>
#include <QSslSocket>
#include <QTimer>
#include <QDialog>
#include <QDialogButtonBox>

namespace {
QString healthFilePath()
{
    return QCoreApplication::applicationDirPath() + "/health_records.json";
}

QString calorieText(double calories)
{
    return QString::number(calories, 'f', 2).remove(QRegularExpression("\\.?0+$"));
}
}

HealthWin::HealthWin(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    setWindowTitle("Health Manager");
    resize(640, 660);
    setMinimumSize(560, 580);
    buildUi();
    loadStore();
    loadDay();
}

void HealthWin::buildUi()
{
    setStyleSheet(R"(
        QWidget { background:#f7fff8; color:#38644c; font-family:"Microsoft YaHei"; }
        QLabel#title { color:#3f8b64; font-size:28px; font-weight:800; }
        QLabel#total { color:#e06d57; font-size:24px; font-weight:800; background:#fff4eb; border:2px solid #ffd3be; border-radius:14px; padding:8px; }
        QGroupBox { border:2px solid #b9e4c9; border-radius:14px; margin-top:12px; font-weight:700; color:#357a57; }
        QGroupBox::title { subcontrol-origin:margin; left:12px; padding:0 5px; }
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QDateEdit { background:white; border:1px solid #b8d8c1; border-radius:7px; padding:5px; }
        QPushButton { background:#5caf7a; color:white; border:0; border-radius:9px; font-weight:700; padding:8px 14px; }
        QPushButton:hover { background:#428e60; }
        QPushButton#remove { background:#e88775; }
        QTableWidget { background:white; border:1px solid #c5e3ce; gridline-color:#e1f2e6; }
        QHeaderView::section { background:#e0f4e6; color:#3c7654; border:0; padding:6px; font-weight:700; }
    )");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 16, 18, 18);
    root->setSpacing(12);
    auto *titleRow = new QHBoxLayout;
    auto *title = new QLabel("♡ Daily Health Manager", this);
    title->setObjectName("title");
    titleRow->addWidget(title);
    titleRow->addStretch();
    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    m_dateEdit->setToolTip("Choose a date to view or update its saved record.");
    titleRow->addWidget(m_dateEdit);
    root->addLayout(titleRow);

    auto *mealBox = new QGroupBox("Meals & Calories", this);
    auto *mealLayout = new QVBoxLayout(mealBox);
    auto *entry = new QHBoxLayout;
    m_mealBox = new QComboBox(mealBox);
    m_mealBox->addItems({"Breakfast", "Lunch", "Dinner", "Snack"});
    m_foodEdit = new QLineEdit(mealBox);
    m_foodEdit->setPlaceholderText("Food or recipe name");
    m_calorieSpin = new QDoubleSpinBox(mealBox);
    m_calorieSpin->setRange(0, 100000);
    m_calorieSpin->setDecimals(2);
    m_calorieSpin->setSingleStep(1.0);
    m_calorieSpin->setSuffix(" kcal");
    auto *add = new QPushButton("Add", mealBox);
    entry->addWidget(m_mealBox);
    entry->addWidget(m_foodEdit, 1);
    entry->addWidget(m_calorieSpin);
    entry->addWidget(add);
    mealLayout->addLayout(entry);
    auto *usdaRow = new QHBoxLayout;
    auto *weight = new QDoubleSpinBox(mealBox);
    weight->setRange(0.1, 10000.0);
    weight->setDecimals(1);
    weight->setValue(100.0);
    weight->setSuffix(" g");
    auto *usdaButton = new QPushButton("Find Calories (USDA)", mealBox);
    auto *usdaHint = new QLabel("Use an English food name, then select the best match.", mealBox);
    usdaRow->addWidget(new QLabel("Serving weight:", mealBox));
    usdaRow->addWidget(weight);
    usdaRow->addWidget(usdaButton);
    usdaRow->addWidget(usdaHint, 1);
    mealLayout->addLayout(usdaRow);
    m_mealTable = new QTableWidget(0, 3, mealBox);
    m_mealTable->setHorizontalHeaderLabels({"Meal", "Food / Recipe", "Calories"});
    m_mealTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_mealTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_mealTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_mealTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_mealTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mealLayout->addWidget(m_mealTable, 1);
    auto *mealBottom = new QHBoxLayout;
    m_totalLabel = new QLabel(mealBox);
    m_totalLabel->setObjectName("total");
    auto *editCalories = new QPushButton("Edit Calories", mealBox);
    auto *remove = new QPushButton("Remove Selected", mealBox);
    remove->setObjectName("remove");
    mealBottom->addWidget(m_totalLabel, 1);
    mealBottom->addWidget(editCalories);
    mealBottom->addWidget(remove);
    mealLayout->addLayout(mealBottom);
    root->addWidget(mealBox, 3);

    auto *habitBox = new QGroupBox("Daily Health Data", this);
    auto *habitForm = new QFormLayout(habitBox);
    m_waterSpin = new QSpinBox(habitBox);
    m_waterSpin->setRange(0, 10000);
    m_waterSpin->setSuffix(" ml");
    m_waterSpin->setSingleStep(100);
    m_stepsSpin = new QSpinBox(habitBox);
    m_stepsSpin->setRange(0, 100000);
    m_stepsSpin->setSuffix(" steps");
    m_stepsSpin->setSingleStep(500);
    m_sleepSpin = new QDoubleSpinBox(habitBox);
    m_sleepSpin->setRange(0, 24);
    m_sleepSpin->setDecimals(1);
    m_sleepSpin->setSingleStep(0.5);
    m_sleepSpin->setSuffix(" hours");
    habitForm->addRow("Water", m_waterSpin);
    habitForm->addRow("Steps", m_stepsSpin);
    habitForm->addRow("Sleep", m_sleepSpin);
    root->addWidget(habitBox);

    m_networkManager = new QNetworkAccessManager(this);

    connect(usdaButton, &QPushButton::clicked, this, [this, usdaButton, weight] {
        const QString food = m_foodEdit->text().trimmed();
        if (food.isEmpty()) {
            QMessageBox::information(this, "USDA Search", "Enter a food name first. English works best.");
            return;
        }
        if (!QSslSocket::supportsSsl()) {
            QMessageBox::warning(this, "USDA Search",
                "HTTPS is unavailable. Check that the matching OpenSSL 1.1 libraries (libssl-1_1.dll and libcrypto-1_1.dll) are in the application folder, then restart.");
            return;
        }
        QByteArray key = qgetenv("USDA_API_KEY").trimmed();
        if (key.isEmpty()) key = "DEMO_KEY";
        QUrl url("https://api.nal.usda.gov/fdc/v1/foods/search");
        QUrlQuery query;
        query.addQueryItem("api_key", QString::fromUtf8(key));
        url.setQuery(query);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Accept", "application/json");
        const QJsonObject body{{"query", food}, {"pageSize", 25},
                               {"dataType", QJsonArray{"Foundation", "SR Legacy", "Survey (FNDDS)"}}};
        usdaButton->setEnabled(false);
        usdaButton->setText("Searching…");
        QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(body).toJson());
        QTimer::singleShot(30000, reply, [reply] {
            if (reply->isRunning()) {
                reply->setProperty("usdaTimedOut", true);
                reply->abort();
            }
        });
        connect(reply, &QNetworkReply::finished, this, [this, reply, usdaButton, weight, food] {
            usdaButton->setEnabled(true);
            usdaButton->setText("Find Calories (USDA)");
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QByteArray data = reply->readAll();
            const bool failed = reply->error() != QNetworkReply::NoError;
            const QString errorDetail = reply->errorString();
            const bool timedOut = reply->property("usdaTimedOut").toBool();
            reply->deleteLater();
            if (failed) {
                const QString message = timedOut ? "The request timed out. Check your network or proxy and try again."
                    : status == 429 ? "Request limit reached. Try again later or set your own USDA_API_KEY."
                    : status == 403 || status == 401 ? "The API key is invalid or unavailable. Check USDA_API_KEY."
                    : QString("Could not connect to USDA (HTTP %1).\nDetails: %2\nCheck your network, proxy, and system time, then try again.").arg(status).arg(errorDetail);
                QMessageBox::warning(this, "USDA Search", message);
                return;
            }
            if (m_foodEdit->text().trimmed() != food) return;
            const QJsonDocument document = QJsonDocument::fromJson(data);
            if (!document.isObject() || !document.object().value("foods").isArray()) {
                QMessageBox::warning(this, "USDA Search", "The database returned an unrecognized response. Please try again later.");
                return;
            }
            QStringList choices;
            QList<double> energies;
            for (const QJsonValue &item : document.object().value("foods").toArray()) {
                const QJsonObject result = item.toObject();
                double energy = -1;
                int priority = 0;
                for (const QJsonValue &entry : result.value("foodNutrients").toArray()) {
                    const QJsonObject nutrient = entry.toObject();
                    const int id = nutrient.value("nutrientId").toInt();
                    const int candidate = id == 1008 ? 3 : id == 2047 ? 2 : id == 2048 ? 1 : 0;
                    if (candidate > priority && nutrient.value("unitName").toString().compare("kcal", Qt::CaseInsensitive) == 0
                        && nutrient.value("value").isDouble() && nutrient.value("value").toDouble() >= 0) {
                        energy = nutrient.value("value").toDouble();
                        priority = candidate;
                    }
                }
                if (energy < 0) continue;
                choices << QString("%1 — %2 kcal/100 g (%3, ID %4)")
                    .arg(result.value("description").toString()).arg(energy, 0, 'f', 1)
                    .arg(result.value("dataType").toString()).arg(result.value("fdcId").toInt());
                energies << energy;
            }
            if (choices.isEmpty()) {
                QMessageBox::information(this, "USDA Search", "No matching food with calorie data was found. Try an English name or enter calories manually.");
                return;
            }
            bool accepted = false;
            const QString selected = QInputDialog::getItem(this, "Choose a USDA food",
                "Choose the result closest to the food and preparation method:", choices, 0, false, &accepted);
            if (!accepted || m_foodEdit->text().trimmed() != food) return;
            const int index = choices.indexOf(selected);
            if (index < 0) return;
            const double calories = energies.at(index) * weight->value() / 100.0;
            if (calories > m_calorieSpin->maximum()) {
                QMessageBox::warning(this, "USDA Search", "The calculation is outside the recordable range. Check the serving weight.");
                return;
            }
            m_calorieSpin->setValue(calories);
            m_calorieSpin->setToolTip(QString("USDA: %1; serving: %2 g. Actual ingredients and preparation may vary.")
                                      .arg(selected).arg(weight->value()));
        });
    });

    connect(m_dateEdit, &QDateEdit::dateChanged, this, [this](const QDate &) { loadDay(); });
    connect(add, &QPushButton::clicked, this, [this] {
        const QString food = m_foodEdit->text().trimmed();
        if (food.isEmpty()) {
            QMessageBox::information(this, "Health Manager", "Enter a food or recipe name first.");
            return;
        }
        const int row = m_mealTable->rowCount();
        m_mealTable->insertRow(row);
        m_mealTable->setItem(row, 0, new QTableWidgetItem(m_mealBox->currentText()));
        m_mealTable->setItem(row, 1, new QTableWidgetItem(food));
        m_mealTable->setItem(row, 2, new QTableWidgetItem(calorieText(m_calorieSpin->value())));
        m_foodEdit->clear();
        m_calorieSpin->setValue(0);
        saveDay();
    });
    connect(remove, &QPushButton::clicked, this, [this] {
        const int row = m_mealTable->currentRow();
        if (row < 0) return;
        m_mealTable->removeRow(row);
        saveDay();
    });
    connect(editCalories, &QPushButton::clicked, this, [this] {
        const int row = m_mealTable->currentRow();
        if (row < 0) {
            QMessageBox::information(this, "Edit Calories", "Select a meal record in the table first.");
            return;
        }
        QTableWidgetItem *calorieItem = m_mealTable->item(row, 2);
        if (!calorieItem) return;
        QDialog dialog(this);
        dialog.setWindowTitle("Edit Calories");
        auto *layout = new QFormLayout(&dialog);
        auto *amount = new QDoubleSpinBox(&dialog);
        amount->setRange(0, 100000000);
        amount->setDecimals(2);
        amount->setValue(calorieItem->text().toDouble());
        auto *unit = new QComboBox(&dialog);
        unit->addItems({"kcal", "cal"});
        layout->addRow("Amount:", amount);
        layout->addRow("Unit:", unit);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        layout->addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() != QDialog::Accepted) return;
        const double calories = amount->value() / (unit->currentText() == "cal" ? 1000.0 : 1.0);
        if (calories > 100000) {
            QMessageBox::warning(this, "Edit Calories", "The converted kcal value is outside the recordable range.");
            return;
        }
        calorieItem->setText(calorieText(calories));
        saveDay();
    });
    connect(m_waterSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { saveDay(); });
    connect(m_stepsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { saveDay(); });
    connect(m_sleepSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) { saveDay(); });
}

QString HealthWin::dayKey() const { return m_dateEdit->date().toString(Qt::ISODate); }

void HealthWin::loadStore()
{
    QFile file(healthFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (document.isObject()) m_store = document.object();
}

void HealthWin::saveStore() const
{
    QFile file(healthFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) file.write(QJsonDocument(m_store).toJson(QJsonDocument::Indented));
}

void HealthWin::loadDay()
{
    const QJsonObject day = m_store.value(dayKey()).toObject();
    m_mealTable->setRowCount(0);
    const QJsonArray meals = day.value("meals").toArray();
    for (const QJsonValue &value : meals) {
        const QJsonObject meal = value.toObject();
        const int row = m_mealTable->rowCount();
        m_mealTable->insertRow(row);
        m_mealTable->setItem(row, 0, new QTableWidgetItem(meal.value("meal").toString()));
        m_mealTable->setItem(row, 1, new QTableWidgetItem(meal.value("food").toString()));
        m_mealTable->setItem(row, 2, new QTableWidgetItem(calorieText(meal.value("calories").toDouble())));
    }
    m_waterSpin->blockSignals(true); m_stepsSpin->blockSignals(true); m_sleepSpin->blockSignals(true);
    m_waterSpin->setValue(day.value("water").toInt());
    m_stepsSpin->setValue(day.value("steps").toInt());
    m_sleepSpin->setValue(day.value("sleep").toDouble());
    m_waterSpin->blockSignals(false); m_stepsSpin->blockSignals(false); m_sleepSpin->blockSignals(false);
    updateTotal();
}

void HealthWin::saveDay()
{
    QJsonArray meals;
    for (int row = 0; row < m_mealTable->rowCount(); ++row) {
        meals.append(QJsonObject{{"meal", m_mealTable->item(row, 0)->text()}, {"food", m_mealTable->item(row, 1)->text()}, {"calories", m_mealTable->item(row, 2)->text().toDouble()}});
    }
    m_store.insert(dayKey(), QJsonObject{{"meals", meals}, {"water", m_waterSpin->value()}, {"steps", m_stepsSpin->value()}, {"sleep", m_sleepSpin->value()}});
    saveStore();
    updateTotal();
}

void HealthWin::updateTotal()
{
    double total = 0;
    for (int row = 0; row < m_mealTable->rowCount(); ++row) total += m_mealTable->item(row, 2)->text().toDouble();
    m_totalLabel->setText(QString("Total: %1 kcal").arg(calorieText(total)));
}
