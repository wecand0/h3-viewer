#include "h3model.h"

#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include <cmath>

const std::map<int, int> H3HexagonModel::ZOOM_TO_H3_RES = {
    {4, 1},  {5, 1},  {6, 2},   {7, 3},   {8, 3},   {9, 4},   {10, 5},  {11, 6},  {12, 6},  {13, 7},
    {14, 8}, {15, 9}, {16, 9}, {17, 10}, {18, 10}, {19, 11}, {20, 11}, {21, 12}, {22, 13}, {23, 14}, {24, 15}};

H3Hexagon::H3Hexagon(const H3Index idx) : index(idx)
{
    // Получаем центр гексагона
    LatLng centerLatLng;
    cellToLatLng(index, &centerLatLng);
    center = QGeoCoordinate(radsToDegs(centerLatLng.lat), radsToDegs(centerLatLng.lng));

    // Получаем границы гексагона
    CellBoundary cellBoundary;
    cellToBoundary(index, &cellBoundary);

    for (int i = 0; i < cellBoundary.numVerts; i++)
    {
        boundary.append(QGeoCoordinate(radsToDegs(cellBoundary.verts[i].lat), radsToDegs(cellBoundary.verts[i].lng)));
    }
    // Замыкаем полигон
    if (!boundary.isEmpty())
    {
        boundary.append(boundary.first());
    }
}

H3HexagonModel::H3HexagonModel(QObject* parent) : QAbstractListModel(parent), m_zoom(5.0), m_h3Resolution(1) {}

int H3HexagonModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_hexagons.size();
}

QVariant H3HexagonModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_hexagons.size())
        return QVariant();

    const H3Hexagon& hexagon = m_hexagons[index.row()];

    switch (role)
    {
    case IndexRole:
        return QString::number(hexagon.index, 16);
    case CenterRole:
        return QVariant::fromValue(hexagon.center);
    case BoundaryRole:
        {
            QVariantList boundary;
            for (const auto& coord : hexagon.boundary)
            {
                boundary.append(QVariant::fromValue(coord));
            }
            return boundary;
        }
    case PropertiesRole:
        return hexagon.properties;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> H3HexagonModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IndexRole] = "h3Index";
    roles[CenterRole] = "center";
    roles[BoundaryRole] = "boundary";
    roles[PropertiesRole] = "properties";
    return roles;
}

void H3HexagonModel::setZoom(double zoom)
{
    if (qFuzzyCompare(m_zoom, zoom))
        return;

    m_zoom = zoom;
    emit zoomChanged();

    if (const int newResolution = zoomToH3Resolution(zoom); newResolution != m_h3Resolution)
    {
        m_h3Resolution = newResolution;
        emit h3ResolutionChanged();
        updateHexagons();
    }
}

void H3HexagonModel::setViewport(const QGeoRectangle& viewport)
{
    if (m_viewport == viewport)
        return;

    m_viewport = viewport;
    emit viewportChanged();

    updateHexagons();
}

auto H3HexagonModel::setViewportFromCenter(const QGeoCoordinate& center, const double widthInDegrees,
                                           const double heightInDegrees) -> void
{
    if (!center.isValid())
        return;

    // Создаем прямоугольник на основе центра и размеров
    const double halfWidth = widthInDegrees / 2.0;
    const double halfHeight = heightInDegrees / 2.0;

    const QGeoCoordinate topLeft(center.latitude() + halfHeight, center.longitude() - halfWidth);
    const QGeoCoordinate bottomRight(center.latitude() - halfHeight, center.longitude() + halfWidth);

    const QGeoRectangle newViewport(topLeft, bottomRight);
    setViewport(newViewport);
}

void H3HexagonModel::setHexagonProperty(const QString& h3IndexStr, const QString& key, const QVariant& value)
{
    const H3Index h3Index = std::stoull(h3IndexStr.toStdString(), nullptr, 16);

    if (const auto it = m_indexMap.find(h3Index); it != m_indexMap.end())
    {
        m_hexagons[it->second].properties[key] = value;
        const QModelIndex modelIndex = index(it->second);
        emit dataChanged(modelIndex, modelIndex, {PropertiesRole});
    }
}

QVariant H3HexagonModel::getHexagonProperty(const QString& h3IndexStr, const QString& key) const
{
    const H3Index h3Index = std::stoull(h3IndexStr.toStdString(), nullptr, 16);
    if (const auto it = m_indexMap.find(h3Index); it != m_indexMap.end())
    {
        return m_hexagons[it->second].properties.value(key);
    }
    return {};
}

void H3HexagonModel::updateHexagons()
{
    emit updateStarted();

    beginResetModel();

    m_hexagons.clear();
    m_indexMap.clear();

    qDebug() << "Updating hexagons - Viewport valid:" << m_viewport.isValid() << "TopLeft:" << m_viewport.topLeft()
             << "BottomRight:" << m_viewport.bottomRight() << "Resolution:" << m_h3Resolution;

    if (m_viewport.isValid() && !m_viewport.isEmpty())
    {
        std::vector<H3Index> hexIndexes = getHexagonsInViewport(m_viewport, m_h3Resolution);

        qDebug() << "Got" << hexIndexes.size() << "hex indexes from H3";

        m_hexagons.reserve(hexIndexes.size());
        for (size_t i = 0; i < hexIndexes.size(); ++i)
        {
            m_hexagons.emplace_back(hexIndexes[i]);
            m_indexMap[hexIndexes[i]] = i;
        }
    }

    endResetModel();

    emit hexagonCountChanged();
    emit updateFinished();

    qDebug() << "Updated hexagons:" << m_hexagons.size() << "at resolution" << m_h3Resolution;
}

int H3HexagonModel::zoomToH3Resolution(const double zoom) const
{
    int intZoom = static_cast<int>(std::round(zoom));

    // Ограничиваем zoom диапазоном
    intZoom = std::max(4, std::min(24, intZoom));

    if (const auto it = ZOOM_TO_H3_RES.find(intZoom); it != ZOOM_TO_H3_RES.end())
    {
        return it->second;
    }

    // Значение по умолчанию
    return 4;
}

std::vector<H3Index> H3HexagonModel::getHexagonsInViewport(const QGeoRectangle& viewport, const int resolution) const
{
    std::vector<H3Index> result;

    if (!viewport.isValid() || viewport.isEmpty())
        return result;

    // Получаем углы viewport
    const QGeoCoordinate topLeft = viewport.topLeft();
    const QGeoCoordinate bottomRight = viewport.bottomRight();

    qDebug() << "Getting hexagons for viewport:"
             << "TL:" << topLeft.latitude() << "," << topLeft.longitude() << "BR:" << bottomRight.latitude() << ","
             << bottomRight.longitude() << "Resolution:" << resolution;

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
    auto error = maxPolygonToCellsSize(&polygon, resolution, 0, &numHexagons);

    qDebug() << "Estimated hexagons count:" << numHexagons;

    if (numHexagons > 0 && numHexagons < 10000)
    { // Ограничение для производительности
        result.resize(numHexagons);
        polygonToCells(&polygon, resolution, 0, result.data());

        // Удаляем нулевые индексы
        std::erase(result, 0);

        qDebug() << "Actual hexagons after filtering:" << result.size();
    }
    else if (numHexagons >= 10000)
    {
        qWarning() << "Too many hexagons requested:" << numHexagons << "- limiting viewport";
    }

    return result;
}
