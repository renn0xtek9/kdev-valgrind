/* This file is part of KDevelop
   Copyright 2006-2008 Hamish Rodda <rodda@kde.org>
   Copyright 2002 Harald Fernengel <harry@kdevelop.org>
   Copyright 2011 Mathieu Lornac <mathieu.lornac@gmail.com>
   Copyright 2011 Damien Coppel <damien.coppel@gmail.com>
   Copyright 2011 Lionel Duc <lionel.data@gmail.com>
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

#pragma once

#include <kconfiggroup.h>
#include <outputview/outputexecutejob.h>

class QWidget;

namespace KDevelop
{

class ILaunchConfiguration;

}

namespace Valgrind
{

class Plugin;

class IJob : public KDevelop::OutputExecuteJob
{
public:
    IJob(KDevelop::ILaunchConfiguration* cfg,
         QString tool,
         bool hasView,
         Plugin* plugin,
         QObject* parent);

    ~IJob() override;

    void start() override;
    using KDevelop::OutputExecuteJob::doKill;

    QString tool() const;
    QString target() const;

    bool hasView();
    virtual QWidget* createView() = 0;

protected:
    void postProcessStderr(const QStringList& lines) override;
    virtual void processValgrindOutput(const QStringList& lines);

    void childProcessExited(int exitCode, QProcess::ExitStatus exitStatus) override;
    void childProcessError(QProcess::ProcessError processError) override;

    virtual bool processEnded();

    virtual void addLoggingArgs(QStringList& args) const;
    virtual void addToolArgs(QStringList& args) const = 0;

    int executeProcess(const QString& executable, const QStringList& args, QByteArray& processOutput);

    QStringList buildCommandLine() const;

    KConfigGroup m_config;

    QString m_tool;
    bool m_hasView;

    Plugin* m_plugin;

    QString m_analyzedExecutable;
    QStringList m_analyzedExecutableArguments;

    QStringList m_valgrindOutput;

    quint16 m_tcpServerPort;
};

}