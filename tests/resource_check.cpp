#include <QApplication>
#include <QPixmap>
#include <QTextStream>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    const QStringList resources = {
        ":/images/appearance/stand_blink/mid_autumn_half.png",
        ":/images/appearance/stand_blink/mid_autumn_closed.png",
        ":/images/appearance/stand_blink/bunny_pajama_half.png",
        ":/images/appearance/stand_blink/bunny_pajama_closed.png",
        ":/images/appearance/stand_blink/cat_pajama_half.png",
        ":/images/appearance/stand_blink/cat_pajama_closed.png",
        ":/images/appearance/reading_expressions/mid_autumn_open.png",
        ":/images/appearance/reading_expressions/mid_autumn_half.png",
        ":/images/appearance/reading_expressions/mid_autumn_closed.png",
        ":/images/appearance/reading_expressions/bunny_pajama_open.png",
        ":/images/appearance/reading_expressions/bunny_pajama_half.png",
        ":/images/appearance/reading_expressions/bunny_pajama_closed.png",
        ":/images/appearance/reading_expressions/cat_pajama_open.png",
        ":/images/appearance/reading_expressions/cat_pajama_half.png",
        ":/images/appearance/reading_expressions/cat_pajama_closed.png"
    };
    for (const QString &path : resources) {
        QPixmap frame(path);
        if (frame.isNull()) {
            QTextStream(stderr) << "Missing resource: " << path << "\n";
            return 1;
        }
    }
    QTextStream(stdout) << "Verified " << resources.size() << " animation resources.\n";
    return 0;
}
