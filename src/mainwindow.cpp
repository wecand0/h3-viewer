#include "mainwindow.h"

#include "QtConcurrent"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QQmlComponent>
#include <QQmlContext>
#include <QRandomGenerator>
#include <QTimer>



MainWindow::MainWindow(QObject *parent)
    : QObject(parent), rootWindow_(nullptr) {

    std::srand(QDateTime::currentMSecsSinceEpoch() / 1000);

    qmlRegisterType<H3HexagonModel>("H3VIEWER", 1, 0, "H3HexagonModel");

    h3HexagonModel_ = new H3HexagonModel();
    engine_.rootContext()->setContextProperty("H3HexagonModel", h3HexagonModel_);

    initMapProvider();
    initEngine();

    connect(&engine_, &QQmlApplicationEngine::quit, &QGuiApplication::quit);
}

MainWindow::~MainWindow() {
    mapProvider_->deleteLater();
    h3HexagonModel_->deleteLater();
    rootWindow_->deleteLater();
}

void MainWindow::initEngine() {
    const QUrl url("qrc:/H3VIEWER/ui/main.qml");
    connect(
            &engine_, &QQmlApplicationEngine::objectCreated,
            this, [this, url](QObject *obj, const QUrl &objUrl) {
                if (!obj && url == objUrl)
                    QCoreApplication::exit(-1);
                if (url == objUrl) {
                    rootWindow_ = qobject_cast<QQuickWindow *>(obj);
                    if (!rootWindow_)
                        qWarning() << "Корневой объект не является QQuickWindow!";
                }
            },
            Qt::QueuedConnection);

    engine_.load(url);
}
void MainWindow::initMapProvider() {
    mapProvider_ = new MapProvider();
    engine_.rootContext()->setContextProperty("mapProvider", mapProvider_);
    const QString pathToMap = "mbtiles://" + QDir::homePath() + QDir::separator() + QApplication::applicationName() + QDir::separator() + "map.mbtiles";
    mapProvider_->exchangeUrl(pathToMap);
}