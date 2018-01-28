/*
 * Copyright (c) 2018 by misuco.org, Claudio Zopfi, Zurich, Switzerland, c@misuco.org
 *
 * This file is part of MISUCO2.
 *
 * MISUCO2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MISUCO2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MISUCO2.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MISUWIDGET_H
#define MISUWIDGET_H

#include <QWidget>
#include "conf/types.h"
#include "conf/pitch.h"

class MisuWidget : public QObject
{    
    Q_OBJECT

public:
    struct misuTouchEvent {
        int id;
        int x;
        int y;
        int state;
        long t;
    };

    explicit MisuWidget(QObject *parent = 0);
    ~MisuWidget();

    static QString font1;
    static int font1size;
    static QColor fgcolor;
    static QColor bgcolor;
    static QColor highlightcolor;

    static bool bwmode;
    static QColor bkeycolor;
    static QColor wkeycolor;
    static QColor hlbkeycolor;
    static QColor hlwkeycolor;

    static float lOff;
    static float lOn;
    static float sOff;
    static float sOn;

    static Pitch * MWPitch[];
    static MWScale Scale;
    static MWSound Sound;
    static MWMicrotune Microtune;
    static int noteSymbols;
    static bool showFreqs;

    static int channel;

    static bool sendCC1;
    static bool holdMode;

    static QObject * overwritePreset;


};

#endif // MISUWIDGET_H
