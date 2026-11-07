#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>
#include <QTextStream>
#include <QTimer>
#include <QUrl>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    out << "SSL available: " << QSslSocket::supportsSsl() << "\n"
        << "SSL runtime: " << QSslSocket::sslLibraryVersionString() << "\n";
    out.flush();
    if (!QSslSocket::supportsSsl()) return 1;
    QNetworkAccessManager network;
    QNetworkRequest request(QUrl("https://api.nal.usda.gov/fdc/v1/foods/search?api_key=DEMO_KEY&query=apple&pageSize=1"));
    QNetworkReply *reply = network.get(request);
    QTimer::singleShot(30000, reply, &QNetworkReply::abort);
    QObject::connect(reply, &QNetworkReply::finished, &app, [&] {
        out << "HTTP status: " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << "\n";
        out << "Network result: " << reply->errorString() << "\n";
        out.flush();
        app.exit(reply->error() == QNetworkReply::NoError ? 0 : 2);
    });
    return app.exec();
}
