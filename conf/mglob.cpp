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

#include "mglob.h"
#include <QEvent>
#include <QTouchEvent>
#include <QDateTime>
#include <QDebug>

QString MGlob::font1("Sans");
int MGlob::font1size(10);


MWSound MGlob::Sound;
MWMicrotune MGlob::Microtune;


int MGlob::channel = 0;

int MGlob::playAreaWidth = 0;
int MGlob::playAreaHeight = 0;
int MGlob::playFieldWidth = 0;
int MGlob::playFieldHeight = 0;

MGlob::MGlob(QObject *parent) : QObject(parent)
{
}

MGlob::~MGlob()
{
}