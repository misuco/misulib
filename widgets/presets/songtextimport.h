#ifndef SONGTEXTIMPORT_H
#define SONGTEXTIMPORT_H

#include <QObject>
#include "presetcollection.h"

class SongTextImport : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible MEMBER _visible  NOTIFY visibleChanged)

public:
    explicit SongTextImport(PresetCollection * pc, QObject *parent = nullptr);

    Q_INVOKABLE void importText(QString text);

signals:
    void visibleChanged();
    void connectScalePresets();

public slots:
    void show();

private:
    PresetCollection * _presetCollection;
    bool    _visible;
    void findNextRootNote(int &i, int &root, QString &text);

};

#endif // SONGTEXTIMPORT_H
