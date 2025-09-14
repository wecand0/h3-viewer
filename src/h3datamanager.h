//
// Created by user on 7/13/25.
//

#ifndef H3DATAMANAGER_H
#define H3DATAMANAGER_H

#include <QObject>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QCache>
#include <QVariantMap>
#include <QColor>

#include <h3api.h>
#include <unordered_map>
#include <memory>
#include <QGeoRectangle>

// Структура для хранения данных гексагона
struct H3Data {
    H3Index index;
    QVariantMap properties;
    double value; // Для визуализации тепловых карт
    QColor color; // Кастомный цвет

    H3Data() : index(0), value(0.0) {}
};

// Задача для параллельной обработки
class H3ComputeTask : public QRunnable {
public:
    H3ComputeTask(const QGeoRectangle &viewport, int resolution,
                  std::function<void(std::vector<H3Index>)> callback);
    void run() override;

private:
    QGeoRectangle m_viewport;
    int m_resolution;
    std::function<void(std::vector<H3Index>)> m_callback;
};

// Менеджер данных H3
class H3DataManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool cacheEnabled READ cacheEnabled WRITE setCacheEnabled NOTIFY cacheEnabledChanged)
    Q_PROPERTY(int cacheSize READ cacheSize WRITE setCacheSize NOTIFY cacheSizeChanged)

public:
    explicit H3DataManager(QObject *parent = nullptr);
    ~H3DataManager();

    // Управление кешем
    bool cacheEnabled() const { return m_cacheEnabled; }
    void setCacheEnabled(bool enabled);

    int cacheSize() const { return m_cache.maxCost(); }
    void setCacheSize(int size);

    // Работа с данными
    Q_INVOKABLE void setHexagonData(H3Index index, const H3Data &data);
    Q_INVOKABLE H3Data getHexagonData(H3Index index) const;
    Q_INVOKABLE void clearData();

    // Агрегация данных
    Q_INVOKABLE void aggregateToParent(H3Index childIndex, double value);
    Q_INVOKABLE double getAggregatedValue(H3Index index) const;

    // Вычисление соседей
    Q_INVOKABLE static QList<H3Index> getNeighbors(H3Index index, int k = 1);

    // Оптимизированное получение гексагонов
    // void getHexagonsInViewportAsync(const QGeoRectangle &viewport, int resolution,
    //                                std::function<void(std::vector<H3Index>)> callback);

    // Утилиты
    Q_INVOKABLE static QString h3IndexToString(H3Index index);
    Q_INVOKABLE static H3Index stringToH3Index(const QString &str);
    Q_INVOKABLE static QGeoCoordinate h3ToGeo(H3Index index);
    Q_INVOKABLE static H3Index geoToH3(const QGeoCoordinate &coord, int resolution);

    // Цветовая карта для визуализации
    Q_INVOKABLE static QColor valueToColor(double value, double minValue, double maxValue);

signals:
    void cacheEnabledChanged();
    void cacheSizeChanged();
    void dataUpdated(H3Index index);
    void computationStarted();
    void computationFinished();

private:
    mutable QMutex m_mutex;
    std::unordered_map<H3Index, H3Data> m_data;
    std::unordered_map<H3Index, double> m_aggregatedValues;
    QCache<H3Index, std::vector<H3Index>> m_cache;
    bool m_cacheEnabled{true};
    QThreadPool *m_threadPool;
};


#endif //H3DATAMANAGER_H
