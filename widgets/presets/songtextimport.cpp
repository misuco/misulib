#include "songtextimport.h"
#include "mwscalepreset.h"
#include <QDebug>
#include <QString>

SongTextImport::SongTextImport(PresetCollection *pc, QObject *parent) : QObject(parent),
    _presetCollection(pc),
    _visible(false)
{}

void SongTextImport::importText(QString text)
{
    _presetCollection->clear();
    QStringList lines = text.split("\n");
    for(int j=0;j<lines.size();j++) {
        QString line = lines.at(j);
        for(int i=0;i<line.size();i++) {
            int root=-1;
            QString text;

            findNextRootNote(i,root,line);

            if(root>=0) {
                int textStart = i;
                bool validChordSign = true;
                QList<bool> scale;
                if(line.size()>i+1) {
                    if(line.at(i+1)=="#") {
                        root+=1;
                        i++;
                    }

                    if(line.at(i+1)=="m") {
                        i++;
                        scale.append(false);
                        scale.append(false);
                        scale.append(true);
                        scale.append(false);
                        scale.append(false);
                        scale.append(false);
                        scale.append(true);
                        scale.append(false);
                        scale.append(false);
                        scale.append(false);
                        scale.append(false);
                    } else if(line.at(i+1)=="7") {
                        i++;
                        scale.append(false);
                        scale.append(false);
                        scale.append(false);
                        scale.append(true);
                        scale.append(false);
                        scale.append(false);
                        scale.append(true);
                        scale.append(false);
                        scale.append(false);
                        scale.append(true);
                        scale.append(false);
                    } else if(line.at(i+1)==" ") {
                        scale.append(false);
                        scale.append(false);
                        scale.append(false);
                        scale.append(true);
                        scale.append(false);
                        scale.append(false);
                        scale.append(true);
                        scale.append(false);
                        scale.append(false);
                        scale.append(false);
                        scale.append(false);
                    } else {
                        validChordSign = false;
                    }
                } else {
                    validChordSign = false;
                }

                if(validChordSign) {
                    if(j<lines.size()-1) {
                        int nextRootNotePosition=i;
                        int nextRootNote=-1;
                        findNextRootNote(nextRootNotePosition,nextRootNote,line);
                        if(nextRootNote>=0) {
                            text = lines.at(j+1);
                            text = text.mid(textStart,nextRootNotePosition-textStart);
                        } else {
                            text = lines.at(j+1);
                            text = text.mid(textStart);
                        }
                    }

                    MWScalePreset * p = new MWScalePreset(root,scale,text,this);
                    _presetCollection->append(p);
                }
            }
        }
    }
    qDebug() << text;
    _visible = false;
    emit visibleChanged();
    emit connectScalePresets();
}

void SongTextImport::show()
{
    _visible = true;
    emit visibleChanged();
}

void SongTextImport::findNextRootNote(int &i, int &root, QString &text)
{
    root = -1;
    for(;i<text.size() && root == -1;i++) {
        QChar c = text.at(i);
        if(c=="C") root=0;
        else if(c=="D") root=2;
        else if(c=="E") root=4;
        else if(c=="F") root=5;
        else if(c=="G") root=7;
        else if(c=="A") root=9;
        else if(c=="H") root=11;
        else if(c=="B") root=11;
    }
    i-=1;
}
