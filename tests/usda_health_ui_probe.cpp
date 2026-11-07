#include "../healthwin.h"
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTextStream>
#include <QTimer>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    HealthWin window;
    window.show();
    auto *food = window.findChild<QLineEdit *>();
    QPushButton *search = nullptr;
    for (auto *button : window.findChildren<QPushButton *>())
        if (button->text().contains("USDA")) search = button;
    if (!food || !search) return 2;
    food->setText("apple");
    QTimer watch;
    QObject::connect(&watch, &QTimer::timeout, [&] {
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            auto *dialog = qobject_cast<QInputDialog *>(widget);
            if (!dialog || !dialog->isVisible()) continue;
            auto *list = dialog->findChild<QComboBox *>();
            if (!list || list->count() == 0) { app.exit(3); return; }
            QTextStream(stdout) << "First result: " << list->itemText(0) << "\n";
            dialog->accept();
            QTimer::singleShot(0, [&] {
                for (auto *spin : window.findChildren<QSpinBox *>()) {
                    if (spin->suffix() == " kcal") {
                        QTextStream(stdout) << "Calculated calories: " << spin->value() << "\n";
                        app.exit(spin->value() > 0 ? 0 : 4);
                        return;
                    }
                }
                app.exit(5);
            });
            watch.stop();
            return;
        }
    });
    watch.start(100);
    QTimer::singleShot(45000, [&] { QTextStream(stdout) << "Timed out or error dialog\n"; app.exit(6); });
    QTimer::singleShot(0, search, &QPushButton::click);
    return app.exec();
}
