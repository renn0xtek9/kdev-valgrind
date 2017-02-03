/* This file is part of KDevelop
 * Copyright 2011 Sebastien Rannou <mxs@sbrk.org>
 * Copyright 2011 Lionel Duc <lionel.data@gmail.com>
 * Copyright 2017 Anton Anikin <anton.anikin@htower.ru>

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

#include "configpage.h"
#include "ui_configpage.h"

#include "settings.h"

#include <KConfigGroup>

namespace valgrind
{

MassifConfigPage::MassifConfigPage(QWidget* parent)
    : LaunchConfigurationPage(parent)
{
    ui = new Ui::MassifConfig();
    ui->setupUi(this);

    connect(ui->extraParameters, &QLineEdit::textEdited, this, &MassifConfigPage::changed);
    connect(ui->launchMassifVisualizer, &QCheckBox::toggled, this, &MassifConfigPage::changed);
    connect(ui->depth, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MassifConfigPage::changed);
    connect(ui->threshold, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MassifConfigPage::changed);
    connect(ui->peakInaccuracy, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MassifConfigPage::changed);
    connect(ui->maxSnapshots, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MassifConfigPage::changed);
    connect(ui->snapshotFreq, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MassifConfigPage::changed);
    connect(ui->timeUnit, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MassifConfigPage::changed);
    connect(ui->profileHeap, &QCheckBox::toggled, this, &MassifConfigPage::changed);
    connect(ui->profileStack, &QCheckBox::toggled, this, &MassifConfigPage::changed);
}

QString MassifConfigPage::title() const
{
    return i18n("Massif");
}

QIcon MassifConfigPage::icon() const
{
    return QIcon::fromTheme("fork");
}

void MassifConfigPage::loadFromConfiguration(const KConfigGroup& cfg, KDevelop::IProject*)
{
    QSignalBlocker blocker(this);
    MassifSettings settings(cfg);

    ui->extraParameters->setText(settings.extraParameters());
    ui->depth->setValue(settings.snapshotTreeDepth());
    ui->threshold->setValue(settings.threshold());
    ui->peakInaccuracy->setValue(settings.peakInaccuracy());
    ui->maxSnapshots->setValue(settings.maximumSnapshots());
    ui->snapshotFreq->setValue(settings.detailedSnapshotsFrequency());
    ui->timeUnit->setCurrentIndex(settings.timeUnit());
    ui->profileHeap->setChecked(settings.profileHeap());
    ui->profileStack->setChecked(settings.profileStack());
    ui->launchMassifVisualizer->setChecked(settings.launchVisualizer());
}

void MassifConfigPage::saveToConfiguration(KConfigGroup cfg, KDevelop::IProject*) const
{
    MassifSettings settings(cfg);

    settings.setExtraParameters(ui->extraParameters->text());
    settings.setSnapshotTreeDepth(ui->depth->value());
    settings.setThreshold(ui->threshold->value());
    settings.setPeakInaccuracy(ui->peakInaccuracy->value());
    settings.setMaximumSnapshots(ui->maxSnapshots->value());
    settings.setDetailedSnapshotsFrequency(ui->snapshotFreq->value());
    settings.setTimeUnit(ui->timeUnit->currentIndex());
    settings.setProfileHeap(ui->profileHeap->isChecked());
    settings.setProfileStack(ui->profileStack->isChecked());
    settings.setLaunchVisualizer(ui->launchMassifVisualizer->isChecked());
}

MassifConfigPageFactory::MassifConfigPageFactory()
{
}

MassifConfigPageFactory::~MassifConfigPageFactory()
{
}

KDevelop::LaunchConfigurationPage* MassifConfigPageFactory::createWidget(QWidget* parent)
{
    return new MassifConfigPage(parent);
}

}
