#include <QDebug>
#include "presetcollection.h"
#include "lib/misulib/models/purchases.h"
#include "mwsoundpreset.h"
#include "mwscalepreset.h"

QObject * PresetCollection::dialogContext = nullptr;
bool PresetCollection::dialogVisible = false;

PresetCollection::PresetCollection(QObject *parent) : QObject(parent)
{

}

void PresetCollection::closeDialogPreset()
{
    dialogVisible=false;
    emit layoutChange();
}

void PresetCollection::overwritePreset()
{
    auto soundPreset = qobject_cast<MWSoundPreset *>(PresetCollection::dialogContext);
    if(soundPreset) {
        soundPreset->overwrite();
        PresetCollection::dialogContext = nullptr;
        //writeXml("synth.xml");
    }

    auto scalePreset = qobject_cast<MWScalePreset *>(PresetCollection::dialogContext);
    if(scalePreset) {
        scalePreset->overwrite();
        PresetCollection::dialogContext = nullptr;
        //writeXml("scales.xml");
    }

    dialogVisible=false;
    emit layoutChange();
}

void PresetCollection::buyPresetManager()
{
    qDebug() << "wlayout::buyPresetManager";
    if(Purchases::productPresetManager) {
        qDebug() << "_productPresetManager->purchase";
        Purchases::productPresetManager->purchase();
    }
}

void PresetCollection::importSong()
{
    dialogVisible=false;
    emit layoutChange();
    emit showImportSong();
}

void PresetCollection::append(QObject *item)
{
    _items.append(item);
    emit itemsChange();
}

void PresetCollection::clear()
{
    for(auto item:_items) item->deleteLater();
    _items.clear();
    emit itemsChange();
}

QList<QObject *> PresetCollection::getItems()
{
    return _items;
}

QObject *PresetCollection::getItem(int i)
{
    return _items.at(i);
}

int PresetCollection::getItemCount()
{
    return _items.size();
}


void PresetCollection::onEditPreset()
{
    dialogVisible = true;
    emit layoutChange();
}
