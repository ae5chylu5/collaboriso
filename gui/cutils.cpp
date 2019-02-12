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

#include "cutils.h"

CU::CU()
{
    QLocale locale;

    collabVers = "0.1a1pre";

    updateURL = "http://127.0.0.1/?time={{TIMESTAMP}}";

    userAgent = QObject::tr("Collaboriso/%1 (%2; %3; qt:%4) %5").
                         arg(collabVers).
                         arg(QSysInfo::productType()).
                         arg(locale.name()).
                         arg(QT_VERSION_STR).
                         arg(QDate::currentDate().toString("MMddyy"));

    collabHome = QDir::homePath() + QDir::separator() + ".collaboriso";

    collabDB = collabHome + QDir::separator() + "collaboriso.sqlite";

    collabGrub = collabHome + QDir::separator() + "grub.cfg";

    isoRoot = "/isos";
    grubRoot = "/boot/grub";
    menuRoot = "/boot/grub/menus";

    isoTable = QObject::tr("CREATE TABLE IF NOT EXISTS isos (id INTEGER PRIMARY KEY AUTOINCREMENT, bootType INTEGER, strictHash INTEGER, ").
                        append("title VARCHAR(256), hash VARCHAR(40), category VARCHAR(64), bootOptions TEXT, created TIMESTAMP, ").
                        append("modified TIMESTAMP, UNIQUE (hash))");

    //catTable = tr("CREATE TABLE IF NOT EXISTS categories (id INTEGER PRIMARY KEY AUTOINCREMENT, category VARCHAR(64), UNIQUE (category))");

    reqApps << "umount" << "parted" << "mkfs.fat" << "mkntfs" << "chroot" << "grub-install" << "lsblk" << "mount" << "7z" << "collaboriso_cli" << "pkexec";
}

bool CU::mkHomeDir()
{
    QDir hd = QDir(collabHome);
    if (!hd.exists()) {
        if (!hd.mkpath(collabHome)) {
            qDebug() << "Unable to create collaboriso directory.";
            return 0;
        }
    }
    return 1;
}

bool CU::prepareDB()
{
    if (!mkHomeDir()) return 0;

    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        qDebug() << "Unable to access the sqlite driver.";
        return 0;
    }

    if (!QSqlDatabase::contains("ISODB")) {
        QSqlDatabase sqldb = QSqlDatabase::addDatabase("QSQLITE", "ISODB");
        sqldb.setDatabaseName(collabDB);
        if (!sqldb.open()) {
            qDebug() << "Unable to establish a database connection.";
            return 0;
        }

        //qDebug() << sqldb.driver()->hasFeature(QSqlDriver::Transactions);

        // create tables if they don't exist
        QSqlQuery query(sqldb);
        bool success = query.exec(isoTable);
        if (!success) {
            qDebug() << query.lastError().text();
        }

        sqldb.commit();
        sqldb.close();
        return success;
    }
    return 1;
}
