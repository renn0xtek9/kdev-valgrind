/* This file is part of KDevelop
   Copyright 2017 Anton Anikin <anton.anikin@htower.ru>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING. If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#pragma once

#include <QString>

#include <functional>

class QAbstractTableModel;
class QProcess;
class QPushButton;
class QWidget;

namespace Valgrind
{

QString eventFullName(const QString& eventShortName);

enum ItemDataRole
{
    SortRole = Qt::UserRole + 1
};

static const int rightAlign = int(Qt::AlignRight | Qt::AlignVCenter);

QString displayValue(int value);
QString displayValue(double value);

void emitDataChanged(QAbstractTableModel* model);

void setupVisualizerProcess(QProcess* visualizerProcess,
                            QPushButton* startButton,
                            std::function<void()> startFunction,
                            bool startImmediately);

QWidget* activeMainWindow();

QString findExecutable(const QString &executableName);

}
