//
// Created by user on 4/19/25.
//

#include "mapProvider.h"

#include <QFile>

void MapProvider::exchangeUrl(const QString &pathToMap) {
    QFile file;
    file.setFileName(QStringLiteral(":/H3VIEWER/data/style.json"));

    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();
    file.close();

    data.replace("%URL%", pathToMap.toLatin1());

    tempStyleFile_ = new QTemporaryFile(this);
    tempStyleFile_->open();
    tempStyleFile_->write(data);
    tempStyleFile_->close();

    setUrl("file:///" + tempStyleFile_->fileName());
}