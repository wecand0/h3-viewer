//
// Created by user on 7/13/25.
//

#ifndef H3MODEL_H
#define H3MODEL_H

#include <QAbstractListModel>

#include <QGeoCoordinate>
#include <QGeoRectangle>
#include <h3api.h>


class H3Hexagon {
public:
    H3Index index;
    QGeoCoordinate center;
    QList<QGeoCoordinate> boundary;
    QVariantMap properties; // Для хранения дополнительных данных

    H3Hexagon() : index(0) {}
    H3Hexagon(H3Index idx);
};

class H3HexagonModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(QGeoRectangle viewport READ viewport WRITE setViewport NOTIFY viewportChanged)
    Q_PROPERTY(int h3Resolution READ h3Resolution NOTIFY h3ResolutionChanged)
    Q_PROPERTY(int hexagonCount READ hexagonCount NOTIFY hexagonCountChanged)

public:
    enum HexagonRoles {
        IndexRole = Qt::UserRole + 1,
        CenterRole,
        BoundaryRole,
        PropertiesRole
    };

    explicit H3HexagonModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Properties
    double zoom() const { return m_zoom; }
    void setZoom(double zoom);

    QGeoRectangle viewport() const { return m_viewport; }
    void setViewport(const QGeoRectangle &viewport);

    // Альтернативный метод установки viewport через центр и размеры
    Q_INVOKABLE void setViewportFromCenter(const QGeoCoordinate &center, double widthInDegrees, double heightInDegrees);

    int h3Resolution() const { return m_h3Resolution; }
    int hexagonCount() const { return m_hexagons.size(); }

    // Методы для работы с данными
    Q_INVOKABLE void setHexagonProperty(const QString &h3Index, const QString &key, const QVariant &value);
    Q_INVOKABLE QVariant getHexagonProperty(const QString &h3Index, const QString &key) const;

signals:
    void zoomChanged();
    void viewportChanged();
    void h3ResolutionChanged();
    void hexagonCountChanged();
    void updateStarted();
    void updateFinished();

private:
    void updateHexagons();
    int zoomToH3Resolution(double zoom) const;
    std::vector<H3Index> getHexagonsInViewport(const QGeoRectangle &viewport, int resolution) const;

    double m_zoom;
    QGeoRectangle m_viewport;
    int m_h3Resolution;
    std::vector<H3Hexagon> m_hexagons;
    std::unordered_map<H3Index, size_t> m_indexMap; // Для быстрого поиска

    // Маппинг zoom -> H3 resolution
    static const std::map<int, int> ZOOM_TO_H3_RES;
};

#endif //H3MODEL_H
