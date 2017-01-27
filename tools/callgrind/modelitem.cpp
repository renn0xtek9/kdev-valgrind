/* This file is part of KDevelop
 * Copyright 2011 Mathieu Lornac <mathieu.lornac@gmail.com>
 * Copyright 2011 Damien Coppel <damien.coppel@gmail.com>
 * Copyright 2011 Sarie Lucas <lucas.sarie@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "modelitem.h"
#include "debug.h"


namespace valgrind
{

CallgrindCallstackItem::CallgrindCallstackItem(CallgrindCallstackFunction *function)
{
    Q_ASSERT(function);
    m_csFunction = function;
    m_numCalls = 0;
}

void  CallgrindCallstackItem::addChild(CallgrindCsItem *child)
{
    m_childs.append(child);
}

CallgrindCallstackItem  *CallgrindCallstackItem::child(int n) const
{
    return m_childs.at(n);
}

int CallgrindCallstackItem::childCount() const
{
    return m_childs.size();
}

void  CallgrindCallstackItem::addParent(CallgrindCsItem *child)
{
    m_parents.append(child);
}

CallgrindCallstackItem  *CallgrindCallstackItem::parent(int n) const
{
    return m_parents.at(n);
}

int CallgrindCallstackItem::parentCount() const
{
    return m_parents.size();
}

CallgrindCallstackFunction  *CallgrindCallstackItem::csFunction() const
{
    return m_csFunction;
}

CallgrindCallstackItem  *CallgrindCallstackItem::totalCountItem() const
{
    return (m_csFunction->totalCountItem());
}

bool CallgrindCallstackItem::hasKey(int n)
{
    if (n == 0 || n == 1 || n == 2 || m_numericValue.contains(n))
        return true;
    return false;
}

void  CallgrindCallstackItem::setNumericValue(int n, int val)
{
    m_numericValue[n] = val;
}

int   CallgrindCallstackItem::numericValue(int n) const
{
    return m_numericValue[n];
}

void  CallgrindCallstackItem::setValue(const QString& key, const QString& value)
{
    int iKey = iCachegrindItem::dataKeyFromName(key);
    if (iKey == iCachegrindItem::Unknow)
        return;
    if (iCachegrindItem::isNumericValue(iKey))
    {
        m_numericValue[iKey] = value.toInt();
    }
    //else if (iKey == iCachegrindItem::FileName)
    //    m_fileName = value;
    //else
    //    m_fctName = value;
}

QVariant  CallgrindCallstackItem::data(iCachegrindItem::Columns key, numberDisplayMode disp) const
{
    switch (key)
    {
    case iCachegrindItem::FileName:
        return this->m_csFunction->filename();
    case iCachegrindItem::CallName:
        return this->m_csFunction->functionName();
    case iCachegrindItem::NumberOfCalls:
        return m_numCalls;
    default:
        return (this->numericValue(key, disp));
    }
    return QVariant();
}

QVariant  CallgrindCallstackItem::numericValue(iCachegrindItem::Columns key, numberDisplayMode disp) const
{
    //TODO: REPLACE WITH PTR ON FCT
    switch (disp)
    {
    case E_NORMAL:
        return this->numericValue(key);
    case E_INCLUDE_NORMAL:
        return this->includeNumericValue(key);
    case E_PERCENT:
        return this->numericValuePercent(key);
    case E_INCLUDE_PERCENT:
        return this->includeNumericValuePercent(key);
    }
    return 0;
}

unsigned long long CallgrindCallstackItem::numericValue(iCachegrindItem::Columns col) const
{
    return (m_numericValue[col]);
}

unsigned long long CallgrindCallstackItem::includeNumericValue(iCachegrindItem::Columns col) const
{
    unsigned long long includeValue = 0;
    for (int i = 0; i < m_parents.size(); ++i)
    {
        includeValue += m_parents[i]->numericValue(col);
    }
    return (includeValue);
}

double CallgrindCallstackItem::numericValuePercent(iCachegrindItem::Columns col) const
{
    return (double) ((int) (((float) numericValue(col)) / ((float) totalCountItem()->numericValue(col)) * 10000)) / 100;
}

double CallgrindCallstackItem::includeNumericValuePercent(iCachegrindItem::Columns col) const
{
    unsigned long long includeValue = 0;
    for (int i = 0; i < m_parents.size(); ++i)
    {
        includeValue += m_parents[i]->numericValue(col);
    }
    return (double) ((int) (((float) includeValue) / ((float) totalCountItem()->numericValue(col)) * 10000)) / 100;
}

///////////////////////////////////////////////////////////////////////////////
//                         CallgrindCallstackFunction                        //
///////////////////////////////////////////////////////////////////////////////

void  CallgrindCallstackFunction::setTotalCountItem(CallgrindCallstackItem *totalCountItem)
{
    m_totalCountItem = totalCountItem;
}

CallgrindCallstackItem  *CallgrindCallstackFunction::totalCountItem() const
{
    return (m_totalCountItem);
}

void  CallgrindCallstackFunction::setFilename(const QString& fn)
{
    m_fileName = fn;
}

const QString& CallgrindCallstackFunction::filename() const
{
    return m_fileName;
}

void  CallgrindCallstackFunction::setFunctionName(const QString& fn)
{
    m_fctName = fn;
}

const QString& CallgrindCallstackFunction::functionName() const
{
    return m_fctName;
}

void CallgrindCallstackFunction::setFullDescName(const QString& fdn)
{
    int iEnd, iBegin;

    iBegin = 0;
    m_fullDescName = fdn;
    //file name
    if ((iEnd = fdn.indexOf(QChar(':'), iBegin)) == -1)
        return;
    this->m_fileName = fdn.mid(iBegin, iEnd - iBegin);
    //function name name
    iBegin = iEnd + 1;
    iEnd = fdn.length();
    this->m_fctName = fdn.mid(iBegin, iEnd - iBegin);

}

const QString& CallgrindCallstackFunction::fullDescName() const
{
    return m_fullDescName;
}
}