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

#include "events.h"

#include <klocalizedstring.h>

#include <QMap>

namespace valgrind
{

QString eventFullName(const QString& eventShortName)
{
    static QMap<QString, QString> fullNames;
    static bool initDone = false;
    if (!initDone) {
        initDone = true;
        // full names are from KCachegrind
        fullNames["Ir"  ] = i18n("Instruction Fetch");
        fullNames["Dr"  ] = i18n("Data Read Access");
        fullNames["Dw"  ] = i18n("Data Write Access");
        fullNames["I1mr"] = i18n("L1 Instr. Fetch Miss");
        fullNames["D1mr"] = i18n("L1 Data Read Miss");
        fullNames["D1mw"] = i18n("L1 Data Write Miss");
        fullNames["ILmr"] = i18n("LL Instr. Fetch Miss");
        fullNames["DLmr"] = i18n("LL Data Read Miss");
        fullNames["DLmw"] = i18n("LL Data Write Miss");

        // full names are from Cachegrind manual
        fullNames["Bc"  ] = i18n("Conditional branches executed)");
        fullNames["Bcm" ] = i18n("Conditional branches mispredicted");
        fullNames["Bi"  ] = i18n("Indirect branches executed");
        fullNames["Bim" ] = i18n("Indirect branches mispredicted");
    }

    return fullNames.value(eventShortName, eventShortName);
}

}