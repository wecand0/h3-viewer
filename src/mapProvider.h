//
// Created by user on 4/19/25.
//

#ifndef MAPPROVIDER_H
#define MAPPROVIDER_H

#include <QObject>
#include <QTemporaryFile>

class MapProvider : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)

public:
    explicit MapProvider(QObject *parent = nullptr) : QObject(parent) {
        tempStyleFile_ = new QTemporaryFile(this);
    }
    ~MapProvider() override {
        tempStyleFile_->deleteLater();
    }
    [[nodiscard]] QString url() const noexcept { return url_; }

    void exchangeUrl(const QString &pathToMap);

public slots:
    void setUrl(const QString &url) noexcept {
        if (url_ != url) {
            url_ = url;
            emit urlChanged();
        }
    }

signals:
    void urlChanged();

private:
    QString url_;
    QTemporaryFile *tempStyleFile_;
};

#endif//MAPPROVIDER_H
