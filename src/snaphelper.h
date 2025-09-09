#ifndef SNAPHELPER_H
#define SNAPHELPER_H

#include <QObject>
#include <QQuickView>

#include <QCoro/QCoroCore>
#include <QCoro/QCoroQmlTask>

class ResponseProxy : public QObject
{
    Q_OBJECT

public:
    ResponseProxy(QObject *parent = nullptr) : QObject(parent) {}
    ~ResponseProxy() = default;

signals:
    void signal(uint, QVariantMap);
public slots:
    void slot(uint i, QVariantMap v) {
        emit signal(i, v);
    }
};

class SnapHelper : public QObject
{
    Q_OBJECT

public:
    SnapHelper();
    ~SnapHelper() = default;

    Q_PROPERTY(bool isSnap READ isSnap CONSTANT)

    QCoro::Task<bool> co_saveFileDialog(QUrl file, bool show);
    Q_INVOKABLE QCoro::QmlTask saveFileDialog(QUrl file, bool show) {
        return co_saveFileDialog(file, show);
    }

    bool isSnap() const;
};

#endif // SNAPHELPER_H
