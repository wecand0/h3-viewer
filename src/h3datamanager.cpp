//
// Created by user on 7/13/25.
//

#include "h3datamanager.h"

#include <QDebug>
#include <algorithm>
#include <utility>

H3ComputeTask::H3ComputeTask(const QGeoRectangle& viewport, const int resolution,
                             std::function<void(std::vector<H3Index>)> callback) :
    m_viewport(viewport), m_resolution(resolution), m_callback(std::move(callback))
{
}

void H3ComputeTask::run()
{
    std::vector<H3Index> result;

    if (m_viewport.isValid())
    {
        // Получаем углы viewport
        const QGeoCoordinate topLeft = m_viewport.topLeft();
        const QGeoCoordinate bottomRight = m_viewport.bottomRight();

        // Преобразуем в H3 GeoPolygon
        std::vector<LatLng> verts;
        verts.push_back({degsToRads(topLeft.latitude()), degsToRads(topLeft.longitude())});
        verts.push_back({degsToRads(topLeft.latitude()), degsToRads(bottomRight.longitude())});
        verts.push_back({degsToRads(bottomRight.latitude()), degsToRads(bottomRight.longitude())});
        verts.push_back({degsToRads(bottomRight.latitude()), degsToRads(topLeft.longitude())});

        GeoPolygon polygon;
        polygon.geoloop.verts = verts.data();
        polygon.geoloop.numVerts = verts.size();
        polygon.numHoles = 0;
        polygon.holes = nullptr;

        // Оцениваем количество гексагонов
        int64_t numHexagons = 0;
        auto error = maxPolygonToCellsSize(&polygon, m_resolution, 0, &numHexagons);

        if (numHexagons > 0 && numHexagons < 100000)
        {
            result.resize(numHexagons);
            polygonToCells(&polygon, m_resolution, 0, result.data());

            // Удаляем нулевые индексы
            std::erase(result, 0);
        }
    }

    if (m_callback)
    {
        m_callback(std::move(result));
    }
}

H3DataManager::H3DataManager(QObject* parent) :
    QObject(parent), m_cacheEnabled(true), m_threadPool(new QThreadPool(this))
{
    m_cache.setMaxCost(5000); // По умолчанию кешируем до 1000 элементов
    m_threadPool->setMaxThreadCount(QThread::idealThreadCount());
}

H3DataManager::~H3DataManager() { m_threadPool->waitForDone(); }

void H3DataManager::setCacheEnabled(const bool enabled)
{
    if (m_cacheEnabled == enabled)
        return;

    m_cacheEnabled = enabled;
    if (!enabled)
    {
        m_cache.clear();
    }
    emit cacheEnabledChanged();
}

void H3DataManager::setCacheSize(int size)
{
    if (m_cache.maxCost() == size)
        return;

    m_cache.setMaxCost(size);
    emit cacheSizeChanged();
}

void H3DataManager::setHexagonData(const H3Index index, const H3Data& data)
{
    QMutexLocker locker(&m_mutex);
    m_data[index] = data;
    emit dataUpdated(index);
}

H3Data H3DataManager::getHexagonData(const H3Index index) const
{
    QMutexLocker locker(&m_mutex);
    if (const auto it = m_data.find(index); it != m_data.end())
    {
        return it->second;
    }
    return H3Data();
}

void H3DataManager::clearData()
{
    QMutexLocker locker(&m_mutex);
    m_data.clear();
    m_aggregatedValues.clear();
    m_cache.clear();
}

void H3DataManager::aggregateToParent(const H3Index childIndex, const double value)
{
    QMutexLocker locker(&m_mutex);

    // Получаем родительский индекс
    H3Index parentIndex = 0;
    auto error = cellToParent(childIndex, getResolution(childIndex) - 1, &parentIndex);

    // Агрегируем значение
    m_aggregatedValues[parentIndex] += value;

    // Рекурсивно агрегируем вверх по иерархии
    if (getResolution(parentIndex) > 0)
    {
        aggregateToParent(parentIndex, value);
    }
}

double H3DataManager::getAggregatedValue(const H3Index index) const
{
    QMutexLocker locker(&m_mutex);
    if (const auto it = m_aggregatedValues.find(index); it != m_aggregatedValues.end())
    {
        return it->second;
    }
    return 0.0;
}

QList<H3Index> H3DataManager::getNeighbors(const H3Index index, const int k)
{
    QList<H3Index> result;

    if (k == 1)
    {
        // Для k=1 используем оптимизированную функцию
        H3Index neighbors[7] = {0}; // 6 соседей + центральная ячейка
        gridDisk(index, 1, neighbors);

        for (int i = 0; i < 7; ++i)
        {
            if (neighbors[i] != 0 && neighbors[i] != index)
            {
                result.append(neighbors[i]);
            }
        }
    }
    else
    {
        // Для k>1 используем gridDiskDistances
        int64_t maxSize = 0;
        auto error = maxGridDiskSize(k, &maxSize);
        std::vector<H3Index> neighbors(maxSize, 0);
        std::vector<int> distances(maxSize, 0);

        gridDiskDistances(index, k, neighbors.data(), distances.data());

        for (size_t i = 0; i < neighbors.size(); ++i)
        {
            if (neighbors[i] != 0 && neighbors[i] != index && distances[i] <= k)
            {
                result.append(neighbors[i]);
            }
        }
    }

    return result;
}

// void H3DataManager::getHexagonsInViewportAsync(const QGeoRectangle& viewport, int resolution,
//                                                std::function<void(std::vector<H3Index>)> callback)
// {
//     emit computationStarted();
//
//     // Проверяем кеш
//     if (m_cacheEnabled)
//     {
//         qDebug() << "cache";
//         // Простой ключ кеша на основе viewport и resolution
//         const H3Index cacheKey = geoToH3(viewport.center(), resolution);
//
//         if (const auto cached = m_cache.object(cacheKey))
//         {
//             callback(*cached);
//             emit computationFinished();
//             return;
//         }
//     }
//
//     // Создаем задачу для выполнения в отдельном потоке
//     auto task =
//         new H3ComputeTask(viewport, resolution,
//                           [this, callback, viewport, resolution](std::vector<H3Index> result)
//                           {
//                               // Кешируем результат
//                               if (m_cacheEnabled && !result.empty())
//                               {
//                                   H3Index cacheKey = geoToH3(viewport.center(), resolution);
//                                   m_cache.insert(cacheKey, new std::vector<H3Index>(result));
//                               }
//
//                               // Вызываем callback в основном потоке
//                               QMetaObject::invokeMethod(
//                                   this, [callback, result]() { callback(std::move(result)); }, Qt::QueuedConnection);
//
//                               emit computationFinished();
//                           });
//
//     m_threadPool->start(task);
// }

QString H3DataManager::h3IndexToString(const H3Index index) { return QString::number(index, 16); }

H3Index H3DataManager::stringToH3Index(const QString& str) { return str.toULongLong(nullptr, 16); }

QGeoCoordinate H3DataManager::h3ToGeo(const H3Index index)
{
    LatLng geo;
    cellToLatLng(index, &geo);
    return QGeoCoordinate(radsToDegs(geo.lat), radsToDegs(geo.lng));
}

H3Index H3DataManager::geoToH3(const QGeoCoordinate& coord, const int resolution)
{
    LatLng geo;
    geo.lat = degsToRads(coord.latitude());
    geo.lng = degsToRads(coord.longitude());
    H3Index out = 0;
    auto error = latLngToCell(&geo, resolution, &out);
    return out;
}

QColor H3DataManager::valueToColor(const double value, const double minValue, const double maxValue)
{
    // Нормализуем значение
    double normalized = (value - minValue) / (maxValue - minValue);
    normalized = std::max(0.0, std::min(1.0, normalized));

    // Градиент от синего к красному через зеленый
    QColor color;
    if (normalized < 0.5)
    {
        // Синий -> Зеленый
        const double t = normalized * 2.0;
        color.setRgbF(0, t, 1.0 - t);
    }
    else
    {
        // Зеленый -> Красный
        const double t = (normalized - 0.5) * 2.0;
        color.setRgbF(t, 1.0 - t, 0);
    }

    return color;
}
