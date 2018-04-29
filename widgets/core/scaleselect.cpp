#include "scaleselect.h"

ScaleSelect::ScaleSelect(QString name, QList<bool> scale, QObject *parent) : QObject(parent),
    _scale(scale),
    _text(name)
{}

void ScaleSelect::onReleased()
{
    _selected  = true;
    emit selectedChanged();
    emit setScale(_scale);
}

void ScaleSelect::onSetScale(QList<bool>)
{
    _selected = false;
    emit selectedChanged();
}
