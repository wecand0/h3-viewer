#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QQmlApplicationEngine>
#include <QQuickWindow>


#include "mapProvider.h"

#include "h3datamanager.h"
#include "h3model.h"

class MainWindow final : public QObject {
    Q_OBJECT
public:
    explicit MainWindow(QObject *parent = nullptr);
    ~MainWindow() override;

private:
    void initEngine();
    void initMapProvider();

    QQmlApplicationEngine engine_;
    QQuickWindow *rootWindow_;
    MapProvider *mapProvider_{};

    H3HexagonModel *h3HexagonModel_{};
};


#endif//MAINWINDOW_H
