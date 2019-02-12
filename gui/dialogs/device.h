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

#ifndef DEVICE_H
#define DEVICE_H

#include <QApplication>
#include <QDialog>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLayout>
#include <QProcess>
#include <QTableWidget>
#include <QTimer>
#include <QtEvents>

#include "cutils.h"

class DeviceSelector : public QDialog
{
    Q_OBJECT

public:
    DeviceSelector(QWidget *parent = 0);
    QTableWidget *usbTable;

private:
    void clearTable();
    bool isUSB(bool hotplug, QString subsystems, QString transport);
    QTimer *timer;

private slots:
    void getUSBDevices();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
};

#endif // DEVICE_H
