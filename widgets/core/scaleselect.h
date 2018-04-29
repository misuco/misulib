#ifndef SCALESELECT_H
#define SCALESELECT_H

#include <QObject>

class ScaleSelect : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString text MEMBER _text CONSTANT)
    Q_PROPERTY(bool selected MEMBER _selected NOTIFY selectedChanged)

public:
    explicit ScaleSelect(QString name, QList<bool> scale, QObject *parent = nullptr);

    Q_INVOKABLE void onReleased();

public slots:
    void onSetScale(QList<bool>);

signals:
    void setScale(QList<bool>);
    // QML
    void selectedChanged();

private:
    QList<bool> _scale;
    QString     _text;
    bool        _selected;

};

#endif // SCALESELECT_H
