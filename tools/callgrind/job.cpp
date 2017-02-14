/* This file is part of KDevelop
   Copyright 2011 Mathieu Lornac <mathieu.lornac@gmail.com>
   Copyright 2011 Damien Coppel <damien.coppel@gmail.com>
   Copyright 2011 Lionel Duc <lionel.data@gmail.com>
   Copyright 2011 Sebastien Rannou <mxs@sbrk.org>
   Copyright 2011 Lucas Sarie <lucas.sarie@gmail.com>
   Copyright 2016-2017 Anton Anikin <anton.anikin@htower.ru>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "job.h"

#include "model.h"
#include "generic/utils.h"
#include "parser.h"
#include "plugin.h"
#include "settings.h"
#include "view.h"

#include <interfaces/ilaunchconfiguration.h>
#include <kconfiggroup.h>
#include <kshell.h>

#include <QBuffer>
#include <QFile>

namespace Valgrind
{

namespace Callgrind
{

Job::Job(KDevelop::ILaunchConfiguration* cfg, Plugin* plugin, QObject* parent)
    : Generic::Job(cfg, QStringLiteral("callgrind"), true, plugin, parent)
    , m_model(new FunctionsModel)
    , m_outputFile(QStringLiteral("%1/kdevvalgrind_callgrind.out").arg(m_workingDir.toLocalFile()))
{
}

Job::~Job()
{
}

bool Job::processEnded()
{
    Settings settings(m_config);

    QStringList caArgs;
    caArgs += argValue(settings.callgrindAnnotateParameters());
    caArgs += QStringLiteral("--tree=calling");
    caArgs += QStringLiteral("--threshold=100");
    caArgs += m_outputFile;

    QByteArray caOutput;
    if (executeProcess(settings.callgrindAnnotateExecutablePath(), caArgs, caOutput)) {
        return false;
    }

    Parser parser;
    parser.parse(caOutput, m_model);

    if (settings.launchKCachegrind()) {
        // Proxy used to remove file at the end of KCachegrind
        new QFileProxyRemove(settings.kcachegrindExecutablePath(),
                             { m_outputFile }, m_outputFile, dynamic_cast<QObject*>(m_plugin));
    }
    else {
        QFile::remove(m_outputFile);
    }

    return true;
}

void Job::addToolArgs(QStringList& args) const
{
    Settings settings(m_config);

    args += QStringLiteral("--callgrind-out-file=%1").arg(m_outputFile);

    args += argValue(settings.extraParameters());
    args += QStringLiteral("--cache-sim=") + argValue(settings.cacheSimulation());
    args += QStringLiteral("--branch-sim=") + argValue(settings.branchSimulation());
}

QWidget* Job::createView()
{
    return new View(m_model);
}

}

}
