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

#include "device.h"

DeviceSelector::DeviceSelector(QWidget *parent)
    : QDialog(parent)
{
    int winWidth = 300;
    int winHeight = 184;

    usbTable = new QTableWidget();
    usbTable->setColumnCount(1);
    usbTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    usbTable->setSelectionMode(QAbstractItemView::SingleSelection);
    usbTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    usbTable->verticalHeader()->hide();
    usbTable->horizontalHeader()->hide();
    usbTable->horizontalHeader()->setStretchLastSection(1);
    usbTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);

    QHBoxLayout *hboxLayout = new QHBoxLayout();
    hboxLayout->addWidget(usbTable, 1);

    QVBoxLayout *vboxLayout = new QVBoxLayout();
    vboxLayout->addLayout(hboxLayout, 1);
    setLayout(vboxLayout);

    setModal(1);
    setWindowModality(Qt::ApplicationModal);
    setWindowTitle("USB Devices");
    setWindowIcon(QIcon(":/images/select48.png"));
    resize(winWidth, winHeight);

    // creates monitor for usb devices that will refresh every 10 seconds
    timer = new QTimer(this);
    timer->setInterval(10000); // 10 seconds
    connect(timer, SIGNAL(timeout()), this, SLOT(getUSBDevices()));
}

bool DeviceSelector::isUSB(bool hotplug, QString subsystems, QString transport) {
    if (hotplug == 1 && subsystems.contains(":usb:") && transport == "usb") {
        return 1;
    }
    return 0;
}

void DeviceSelector::getUSBDevices()
{
    clearTable();

    int row;
    bool hotplug;
    QString devLabel, devPath, devUUID, devMountPoint;
    QJsonValue jsonVal2;
    QJsonObject dev, partition;
    QJsonArray devChildren;
    QTableWidgetItem *twi;

    QProcess lsblk;
    lsblk.start("lsblk -Jpno VENDOR,SIZE,NAME,HOTPLUG,SUBSYSTEMS,TRAN");
    if (!lsblk.waitForStarted(-1) || !lsblk.waitForFinished(-1)) return;

    QByteArray jsonRaw = lsblk.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonRaw);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) return;

    QJsonObject jsonRootObj = jsonDoc.object();
    QJsonArray devices = jsonRootObj["blockdevices"].toArray();
    foreach (const QJsonValue & jsonVal, devices) {
        dev = jsonVal.toObject();
        // different versions of lsblk return different variable types for hotplug.
        // can be either bool or integer as a string. ex: "1"
        hotplug = (dev["hotplug"].isBool()) ? dev["hotplug"].toBool() : dev["hotplug"].toString().toInt();
        if (!isUSB(hotplug, dev["subsystems"].toString(), dev["tran"].toString())) continue;

        devPath = dev["name"].toString();
        devLabel = tr("%1 %2 %3").arg(dev["vendor"].toString().trimmed()).arg(dev["size"].toString()).arg(devPath);

        twi = new QTableWidgetItem(devLabel);
        twi->setTextAlignment(Qt::AlignCenter);
        twi->setData(CU::PathRole, devPath);

        if (dev.contains("children")) {
            devChildren = dev["children"].toArray();
            if (devChildren.size() > 0) {
                jsonVal2 = devChildren.at(0);
                partition = jsonVal2.toObject();
                devUUID = partition["uuid"].toString();
                devMountPoint = partition["mountpoint"].toString();
                twi->setData(CU::UUIDRole, devUUID);
                twi->setData(CU::MountRole, devMountPoint);
            }
        }

        row = usbTable->rowCount();
        usbTable->insertRow(row);
        usbTable->setItem(row, 0, twi);
    }
}

void DeviceSelector::clearTable()
{
    int rows = usbTable->rowCount();
    for (int i = rows - 1; i >= 0; --i) {
        usbTable->removeRow(i);
    }
}

void DeviceSelector::showEvent(QShowEvent *event)
{
    getUSBDevices();
    timer->start();
    event->accept();
}

void DeviceSelector::hideEvent(QHideEvent *event)
{
    timer->stop();
    event->accept();
}
