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

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>

#include <unistd.h>

QString defaultMountPoint;
QStringList vFolders;
QStringList reqApps;

// return list of apps not found
QStringList hasApps(QStringList apps) {
    QString path;
    QStringList notFound;
    for (int i=0,total=apps.size(); i<total; ++i) {
        path = QStandardPaths::findExecutable(apps[i]);
        if (path.isEmpty()) {
            notFound << apps[i];
        }
    }
    return notFound;
}

// return list of partitions for specified device
QStringList getParts(QString device, int repeat = 0)
{
    QStringList parts;

    QProcess lsblk;
    lsblk.start(QObject::tr("lsblk -Jpno NAME,MOUNTPOINT %1").arg(device));
    if (!lsblk.waitForStarted() || !lsblk.waitForFinished()) return parts;

    QByteArray jsonRaw = lsblk.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonRaw);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) return parts;

    QJsonObject jsonRootObj = jsonDoc.object();
    QJsonArray devices = jsonRootObj["blockdevices"].toArray();
    if (devices.size() != 1) return parts;

    QJsonValue jsonVal = devices.at(0);
    QJsonObject dev = jsonVal.toObject();
    QJsonArray devChildren = dev["children"].toArray();

    QJsonObject partition;
    QString partStr;
    for (int i = 0, total = devChildren.size(); i < total; ++i) {
        jsonVal = devChildren.at(i);
        partition = jsonVal.toObject();
        partStr = partition["name"].toString();
        if (!partStr.isEmpty()) parts << partStr;
    }

    for (int i = 0; i < repeat && parts.size() < 1; ++i) {
        // occasionally the previously run command, like parted, may still be
        // in process after the finished signal is received. so if we try to
        // retrieve the partitions they may not all be visible to lsblk. we
        // can specify a number of iterations to continue and try to retrieve
        // the partitions if they are not detected the first time.
        qDebug() << "No partitions detected on the usb device. Trying again...";
        parts = getParts(device);
    }

    return parts;
}

// return list of mount points for specified device
QStringList getMounts(QString device, int repeat = 0)
{
    QStringList mounts;

    QProcess lsblk;
    lsblk.start(QObject::tr("lsblk -Jpno NAME,MOUNTPOINT %1").arg(device));
    if (!lsblk.waitForStarted() || !lsblk.waitForFinished()) return mounts;

    QByteArray jsonRaw = lsblk.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonRaw);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) return mounts;

    QJsonObject jsonRootObj = jsonDoc.object();
    QJsonArray devices = jsonRootObj["blockdevices"].toArray();
    if (devices.size() != 1) return mounts;

    QJsonValue jsonVal = devices.at(0);
    QJsonObject dev = jsonVal.toObject();
    QJsonArray devChildren = dev["children"].toArray();

    QJsonObject partition;
    QString mountStr;
    for (int i = 0, total = devChildren.size(); i < total; ++i) {
        jsonVal = devChildren.at(i);
        partition = jsonVal.toObject();
        mountStr = partition["mountpoint"].toString();
        if (!mountStr.isEmpty()) mounts << mountStr;
    }

    for (int i = 0; i < repeat && mounts.size() < 1; ++i) {
        // occasionally the previously run command, like parted, may still be
        // in process after the finished signal is received. so if we try to
        // retrieve the mount points they may not all be visible to lsblk. we
        // can specify a number of iterations to continue and try to retrieve
        // the mount points if they are not detected the first time.
        qDebug() << "No mount points detected on the usb device. Trying again...";
        mounts = getMounts(device);
    }

    return mounts;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    defaultMountPoint = "/mnt/collaboriso";
    vFolders << "/dev" << "/proc" << "/sys" << "/usr" << "/bin" << "/lib" << "/lib64";
    reqApps << "lsblk" << "umount" << "parted" << "mkfs.fat" << "mkntfs" << "mount" << "chroot" << "grub-install";

    if (getuid()) {
        qDebug() << "You must run this application as root.";
        return 1;
    }

    // begin req apps test
    QStringList nf = hasApps(reqApps);
    if (nf.size() > 0) {
        qDebug() << "These apps cannot be found:";
        qDebug() << nf;
        return 1;
    }
    // end req apps test

    // begin CLI options parser
    QCommandLineParser parser;
    //QCommandLineOption ignoreOption("ignore", QCoreApplication::translate("main", "Ignore warnings"));
    //parser.addOption(ignoreOption);
    QCommandLineOption targetOption(QStringList() << "t" << "target-device",
        QCoreApplication::translate("main", "Partition, format and install grub on <device>."),
        QCoreApplication::translate("main", "device"));
    parser.addOption(targetOption);
    QCommandLineOption fsOption(QStringList() << "f" << "filesystem-type",
        QCoreApplication::translate("main", "Format device as <filesystem>."),
        QCoreApplication::translate("main", "filesystem"));
    parser.addOption(fsOption);
    parser.process(a);
    //bool ignore = parser.isSet(ignoreOption);
    QString targetDev = parser.value(targetOption);
    QString fsType = parser.value(fsOption);
    if (targetDev.isEmpty() || fsType.isEmpty() || !fsType.contains(QRegExp("^fat32$|^ntfs$"))) {
        qDebug() << "You must specify a target device and valid file system.";
        return 1;
    }
    // end CLI options parser

    // unmount all usb partitions
    QProcess umount;
    QStringList mounts = getMounts(targetDev);
    for (int i = 0, total = mounts.size(); i < total; ++i) {
        umount.start(QObject::tr("umount -l %1").arg(mounts[i]));
        if (!umount.waitForStarted() || !umount.waitForFinished()) return 1;
        if (!umount.readAll().isEmpty()) qDebug() << umount.readAll();
    }

    QProcess parted;
    parted.start(QObject::tr("parted -s %1 mklabel msdos").arg(targetDev));
    if (!parted.waitForStarted() || !parted.waitForFinished()) return 1;
    if (!parted.readAll().isEmpty()) qDebug() << parted.readAll();

    parted.start(QObject::tr("parted -s -a opt %1 mkpart primary %2 0% 100%").arg(targetDev).arg(fsType));
    if (!parted.waitForStarted() || !parted.waitForFinished()) return 1;
    if (!parted.readAll().isEmpty()) qDebug() << parted.readAll();

    QStringList parts = getParts(targetDev, 3);
    if (parts.size() < 1) {
        qDebug() << "No partitions detected on the usb device. Exiting!";
        return 1;
    }

    QProcess mkfs;
    mkfs.start(QObject::tr("%1 COLLABORISO %2").arg((fsType == "fat32") ? "mkfs.fat -n" : "mkntfs -L").arg(parts[0]));
    if (!mkfs.waitForStarted() || !mkfs.waitForFinished()) return 1;
    if (!mkfs.readAll().isEmpty()) qDebug() << mkfs.readAll();

    // set the boot flag
    parted.start(QObject::tr("parted -s %1 set %2 boot on").arg(targetDev).arg("1"));
    if (!parted.waitForStarted() || !parted.waitForFinished()) return 1;
    if (!parted.readAll().isEmpty()) qDebug() << parted.readAll();

    QDir mp = QDir(defaultMountPoint);
    if (!mp.exists()) {
        if (!mp.mkpath(defaultMountPoint)) {
            qDebug() << "Unable to create mount directory.";
            return 1;
        }
    }

    parts.clear();
    parts = getParts(targetDev, 3);
    if (parts.size() < 1) {
        qDebug() << "No partitions detected on the usb device. Exiting!";
        return 1;
    }

    // we need non-root user to have write access so we set the umask.
    // the isos are extracted after the cli app is completed and we
    // won't have write access from the gui without the umask setting.
    QProcess mount;
    mount.start(QObject::tr("mount -o umask=000 %1 %2").arg(parts[0]).arg(defaultMountPoint));
    if (!mount.waitForStarted() || !mount.waitForFinished()) return 1;
    if (!mount.readAll().isEmpty()) qDebug() << mount.readAll();

    mounts.clear();
    mounts = getMounts(targetDev, 3);
    if (mounts.size() < 1) {
        qDebug() << "No mount points detected on the usb device. Exiting!";
        return 1;
    }

    // create and mount virtual folders for chroot
    QDir vdir;
    QFileInfo finfo;
    for (int i = 0, total = vFolders.size(); i < total; ++i) {
        finfo.setFile(vFolders[i]);
        if (finfo.exists()) {
            vdir.setPath(QObject::tr("%1%2").arg(defaultMountPoint).arg(vFolders[i]));
            if (!vdir.exists()) {
                if (!vdir.mkpath(vdir.path())) {
                    qDebug() << "Unable to create virtual directory";
                    return 1;
                }
            }
            mount.start(QObject::tr("mount --bind %1 %2").arg(vFolders[i]).arg(vdir.path()));
            if (!mount.waitForStarted() || !mount.waitForFinished()) return 1;
            if (!mount.readAll().isEmpty()) qDebug() << mount.readAll();
        }
    }

    QProcess grubInstall;
    grubInstall.start(QObject::tr("chroot %1 grub-install --recheck %2").arg(defaultMountPoint).arg(targetDev));
    if (!grubInstall.waitForStarted() || !grubInstall.waitForFinished()) return 1;
    if (!grubInstall.readAll().isEmpty()) qDebug() << grubInstall.readAll();

    // unmount all virtual folders
    for (int i = 0, total = vFolders.size(); i < total; ++i) {
        finfo.setFile(vFolders[i]);
        if (finfo.exists()) {
            umount.start(QObject::tr("umount -l %1%2").arg(defaultMountPoint).arg(vFolders[i]));
            if (!umount.waitForStarted() || !umount.waitForFinished()) return 1;
            if (!umount.readAll().isEmpty()) qDebug() << umount.readAll();
        }
    }

    return 0;
    //return a.exec();
}
