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

#ifndef CUTILS_H
#define CUTILS_H

#include <QDate>
#include <QDir>
#include <QDebug>
#include <QLocale>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSysInfo>

class CU
{

public:
    CU();
    bool mkHomeDir();
    bool prepareDB();
    QString collabVers;
    QString updateURL;
    QString userAgent;
    QString collabHome;
    QString collabDB;
    QString collabGrub;
    QString isoTable;
    QString isoRoot;
    QString grubRoot;
    QString menuRoot;
    //QString catTable;
    QStringList reqApps;
    enum ItemMode { Root, Menu, Iso };
    enum IsoMode { Unsupported, Supported, Unknown };
    enum MoveMode { Implicit, Selected, Unselected };
    // Qt::UserRole starts at 0x0100 so we define our custom
    // roles beginning at 101
    enum DataRoles { IsoRole = 0x0101, PathRole, CategoryRole, HashRole, ItemRole, UUIDRole, MountRole };
};

#endif // CUTILS_H
