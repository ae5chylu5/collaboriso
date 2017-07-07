/********************************************************************

Name: Collaboriso
Homepage: http://github.com/ae5chylu5/collaboriso
Author: ae5chylu5
Description: A multiboot usb generator

Copyright (C) 2017 ae5chylu5

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************/

#include "settings.h"

Settings::Settings(QWidget *parent)
    : QDialog(parent)
{
    int winWidth = 500;
    int winHeight = 400;

    recursiveSort = new QCheckBox(tr("Sort recursively"));
    recursiveSort->setWhatsThis(tr("If nothing is selected then all tree items will be sorted recursively. If a menu is selected then only the selected menu will be sorted recursively. Selected ISOs are ignored."));

    QLabel *filesystemLabel = new QLabel(tr("File system:"));
    filesystemLabel->setStyleSheet(tr("QLabel { font-weight:700; }"));

    fsType = new QComboBox();
    fsType->addItems(QStringList() << "fat32" << "ntfs");
    fsType->setWhatsThis(tr("The file system you select here will be installed on the usb device."));

    QHBoxLayout *filesystemHbox = new QHBoxLayout();
    filesystemHbox->addStretch(1);
    filesystemHbox->addWidget(filesystemLabel);
    filesystemHbox->addWidget(fsType);
    filesystemHbox->addStretch(1);

    QLabel *selectBhvrLabel = new QLabel(tr("Selection behavior:"));
    selectBhvrLabel->setStyleSheet(tr("QLabel { font-weight:700; }"));

    selectBhvr = new QComboBox();
    selectBhvr->addItems(QStringList() << "Implicit" << "Explicit");
    selectBhvr->setWhatsThis(tr("Implicit: Unselected child items of a selected parent are assumed to be part of the selection.\n\nExplicit: Each item must be selected to be part of the selection."));

    QHBoxLayout *selectBhvrHbox = new QHBoxLayout();
    selectBhvrHbox->addStretch(1);
    selectBhvrHbox->addWidget(selectBhvrLabel);
    selectBhvrHbox->addWidget(selectBhvr);
    selectBhvrHbox->addStretch(1);

    QLabel *grubHeadBhvrLabel = new QLabel(tr("Prepend grub header:"));
    grubHeadBhvrLabel->setStyleSheet(tr("QLabel { font-weight:700; }"));

    grubHeadBhvr = new QComboBox();
    grubHeadBhvr->addItems(QStringList() << "Disabled" << "Main Menu" << "All Menus");
    grubHeadBhvr->setWhatsThis(tr("Main Menu: Prepend grub header options from text box in this window to main grub.cfg file.\n\nAll Menus: Prepend grub header options to all .cfg menus."));

    QHBoxLayout *grubHeadBhvrHbox = new QHBoxLayout();
    grubHeadBhvrHbox->addStretch(1);
    grubHeadBhvrHbox->addWidget(grubHeadBhvrLabel);
    grubHeadBhvrHbox->addWidget(grubHeadBhvr);
    grubHeadBhvrHbox->addStretch(1);

    QLabel *grubHeadLabel = new QLabel(tr("Grub header"));
    grubHeadLabel->setStyleSheet(tr("QLabel { font-weight:700; }"));

    grubHeadText = new QTextEdit();
    grubHeadText->setWhatsThis(tr("Enter custom commands to be included as header for the grub.cfg file."));

    QVBoxLayout *vboxLayout = new QVBoxLayout();
    vboxLayout->addWidget(recursiveSort, 0, Qt::AlignHCenter);
    vboxLayout->addLayout(filesystemHbox);
    vboxLayout->addLayout(selectBhvrHbox);
    vboxLayout->addLayout(grubHeadBhvrHbox);
    vboxLayout->addWidget(grubHeadLabel);
    vboxLayout->addWidget(grubHeadText, 1);
    setLayout(vboxLayout);

    setWindowTitle("Settings");
    setWindowIcon(QIcon(":/images/settings48.png"));
    resize(winWidth, winHeight);
    importSettings();
}

void Settings::saveSettings()
{
    QSettings settings("ISOCLUB", "Collaboriso");
    settings.setValue("settings/selectionBehavior", selectBhvr->currentIndex());
    settings.setValue("settings/grubHeaderBehavior", grubHeadBhvr->currentIndex());
    settings.setValue("settings/filesystemType", fsType->currentIndex());
    settings.setValue("settings/sortRecursively", recursiveSort->isChecked());

    CU utils;
    if (utils.mkHomeDir()) {
        QFile file(utils.collabGrub);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&file);
            out << grubHeadText->toPlainText();
            file.close();
            file.deleteLater();
        }
    }
}

void Settings::importSettings()
{
    QSettings settings("ISOCLUB", "Collaboriso");
    selectBhvr->setCurrentIndex(settings.value("settings/selectionBehavior").toInt());
    grubHeadBhvr->setCurrentIndex(settings.value("settings/grubHeaderBehavior").toInt());
    fsType->setCurrentIndex(settings.value("settings/filesystemType").toInt());
    recursiveSort->setChecked(settings.value("settings/sortRecursively").toBool());

    CU utils;
    QFile file(utils.collabGrub);
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        grubHeadText->setPlainText(file.readAll());
        file.close();
        file.deleteLater();
    }
}

void Settings::showEvent(QShowEvent *event)
{
    importSettings();
    event->accept();
}

void Settings::hideEvent(QHideEvent *event)
{
    saveSettings();
    event->accept();
}
