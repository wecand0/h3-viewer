#pragma once

#include <QApplication>

#include <QKeyEvent>
#include <qsystemsemaphore.h>
#include <QSharedMemory>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include <QLockFile>

#include "mainwindow.h"

#include <csignal>

#include <arpa/inet.h>
#include <ifaddrs.h>

class Application final : public QApplication
{
public:
    Application(int &argc, char **argv);
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};