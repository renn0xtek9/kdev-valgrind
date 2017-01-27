/* This file is part of KDevelop
   Copyright 2011 Mathieu Lornac <mathieu.lornac@gmail.com>
   Copyright 2011 Damien Coppel <damien.coppel@gmail.com>
   Copyright 2011 Lionel Duc <lionel.data@gmail.com>
   Copyright 2011 Sebastien Rannou <mxs@sbrk.org>
   Copyright 2011 Lucas Sarie <lucas.sarie@gmail.com>
   Copyright 2006-2008 Hamish Rodda <rodda@kde.org>
   Copyright 2002 Harald Fernengel <harry@kdevelop.org>
   Copyright 2016 Anton Anikin <anton.anikin@htower.ru>

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

#include "ijob.h"

#include "iparser.h"
#include "debug.h"
#include "modelwrapper.h"
#include "plugin.h"

#include "globalsettings.h"

#include "cachegrind/job.h"
#include "callgrind/job.h"
#include "massif/job.h"
#include "memcheck/job.h"

#include <execute/iexecuteplugin.h>
#include <interfaces/icore.h>
#include <interfaces/ilaunchconfiguration.h>
#include <interfaces/iplugincontroller.h>
#include <kconfiggroup.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kshell.h>
#include <util/environmentgrouplist.h>

#include <QApplication>
#include <QBuffer>
#include <QFileInfo>

namespace valgrind
{

// The factory for jobs
IJob* IJob::createToolJob(KDevelop::ILaunchConfiguration* cfg, Plugin* plugin, QObject* parent)
{
    const QString& name = cfg->config().readEntry(QStringLiteral("Current Tool"), QStringLiteral("memcheck"));

    if (name == QStringLiteral("memcheck"))
        return new MemcheckJob(cfg, plugin, parent);

    else if (name == QStringLiteral("massif"))
        return new MassifJob(cfg, plugin, parent);

    else if (name == QStringLiteral("cachegrind"))
        return new CachegrindJob(cfg, plugin, parent);

    else if (name == QStringLiteral("callgrind"))
        return new CallgrindJob(cfg, plugin, parent);

    qCDebug(KDEV_VALGRIND) << "can't create this job, " << name << "unknow job";

    return nullptr;
}

IJob::IJob(
    KDevelop::ILaunchConfiguration* cfg,
    QString tool,
    IModel* model,
    IParser* parser,
    Plugin* plugin,
    QObject* parent)

    : KDevelop::OutputExecuteJob(parent)
    , m_launchcfg(cfg)
    , m_tool(tool)
    , m_model(model)
    , m_parser(parser)
    , m_plugin(plugin)
{
    Q_ASSERT(m_launchcfg);
    Q_ASSERT(m_model);
    Q_ASSERT(m_parser);
    Q_ASSERT(m_plugin);

    setProperties(KDevelop::OutputExecuteJob::JobProperty::DisplayStdout);
    setProperties(KDevelop::OutputExecuteJob::JobProperty::DisplayStderr);
    setProperties(KDevelop::OutputExecuteJob::JobProperty::PostProcessOutput);

    setCapabilities(KJob::Killable);
    setStandardToolView(KDevelop::IOutputView::TestView);
    setBehaviours(KDevelop::IOutputView::AutoScroll);

    KConfigGroup config(m_launchcfg->config());

    IExecutePlugin* iface = KDevelop::ICore::self()->pluginController()->pluginForExtension("org.kdevelop.IExecutePlugin")->extension<IExecutePlugin>();
    Q_ASSERT(iface);

    KDevelop::EnvironmentGroupList l(KSharedConfig::openConfig());
    QString envgrp = iface->environmentGroup(m_launchcfg);

    if (envgrp.isEmpty()) {
        qCWarning(KDEV_VALGRIND) << i18n("No environment group specified, looks like a broken "
                           "configuration, please check run configuration '%1'. "
                           "Using default environment group.", m_launchcfg->name());
        envgrp = l.defaultGroup();
    }
    // FIXME
//     m_process->setEnvironment(l.createEnvironment(envgrp, m_process->systemEnvironment()));

    QString errorString;

    m_valgrindExecutable = KDevelop::Path(GlobalSettings::valgrindExecutablePath()).toLocalFile();

    m_analyzedExecutable = iface->executable(m_launchcfg, errorString).toLocalFile();
    if (!errorString.isEmpty()) {
        setError(-1);
        setErrorText(errorString);
    }

    m_analyzedExecutableArguments = iface->arguments(m_launchcfg, errorString);
    if (!errorString.isEmpty()) {
        setError(-1);
        setErrorText(errorString);
    }

    m_workingDir = iface->workingDirectory(m_launchcfg);
    if (m_workingDir.isEmpty() || !m_workingDir.isValid())
        m_workingDir = QUrl::fromLocalFile(QFileInfo(m_analyzedExecutable).absolutePath());
//     setWorkingDirectory(m_workingDir.toLocalFile()); // FIXME

    connect(m_parser, &IParser::newElement, this, [this](IModel::eElementType t){
        m_model->newElement(t);
    });

    connect(m_parser, &IParser::newData, this,
            [this](IModel::eElementType t, const QString& name, const QString& value){
        m_model->newData(t, name, value);
    });

    connect(m_parser, &IParser::newItem, this, [this](ModelItem* item){
        m_model->newItem(item);
    });

    connect(m_parser, &IParser::reset, this, [this](){
        m_model->reset();
    });

    m_model->setModelWrapper(new ModelWrapper(m_model));
    m_model->modelWrapper()->job(this);
    m_model->reset();

    m_plugin->incomingModel(m_model);
}

IJob::~IJob()
{
    doKill();
    if (m_model && m_model->modelWrapper())
        m_model->modelWrapper()->job(nullptr);

    delete m_parser;
}

QString IJob::tool() const
{
    return m_tool;
}

QString IJob::target() const
{
    return QFileInfo(m_analyzedExecutable).fileName();
}

void IJob::processModeArgs(
    QStringList& out,
    const t_valgrind_cfg_argarray modeArgs,
    int modeArgsCount,
    KConfigGroup& config) const
{
    // For each option, set the right string in the arguments list
    for (int i = 0; i < modeArgsCount; ++i) {
        QString val;
        QString argType = modeArgs[i][2];

        if (argType == QStringLiteral("str"))
            val = config.readEntry(modeArgs[i][0]);

        else if (argType == QStringLiteral("int")) {
            int n = config.readEntry(modeArgs[i][0], 0);
            if (n)
                val.sprintf("%d", n);
        }

        else if (argType == QStringLiteral("bool")) {
            bool n = config.readEntry(modeArgs[i][0], false);
            val = n ? QStringLiteral("yes") : QStringLiteral("no");
        }

        else if (argType == QStringLiteral("float")) {
            int n = config.readEntry(modeArgs[i][0], 1);
            val.sprintf("%d.0", n);
        }

        else if (argType == QStringLiteral("float")) {
            int n = config.readEntry(modeArgs[i][0], 1);
            val.sprintf("%d.0", n);
        }

        if (!val.isEmpty())
            out += QString(modeArgs[i][1]) + val;
    }
}

QStringList IJob::buildCommandLine() const
{
    static const t_valgrind_cfg_argarray genericArgs = {
        {QStringLiteral("Current Tool"), QStringLiteral("--tool="), QStringLiteral("str")},
        {QStringLiteral("Stackframe Depth"), QStringLiteral("--num-callers="), QStringLiteral("int")},
        {QStringLiteral("Maximum Stackframe Size"), QStringLiteral("--max-stackframe="), QStringLiteral("int")},
        {QStringLiteral("Limit Errors"), QStringLiteral("--error-limit="), QStringLiteral("bool")}
    };
    static const int genericArgsCount = sizeof(genericArgs) / sizeof(*genericArgs);

    KConfigGroup config = m_launchcfg->config();
    QStringList result = KShell::splitArgs(config.readEntry(QStringLiteral("Valgrind Arguments"),
                                                            QStringLiteral("")));

    processModeArgs(result, genericArgs, genericArgsCount, config);
    addToolArgs(result, config);

    return result;
}

void IJob::start()
{
    if (error()) {
        emitResult();
        return;
    }

    *this << m_valgrindExecutable;
    *this << buildCommandLine();
    *this << m_analyzedExecutable;
    *this << m_analyzedExecutableArguments;

   qCDebug(KDEV_VALGRIND) << "executing:" << commandLine().join(' ');

    beforeStart();
    KDevelop::OutputExecuteJob::start();
    processStarted();
}

void IJob::postProcessStdout(const QStringList& lines)
{
    m_standardOutput << lines;
    KDevelop::OutputExecuteJob::postProcessStdout(lines);
}

void IJob::postProcessStderr(const QStringList& lines)
{
    m_errorOutput << lines;
    KDevelop::OutputExecuteJob::postProcessStderr(lines);
}

void IJob::childProcessExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    qCDebug(KDEV_VALGRIND) << "Process Finished, exitCode" << exitCode << "process exit status" << exitStatus;

    if (exitCode) {
        /*
        ** Here, check if Valgrind failed (because of bad parameters or whatever).
        ** Because Valgrind always returns 1 on failure, and the profiled application's return
        ** on success, we cannot know for sure which process returned != 0.
        **
        ** The only way to guess that it is Valgrind which failed is to check stderr and look for
        ** "valgrind:" at the beginning of each line, even though it can still be the profiled
        ** process that writes it on stderr. It is, however, unlikely enough to be reliable in
        ** most cases.
        */
        const QString s = m_errorOutput.join(' ');

        if (s.startsWith("valgrind:")) {
            QString err = s.split("\n")[0];
            err = err.replace("valgrind:", "");
            err += "\n\nPlease review your Valgrind launch configuration.";

            KMessageBox::error(qApp->activeWindow(), err, i18n("Valgrind Error"));
        }
    }

    processEnded();
    emitResult();

    m_plugin->jobFinished(this);
}

void IJob::childProcessError(QProcess::ProcessError processError)
{
    QString errorMessage;

    switch (processError) {

    case QProcess::FailedToStart:
        errorMessage = i18n("Failed to start valgrind from \"%1\".", commandLine().first());
        break;

    case QProcess::Crashed:
        // if the process was killed by the user, the crash was expected
        // don't notify the user
        if (status() != KDevelop::OutputExecuteJob::JobStatus::JobCanceled)
            errorMessage = i18n("Valgrind crashed.");
        break;

    case QProcess::Timedout:
        errorMessage = i18n("Valgrind process timed out.");
        break;

    case QProcess::WriteError:
        errorMessage = i18n("Write to Valgrind process failed.");
        break;

    case QProcess::ReadError:
        errorMessage = i18n("Read from Valgrind process failed.");
        break;

    case QProcess::UnknownError:
        errorMessage = i18n("Unknown Valgrind process error.");
        break;
    }

    if (!errorMessage.isEmpty())
        KMessageBox::error(qApp->activeWindow(), errorMessage, i18n("Valgrind Error"));

    KDevelop::OutputExecuteJob::childProcessError(processError);
}

void IJob::beforeStart()
{
}

void IJob::processStarted()
{
}

void IJob::processEnded()
{
}

/**
 * KProcessOutputToParser implementation
 */
KProcessOutputToParser::KProcessOutputToParser(IParser* parser)
    : m_process(new QProcess)
    , m_device(new QBuffer)
    , m_parser(parser)
{
    m_device->open(QIODevice::WriteOnly);
}

KProcessOutputToParser::~KProcessOutputToParser()
{
    if (m_device)
        delete m_device;

    if (m_process)
        delete m_process;
}

int KProcessOutputToParser::execute(const QString& execPath, const QStringList& args)
{
    // execute and wait the end of the program
    m_process->start(execPath, args);
    if (m_process->waitForFinished()) {
        m_device->write(m_process->readAllStandardOutput());
        m_device->close();
        m_device->open(QIODevice::ReadOnly);

        m_parser->setDevice(m_device);
        m_parser->parse();
    }

    return m_process->exitCode();
}

/**
 * QFileProxyRemove Implementation
 */
QFileProxyRemove::QFileProxyRemove(
    const QString& programPath,
    const QStringList& args,
    const QString& fileName,
    QObject* parent)

    : QObject(parent)
    , m_file(new QFile(fileName))
    , m_process(new QProcess(this))
    , m_execPath(programPath)
{
    connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this](int, QProcess::ExitStatus) {
        deleteLater();
    });

    connect(m_process, static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
            this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            KMessageBox::error(qApp->activeWindow(),
                               i18n("Unable to launch the process %1 (%2)", m_execPath, error),
                               i18n("Valgrind Error"));
        deleteLater();
    });

    m_process->start(programPath, args);
}

QFileProxyRemove::~QFileProxyRemove()
{
    m_file->remove();
    delete m_file;

    delete m_process;
}

}