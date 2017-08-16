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

#include "collaboriso.h"

Collaboriso::Collaboriso(QWidget *parent)
    : QMainWindow(parent)
{
    int winWidth = 450;
    int winHeight = 500;

    cliPath = "collaboriso_cli";
    suApp = "kdesudo";

    createToolBars();
    createStatusBar();
    createTreeWidget();

    setMinimumSize(winWidth,winHeight);
    setWindowTitle("Collaboriso");
    setWindowIcon(QIcon(":/images/collaboriso48.png"));

    devDlg = new DeviceSelector();
    booDlg = new BootSelector();
    proDlg = new ProgressMeter();
    setDlg = new Settings();
    abtDlg = new About();

    netReply = 0;
    naMgr = new QNetworkAccessManager();
    // requires qt 5.9
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    naMgr->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
#endif
    ncMgr = new QNetworkConfigurationManager();

    // test for required applications AFTER toolbars are created
    // so we can disable qactions if necessary
    CU utils;
    QStringList nf = hasApps(utils.reqApps);
    if (nf.size() > 0) {
        if (nf.contains("collaboriso_cli")) {
            QDir buildDir = QDir::current();
            buildDir.cdUp();
            QStringList cliPaths;
            cliPaths << tr("%1%2").arg(buildDir.absolutePath()).arg("/cli")
                     << QDir::currentPath();

            QStringList cliFound = hasApps(QStringList() << "collaboriso_cli", cliPaths, true);

            if (cliFound.size() > 0) {
                nf.removeAt(nf.indexOf(QRegExp("collaboriso_cli")));
                cliPath = cliFound[0];
            }
        }

        bool kdeNF = nf.contains("kdesudo");
        bool gkNF = nf.contains("gksudo");
        if (kdeNF && gkNF) {
            qDebug() << "You must have either gksu or kdesudo installed.";
        } else if (kdeNF) {
            // kdesudo not found, gksudo is available
            nf.removeAt(nf.indexOf(QRegExp("kdesudo")));
            suApp = "gksudo";
        } else if (gkNF) {
            // gksudo not found, kdesudo is available
            nf.removeAt(nf.indexOf(QRegExp("gksudo")));
        }

        if (nf.size() > 0) {
            qDebug() << "These apps cannot be found: " << nf;
            // disable usb generator button if any apps are missing
            usbAction->setDisabled(1);
            // disable device selector if lsblk is missing
            if (nf.contains("lsblk")) selAction->setDisabled(1);
        }
    }

    // connect events AFTER dialogs are created
    connectEvents();
}

Collaboriso::~Collaboriso()
{

}

void Collaboriso::generateUSB()
{
    QString devPath = tree->rootItem->text(1);
    if (devPath.isEmpty()) {
        qDebug() << "You must select a usb device in order to continue.";
        devDlg->show();
        return;
    }

    if (tree->rootItem->childCount() < 1) {
        qDebug() << "You must add ISOs or a menu to the tree in order to continue.";
        return;
    }

    verifyISOFiles();

    /*QString uname = qgetenv("USER");
    if (uname.isEmpty()) {
        uname = qgetenv("USERNAME");
    }*/

    QString suArgs;
    if (suApp == "kdesudo") {
        suArgs = tr("-d --title \"Collaboriso\" --comment \"Many of the utilities required by collaboriso must be run as root.\" -c");
    } else {
        suArgs = tr("-m \"Many of the utilities required by collaboriso must be run as root.\"");
    }

    /*proDlg->setProgressMsg1(tr("Generating usb..."));
    proDlg->setProgressRange(0, 4);
    proDlg->setProgressMsg2(tr(""));
    proDlg->setProgressPercent(0, 100);
    proDlg->show();*/

    QMessageBox::information(this, tr("ALERT!"),
                                   tr("The progress dialog is not currently functional for the usb creation process. Please be patient while the usb is being generated. Depending on how many isos you have selected it may take quite a bit of time. If you are running this application from the console then you'll be able to view error messages and the iso iteration. A popup will be displayed if everything is successful. Prepare your panic room and press OK when ready."),
                                   QMessageBox::Ok);

    QProcess collabCLI;
    collabCLI.start(tr("%1 %2 \"%3 -t %4 -f %5\"").arg(suApp).arg(suArgs).arg(cliPath).arg(devPath).arg(setDlg->fsType->currentText()));
    if (!collabCLI.waitForStarted() || !collabCLI.waitForFinished()) return;

    int btn;
    QString output = collabCLI.readAll();
    if (!output.isEmpty()) {
        qDebug() << output;
        //proDlg->hide();
        btn = QMessageBox::information(this, tr("ALERT!"),
                                       tr("Collaboriso requires a number of 3rd party utilities to perform the usb generation functions. It can be difficult to detect errors since the output must be searched for specific strings. This is a pre-alpha release and the entire generateUSB function will eventually be replaced with cross-platform libraries so it's up to you to decide whether to continue. If you see any errors in your console please click Cancel now. The 'lost/has new interfaces' messages are not considered errors."),
                                       QMessageBox::Ok | QMessageBox::Cancel);
        if (btn == QMessageBox::Cancel) return;
    }

    getDeviceInfo();

    // test for valid mountpoint
    QString mount = tree->rootItem->data(0, CU::MountRole).toString();
    if (mount.isEmpty()) {
        qDebug() << "The device is not mounted.";
        //proDlg->hide();
        return;
    }

    // test for valid mountpoint
    QDir mDir(mount);
    if (!mDir.exists() || !mDir.isReadable()) {
        qDebug() << "The mount point is invalid. Check that the folder exists and has the correct permissions.";
        //proDlg->hide();
        return;
    }

    CU utils;
    QDir isoDir;
    QProcess z7;
    QFileInfo fileInfo;
    QString isoFileNameClean, isoDestPath;
    QString rootDestDir = tr("%1%2").arg(mount).arg(utils.isoRoot);

    // retrieve only iso items
    QList<QTreeWidgetItem*> isos = tree->findItems(".iso", Qt::MatchEndsWith | Qt::MatchRecursive, 1);

    //proDlg->setProgressMsg1(tr("Processing isos..."));
    //proDlg->setProgressRange(1, 4);

    qDebug() << "Entering iso loop...";

    for (int i = 0, total = isos.size(); i < total; ++i) {
        qDebug() << tr("processing %1 of %2 isos").arg(QString::number(i+1)).arg(QString::number(total));

        /*if (!proDlg->isVisible()) {
            return;
        }*/

        //proDlg->setProgressMsg2(tr("%1 iso processed of %2").arg(QString::number(i)).arg(QString::number(total)));
        //proDlg->setProgressPercent(i, total);

        fileInfo.setFile(isos[i]->text(1));
        isoFileNameClean = fileInfo.fileName().replace(QRegExp("\\W"), "_");
        isoDestPath = tr("%1%2%3").arg(rootDestDir).arg(QDir::separator()).arg(isoFileNameClean);
        isoDir.setPath(isoDestPath);
        if (!isoDir.exists()) {
            if (!isoDir.mkpath(isoDestPath)) {
                qDebug() << isoDestPath << "The folder could not be created.";
                return;
            }
        } else {
            // prompt user if iso folder exists
            btn = QMessageBox::warning(this, tr("PANIC!"),
                                           tr("%1\n\nThe folder already exists! Click OK to overwrite.").arg(isoDestPath),
                                           QMessageBox::Ok, QMessageBox::Cancel);
            // we continue here instead of returning since the user is aware
            // of the issue and may not want to cancel the entire operation.
            if (btn == QMessageBox::Cancel) continue;

            if (!isoDir.removeRecursively()) {
                qDebug() << isoDestPath << "An error occurred while attempting to clear the folder.";
                return;
            }
            if (!isoDir.mkpath(isoDestPath)) {
                qDebug() << isoDestPath << "The folder could not be created.";
                return;
            }
        }
        z7.start(tr("7z x -o%1 %2").arg(isoDestPath).arg(isos[i]->text(1)));
        if (!z7.waitForStarted() || !z7.waitForFinished()) return;
    }

    // save grub to mount point
    /*proDlg->setProgressMsg1(tr("Saving grub menus..."));
    proDlg->setProgressRange(2, 4);
    proDlg->setProgressMsg2(tr(""));
    proDlg->setProgressPercent(0, 100);*/
    saveGrub(tr("%1%2").arg(mount).arg(utils.grubRoot));

    // save xml to mount point
    //proDlg->setProgressMsg1(tr("Backing up xml tree..."));
    //proDlg->setProgressRange(3, 4);
    saveXML(tr("%1%2%3").arg(mount).arg(QDir::separator()).arg(".collaboriso.xml"));

    //proDlg->hide();

    QMessageBox::information(this, tr("SUCCESS!"),
                                   tr("Your multiboot USB is ready!"),
                                   QMessageBox::Ok);
}

void Collaboriso::getDeviceInfo()
{
    QString devPath = tree->rootItem->text(1);
    if (devPath.isEmpty()) return;

    QProcess lsblk;
    lsblk.start(tr("lsblk -Jpno NAME,UUID,MOUNTPOINT %1").arg(devPath));
    if (!lsblk.waitForStarted() || !lsblk.waitForFinished()) return;

    QByteArray jsonRaw = lsblk.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonRaw);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) return;

    QJsonObject jsonRootObj = jsonDoc.object();
    QJsonArray devices = jsonRootObj["blockdevices"].toArray();
    if (devices.size() < 1) return;

    QJsonValue jsonVal = devices.at(0);
    QJsonObject dev = jsonVal.toObject();
    QJsonArray devChildren = dev["children"].toArray();
    if (devChildren.size() < 1) return;

    jsonVal = devChildren.at(0);
    QJsonObject partition = jsonVal.toObject();
    QString devUUID = partition["uuid"].toString();
    QString devMountPoint = partition["mountpoint"].toString();

    tree->rootItem->setData(0, CU::UUIDRole, devUUID);
    tree->rootItem->setData(0, CU::MountRole, devMountPoint);
}

QString Collaboriso::getLatestTimestamp()
{
    CU utils;
    if (!utils.prepareDB()) return "";

    QSqlDatabase sqldb = QSqlDatabase::database("ISODB");
    sqldb.setDatabaseName(utils.collabDB);
    if (!sqldb.open()) return "";

    QSqlQuery sq(sqldb);

    QString query = "SELECT modified FROM isos ORDER BY modified DESC LIMIT 1";

    bool success = sq.exec(query);
    if (!success) {
        qDebug() << sq.lastError().text();
    }

    // counting the results also positions the query on the first
    // record. an error will be thrown if we try to grab the value
    // before selecting the first record.
    if (countQueryResults(&sq) < 1) {
        success = false;
        qDebug() << "No timestamps found in database.";
    }

    sqldb.commit();
    sqldb.close();

    return (!success) ? "null" : sq.value(0).toString();
}

void Collaboriso::update()
{
    if (!ncMgr->isOnline() || naMgr->networkAccessible() != QNetworkAccessManager::Accessible) {
        qDebug() << "The network cannot be accessed.";
        return;
    }

    proDlg->setProgressMsg1(tr("Downloading updates..."));
    proDlg->setProgressRange(0, 2);
    proDlg->setProgressMsg2(tr("%1 bytes received of %2").arg(tr("0")).arg(tr("0")));
    proDlg->setProgressPercent(0, 100);
    proDlg->show();

    CU utils;
    QString urlStr = utils.updateURL;
    if (urlStr.contains("{{TIMESTAMP}}")) {
        QString tstamp = getLatestTimestamp();
        urlStr.replace("{{TIMESTAMP}}", tstamp);
        qDebug() << urlStr;
    }

    QUrl url(urlStr);
    download(url);
}

void Collaboriso::download(QUrl url)
{
    CU utils;
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", utils.userAgent.toLocal8Bit());
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    request.setMaximumRedirectsAllowed(3);

    // cancel any existing download
    if (netReply) {
        netReply->disconnect(this);
        netReply->deleteLater();
        netReply = 0;
    }

    netReply = naMgr->get(request);
    connect(netReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
    connect(netReply, SIGNAL(redirected(QUrl)), this, SLOT(downloadRedirected(QUrl)));
    connect(netReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
#ifndef QT_NO_SSL
    connect(netReply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(downloadSslErrors(QList<QSslError>)));
#endif
}

void Collaboriso::downloadRedirected(const QUrl &url)
{
    if (!url.url().isEmpty() && url.isValid()) {
        qDebug() << tr("Redirecting -> %1").arg(url.url());
    }
}

void Collaboriso::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (!proDlg->isVisible()) {
        // update was canceled so abort connection
        if (netReply) netReply->abort();
        return;
    }

    // bytesTotal will be -1 if download size cannot be determined.
    // this will occur on redirects.
    qint64 total = (bytesTotal > -1) ? bytesTotal : bytesReceived;
    proDlg->setProgressMsg2(tr("%1 bytes received of %2").arg(QString::number(bytesReceived)).arg(QString::number(total)));
    proDlg->setProgressPercent(bytesReceived, total);
}

void Collaboriso::downloadError(QNetworkReply::NetworkError code)
{
    qDebug() << tr("An error has occurred during the download.") << code;
    if (netReply) netReply->abort();
}

void Collaboriso::downloadSslErrors(const QList<QSslError> &sslErrors)
{
#ifndef QT_NO_SSL
    foreach (const QSslError &error, sslErrors) {
        qDebug() << tr("SSL error: %1").arg(error.errorString());
    }
#else
    Q_UNUSED(sslErrors);
#endif
    if (netReply) netReply->abort();
}

void Collaboriso::downloadFinished(QNetworkReply *reply)
{
    if (reply->error()) {
        qDebug() << reply->errorString();
    } else {
        proDlg->setProgressMsg1(tr("Installing updates..."));
        proDlg->setProgressRange(1, 2);

        // create json object from reply data
        QJsonObject iso;
        QJsonValue jsonVal;
        QString query;

        QByteArray jsonRaw = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonRaw);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) return;

        QJsonObject jsonRootObj = jsonDoc.object();
        QJsonArray isos = jsonRootObj["isos"].toArray();
        int total = isos.size();

        CU utils;
        if (!utils.prepareDB()) return;

        QSqlDatabase sqldb = QSqlDatabase::database("ISODB");
        sqldb.setDatabaseName(utils.collabDB);
        if (!sqldb.open()) return;
        QSqlQuery sq(sqldb);

        for (int i = 0; i < total; ++i) {
            if (!proDlg->isVisible()) {
                sqldb.commit();
                sqldb.close();
                return;
            }

            proDlg->setProgressMsg2(tr("%1 updates installed of %2").arg(QString::number(i)).arg(QString::number(total)));
            proDlg->setProgressPercent(i, total);

            jsonVal = isos.at(i);
            iso = jsonVal.toObject();
            query = tr("INSERT INTO isos VALUES(null, %1, %2, '%3', '%4', '%5', '%6', '%7', '%8')").
                    arg(iso["bootType"].toInt()).
                    arg(iso["strictHash"].toInt()).
                    arg(iso["title"].toString()).
                    arg(iso["hash"].toString()).
                    arg(iso["category"].toString()).
                    arg(iso["bootOptions"].toString()).
                    arg(iso["created"].toString()).
                    arg(iso["modified"].toString());

            if (!sq.exec(query)) {
                qDebug() << sq.lastError().text();
                // an error will be thrown if the entry already exists in db since
                // we use a unique key on the hash field. alternative is to use
                // REPLACE INTO instead of INSERT but that will modify id field
                // and it's possible that user manually edited the bootOptions for
                // an entry and that's why it already exists in db.
                // since this isn't a critical error we just print the error and
                // move on to next iteration.
                continue;
            }
        }
        sqldb.commit();
        sqldb.close();
    }

    // cancel any existing download
    if (netReply) {
        netReply->disconnect(this);
        netReply->deleteLater();
        netReply = 0;
    }

    proDlg->hide();
}

// returns list of apps not found OR list of found paths
QStringList Collaboriso::hasApps(QStringList apps, QStringList pathsToSearch, bool returnFound)
{
    QString path;
    QStringList found;
    QStringList notFound;
    for (int i = 0, total = apps.size(); i < total; ++i) {
        if (pathsToSearch.size() > 0) {
            path = QStandardPaths::findExecutable(apps[i], pathsToSearch);
        } else {
            path = QStandardPaths::findExecutable(apps[i]);
        }
        if (path.isEmpty()) {
            notFound << apps[i];
        } else {
            found << path;
        }
    }
    return (returnFound) ? found : notFound;
}

void Collaboriso::connectEvents()
{
    connect(devDlg->usbTable, SIGNAL(itemClicked(QTableWidgetItem*)), this, SLOT(setRootItem(QTableWidgetItem*)));
    connect(booDlg->isoTable, SIGNAL(itemClicked(QTableWidgetItem*)), this, SLOT(bootISOAs(QTableWidgetItem*)));
    connect(setDlg->selectBhvr, SIGNAL(currentIndexChanged(int)), this, SLOT(setSelectionBehavior(int)));
    connect(tree, SIGNAL(itemSelectionChanged()), this, SLOT(toggleButtonAccess()));
    connect(selAction, SIGNAL(triggered()), devDlg, SLOT(show()));
    connect(verAction, SIGNAL(triggered()), this, SLOT(verifyISOFiles()));
    connect(treeAction, SIGNAL(triggered()), this, SLOT(saveXML()));
    connect(grubAction, SIGNAL(triggered()), this, SLOT(saveGrub()));
    connect(addMenuAction, SIGNAL(triggered()), this, SLOT(addMenu()));
    connect(addISOAction, SIGNAL(triggered()), this, SLOT(addISO()));
    connect(removeAction, SIGNAL(triggered()), this, SLOT(removeSelected()));
    connect(removeAllAction, SIGNAL(triggered()), tree, SLOT(removeAll()));
    connect(bootAction, SIGNAL(triggered()), booDlg, SLOT(show()));
    connect(setAction, SIGNAL(triggered()), setDlg, SLOT(show()));
    connect(abtAction, SIGNAL(triggered()), abtDlg, SLOT(show()));
    connect(sortAZAction, SIGNAL(triggered()), this, SLOT(sortAsc()));
    //connect(sortZAAction, SIGNAL(triggered()), this, SLOT(sortDesc()));
    connect(moveUpAction, SIGNAL(triggered()), this, SLOT(moveOneUp()));
    connect(moveDownAction, SIGNAL(triggered()), this, SLOT(moveOneDown()));
    connect(updAction, SIGNAL(triggered()), this, SLOT(update()));
    connect(usbAction, SIGNAL(triggered()), this, SLOT(generateUSB()));
    connect(naMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
}

void Collaboriso::setSelectionBehavior(int index)
{
    tree->isExplicit = (index == 1); // -1 = nothing is selected, 0 = implicit
}

void Collaboriso::sort(QTreeWidgetItem *twi, Qt::SortOrder sorder)
{
    twi->sortChildren(0, sorder);
    if (!setDlg->recursiveSort->isChecked()) return;

    for (int imode, i = 0, total = twi->childCount(); i < total; ++i) {
        imode = twi->child(i)->data(0, CU::ItemRole).toInt();
        if (imode == CU::Menu) sort(twi->child(i), sorder);
    }
}

void Collaboriso::presort(Qt::SortOrder sorder)
{
    QList<QTreeWidgetItem*> items = tree->selectedItems();
    int total = items.size();

    if (total < 1) {
        sort(tree->rootItem, sorder);
        return;
    }

    for (int imode, i = 0; i < total; ++i) {
        imode = items[i]->data(0, CU::ItemRole).toInt();
        if (imode == CU::Menu) sort(items[i], sorder);
    }
}

void Collaboriso::sortAsc()
{
    presort(Qt::AscendingOrder);
}

void Collaboriso::sortDesc()
{
    presort(Qt::DescendingOrder);
}

void Collaboriso::moveOne(bool moveUp)
{
    QList<QTreeWidgetItem*> items = tree->selectedItems();
    int total = items.size();
    if (total != 1) return;

    int indexSrc = items[0]->parent()->indexOfChild(items[0]);
    int indexDest = (moveUp) ? indexSrc - 1 : indexSrc + 2;

    if (indexDest >= 0 && indexDest <= items[0]->parent()->childCount()) {
        QTreeWidgetItem *twiClone = items[0]->clone();
        items[0]->parent()->insertChild(indexDest, twiClone);
        items[0]->parent()->removeChild(items[0]);
        twiClone->setSelected(1);
    }
}

void Collaboriso::moveOneUp()
{
    moveOne(true);
}

void Collaboriso::moveOneDown()
{
    moveOne(false);
}

void Collaboriso::toggleButtonAccess()
{
    QString hash;
    int isoMode = 0;
    int itemMode = 0;

    QList<QTreeWidgetItem*> items = tree->selectedItems();
    int total = items.size();

    if (total > 0) {
        hash = items[0]->text(2);
        isoMode = items[0]->data(0, CU::IsoRole).toInt();
        itemMode = items[0]->data(0, CU::ItemRole).toInt();
    }

    bootAction->setDisabled((total != 1 || itemMode != CU::Iso || hash.isEmpty() || isoMode == CU::Supported));
    removeAction->setDisabled((total < 1));
    moveUpAction->setDisabled((total != 1));
    moveDownAction->setDisabled((total != 1));
}

void Collaboriso::createToolBars()
{
    selAction = new QAction(QIcon(":/images/select48.png"), "");
    selAction->setStatusTip(tr("Select usb device"));

    verAction = new QAction(QIcon(":/images/verify48.png"), "");
    verAction->setStatusTip(tr("Generate and verify hashes"));

    treeAction = new QAction(QIcon(":/images/tree48.png"), "");
    treeAction->setStatusTip(tr("Save XML tree"));

    grubAction = new QAction(QIcon(":/images/grub48.png"), "");
    grubAction->setStatusTip(tr("Save grub.cfg"));

    usbAction = new QAction(QIcon(":/images/usb48.png"), "");
    usbAction->setStatusTip(tr("Generate usb"));

    updAction = new QAction(QIcon(":/images/update48.png"), "");
    updAction->setStatusTip(tr("Update iso support"));

    setAction = new QAction(QIcon(":/images/settings48.png"), "");
    setAction->setStatusTip(tr("Settings"));

    abtAction = new QAction(QIcon(":/images/about48.png"), "");
    abtAction->setStatusTip(tr("About"));

    QWidget *spacer1 = new QWidget();
    spacer1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    topToolBar = new QToolBar(tr("USB Operations"));
    topToolBar->setIconSize(QSize(40,40));
    topToolBar->addAction(selAction);
    topToolBar->addAction(verAction);
    topToolBar->addAction(treeAction);
    topToolBar->addAction(grubAction);
    topToolBar->addAction(usbAction);
    topToolBar->addWidget(spacer1);
    topToolBar->addAction(updAction);
    topToolBar->addAction(setAction);
    topToolBar->addAction(abtAction);
    topToolBar->setMovable(0);
    topToolBar->setFloatable(0);
    addToolBar(Qt::TopToolBarArea, topToolBar);

    addMenuAction = new QAction(QIcon(":/images/addmenu48.png"), "");
    addMenuAction->setStatusTip(tr("Add menu"));

    addISOAction = new QAction(QIcon(":/images/addiso48.png"), "");
    addISOAction->setStatusTip(tr("Add ISO"));

    bootAction = new QAction(QIcon(":/images/boot48.png"), "");
    bootAction->setStatusTip(tr("Boot as..."));
    bootAction->setDisabled(1);

    moveUpAction = new QAction(QIcon(":/images/up48.png"), "");
    moveUpAction->setStatusTip(tr("Move up"));
    moveUpAction->setDisabled(1);

    moveDownAction = new QAction(QIcon(":/images/down48.png"), "");
    moveDownAction->setStatusTip(tr("Move down"));
    moveDownAction->setDisabled(1);

    removeAction = new QAction(QIcon(":/images/remove48.png"), "");
    removeAction->setStatusTip(tr("Remove"));
    removeAction->setDisabled(1);

    sortAZAction = new QAction(QIcon(":/images/sortaz48.png"), "");
    sortAZAction->setStatusTip(tr("Sort A->Z"));
    //sortAZAction->setDisabled(1);

    //sortZAAction = new QAction(QIcon(":/images/sortza48.png"), "");
    //sortZAAction->setStatusTip(tr("Sort Z->A"));
    //sortZAAction->setDisabled(1);

    removeAllAction = new QAction(QIcon(":/images/removeall48.png"), "");
    removeAllAction->setStatusTip(tr("Remove all"));
    //removeAllAction->setDisabled(1);

    QWidget *spacer2 = new QWidget();
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    botToolBar = new QToolBar(tr("Tree Operations"));
    botToolBar->setIconSize(QSize(24,24));
    botToolBar->addAction(addMenuAction);
    botToolBar->addAction(addISOAction);
    botToolBar->addAction(bootAction);
    botToolBar->addAction(moveUpAction);
    botToolBar->addAction(moveDownAction);
    botToolBar->addAction(removeAction);
    botToolBar->addWidget(spacer2);
    botToolBar->addAction(sortAZAction);
    //botToolBar->addAction(sortZAAction);
    botToolBar->addAction(removeAllAction);
    botToolBar->setMovable(0);
    botToolBar->setFloatable(0);
    addToolBar(Qt::BottomToolBarArea, botToolBar);
}

void Collaboriso::createStatusBar()
{
    statusBar = new QStatusBar();
    statusBar->setSizeGripEnabled(0);
    setStatusBar(statusBar);
}

void Collaboriso::createTreeWidget()
{
    tree = new TreeWidget();
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    //tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    //tree->setSelectionModel(QItemSelectionModel::Rows);
    tree->setColumnCount(3);
    tree->setHeaderHidden(1);
    tree->setDragEnabled(1);
    tree->setAcceptDrops(1);
    tree->setDropIndicatorShown(1);
    tree->setDragDropMode(QAbstractItemView::InternalMove);
    tree->hideColumn(1);
    tree->hideColumn(2);
    tree->setMouseTracking(1);
    tree->viewport()->setAcceptDrops(1);

    tree->rootItem = new QTreeWidgetItem(tree, QStringList() << "/device" << "" << "");
    tree->rootItem->setFlags(Qt::ItemIsEnabled);
    tree->rootItem->setTextAlignment(0, Qt::AlignVCenter | Qt::AlignLeft);
    tree->rootItem->setStatusTip(0, "/device");
    tree->rootItem->setExpanded(1);
    tree->rootItem->setData(0, CU::ItemRole, CU::Root);
    tree->addTopLevelItem(tree->rootItem);

    QVBoxLayout *vbox = new QVBoxLayout();
    vbox->addWidget(tree, 1);

    QWidget *mainWidget = new QWidget();
    mainWidget->setLayout(vbox);
    setCentralWidget(mainWidget);
}

void Collaboriso::setRootItem(QTableWidgetItem *twi)
{
    tree->rootItem->setText(0, twi->text());
    tree->rootItem->setText(1, twi->data(CU::PathRole).toString());
    tree->rootItem->setData(0, CU::UUIDRole, twi->data(CU::UUIDRole).toString());
    tree->rootItem->setData(0, CU::MountRole, twi->data(CU::MountRole).toString());
    tree->rootItem->setStatusTip(0, twi->text());
    devDlg->hide();
}

int Collaboriso::countMenus()
{
    // retrieve only menu items
    QList<QTreeWidgetItem*> list = tree->findItems("", Qt::MatchFixedString | Qt::MatchRecursive, 1);
    int total = list.size();
    if (total > 0 && list[0] == tree->rootItem) total--;
    return total;
}

void Collaboriso::verifyISOFiles()
{
    // retrieve all items
    //QList<QTreeWidgetItem*> list = tree->findItems("*", Qt::MatchWildcard | Qt::MatchRecursive);

    // retrieve only menu items
    //QList<QTreeWidgetItem*> list = tree->findItems("", Qt::MatchFixedString | Qt::MatchRecursive, 1);

    // retrieve only iso items
    QList<QTreeWidgetItem*> list = tree->findItems(".iso", Qt::MatchEndsWith | Qt::MatchRecursive, 1);
    if (list.size() < 1) return;

    CU::IsoMode imode;
    QString path, hash, indexStr, query, twiTitle, twiCat, twiIcon;
    int totalInt = list.size();
    QString totalStr = QString::number(totalInt, 10);
    QFileInfo fileInfo;

    proDlg->setProgressMsg1(tr("Generating hash %1 of %2").arg(tr("0")).arg(tr("0")));
    proDlg->setProgressRange(0, totalInt);
    proDlg->setProgressMsg2(tr("Processing..."));
    proDlg->setProgressPercent(0, 100);
    proDlg->show();

    CU utils;
    if (!utils.prepareDB()) return;

    QSqlDatabase sqldb = QSqlDatabase::database("ISODB");
    sqldb.setDatabaseName(utils.collabDB);
    if (!sqldb.open()) return;
    QSqlQuery sq(sqldb);

    for (int i = 0; i < totalInt; ++i) {
        if (!proDlg->isVisible()) {
            qDebug() << "cancel event detected";
            sqldb.commit();
            sqldb.close();
            return;
        }

        indexStr = QString::number(i+1, 10);

        path = list[i]->text(1);
        hash = list[i]->text(2);
        // we only hash isos and we don't re-hash. if hash exists skip it.
        if (path.right(4) != ".iso" || !hash.isEmpty()) continue;

        fileInfo.setFile(path);

        proDlg->setProgressMsg1(tr("Generating hash %1 of %2").arg(indexStr).arg(totalStr));
        proDlg->setProgressMsg2(tr("Processing %1...").arg(fileInfo.fileName()));
        proDlg->setProgressRange(i, totalInt);

        hash = getISOHash(path);
        if (hash.isEmpty()) continue;

        list[i]->setText(2, hash);
        list[i]->setStatusTip(0, hash);

        // search sqlite for hash and retrieve title and category
        query = "SELECT title, category FROM isos WHERE hash = '" + hash + "'";
        if (!sq.exec(query)) {
            qDebug() << sq.lastError().text();
            continue;
        }

        if (countQueryResults(&sq) > 0) {
            imode = CU::Supported;
            twiTitle = sq.value(0).toString();
            twiCat = sq.value(1).toString();
            twiIcon = "supported48.png";
        } else {
            imode = CU::Unsupported;
            twiTitle = fileInfo.fileName();
            twiCat = "Unsupported";
            twiIcon = "unsupported48.png";
        }

        updateTreeItem(list[i], twiTitle, twiCat, hash, twiIcon, imode);
    }

    sqldb.commit();
    sqldb.close();

    proDlg->hide();
}

void Collaboriso::updateTreeItem(QTreeWidgetItem* twi, QString title, QString category, QString hash, QString icon, CU::IsoMode imode)
{
    twi->setText(0, title);
    twi->setText(2, hash);
    twi->setStatusTip(0, hash);
    twi->setData(0, CU::IsoRole, imode);
    twi->setIcon(0, QIcon(":/images/" + icon));

    QTreeWidgetItem* itemToMove = twi->clone();
    twi->parent()->removeChild(twi);

    // if category is found it will be returned. otherwise a new
    // category will be created and returned.
    QTreeWidgetItem *catItem = tree->addMenu(category);

    // move item to child of new catItem
    catItem->addChild(itemToMove);
}

QString Collaboriso::getISOHash(QString path)
{
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        qDebug() << path << "does not exist or cannot be read";
        return "";
    }

    QCryptographicHash *hash = new QCryptographicHash(QCryptographicHash::Sha1);

    int bytesRead;
    int maxLength = 32768;
    qint64 totalBytesRead = 0;
    char buffer[maxLength];
    qint64 fileSize = file.size();

    QDataStream in(&file);

    while (!in.atEnd()) {
        if (!proDlg->isVisible()) {
            hash->reset();
            qDebug() << "cancel event detected";
            return "";
        }

        bytesRead = in.readRawData(buffer, maxLength);
        hash->addData(buffer, bytesRead);
        totalBytesRead += bytesRead;
        proDlg->setProgressPercent(totalBytesRead, fileSize);
        qApp->processEvents();
    }

    QString hashResult = hash->result().toHex();

    hash->reset();
    file.close();
    file.deleteLater();

    return hashResult;
}

int Collaboriso::countQueryResults(QSqlQuery *query)
{
    // query.size() does not report the correct size on Linux so
    // we just loop through the entries and count them manually
    int total = 0;
    while (query->next()) {
        total++;
    }
    // reset the selected record otherwise it will be positioned on
    // an invalid entry after manually counting the results
    query->first();
    return total;
}

QString Collaboriso::prepareQuery(QString str)
{
    // single quotes are the only characters that need to be
    // escaped for sqlite
    return str.replace("'", "''");
}

bool Collaboriso::saveXML(QString outputPath)
{
    if (outputPath.isEmpty()) {
        outputPath = QFileDialog::getSaveFileName(this, tr("Save XML Tree"),
                                                    QDir::homePath() + QDir::separator() + "Desktop",
                                                    tr("XML (*.xml)"));
        if (outputPath.isEmpty()) return 0;
    }

    QFile file(outputPath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qDebug() << file.errorString();
        return 0;
    }

    QXmlStreamWriter xml;
    xml.setAutoFormatting(1);
    xml.setDevice(&file);
    xml.writeStartDocument();
    //xml.writeDTD("<!DOCTYPE collaboriso>");
    xml.writeStartElement("collaboriso");
    xml.writeAttribute("version", "1.0");

    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        writeXMLItem(&xml, tree->topLevelItem(i));
    }

    xml.writeEndDocument();

    return 1;
}

void Collaboriso::writeXMLItem(QXmlStreamWriter *xml, QTreeWidgetItem *item)
{
    QString tag, label, path, hash, isoModeStr;
    int itemMode;
    label = item->text(0);
    path = item->text(1);
    hash = item->text(2);
    isoModeStr = item->data(0, CU::IsoRole).toString();
    itemMode = item->data(0, CU::ItemRole).toInt();

    if (itemMode == CU::Root) {
        tag = "device";
    } else if (itemMode == CU::Menu) {
        tag = "menu";
    } else {
        tag = "iso";
    }

    xml->writeStartElement(tag);
    if (!label.isEmpty()) xml->writeAttribute("label", label);
    if (!path.isEmpty()) xml->writeAttribute("path", path);
    if (!hash.isEmpty()) xml->writeAttribute("hash", hash);
    if (!isoModeStr.isEmpty()) xml->writeAttribute("mode", isoModeStr);

    for (int i = 0; i < item->childCount(); ++i) {
        writeXMLItem(xml, item->child(i));
    }

    xml->writeEndElement();
}

bool Collaboriso::saveGrub(QString outputDir)
{
    if (outputDir.isEmpty()) {
        outputDir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"),
                                                              QDir::homePath() + QDir::separator() + "Desktop",
                                                              QFileDialog::ShowDirsOnly |
                                                              QFileDialog::DontResolveSymlinks);
        if (outputDir.isEmpty()) return 0;
    }

    QString grubFile = tr("%1%2%3").arg(outputDir).arg(QDir::separator()).arg("grub.cfg");
    QString menusDir = tr("%1%2%3").arg(outputDir).arg(QDir::separator()).arg("menus");

    int menuTotal = countMenus();

    if (menuTotal > 0) {
        QDir mdir(menusDir);
        if (!mdir.exists()) {
            if (!mdir.mkpath(menusDir)) {
                qDebug() << "The menus directory could not be created.";
                return 0;
            }
        }
    }

    CU utils;
    if (!utils.prepareDB()) return 0;

    QSqlDatabase sqldb = QSqlDatabase::database("ISODB");
    sqldb.setDatabaseName(utils.collabDB);
    if (!sqldb.open()) return 0;
    QSqlQuery sq(sqldb);

    createGrubMenu(tree->rootItem, &sq, grubFile, menusDir);

    sqldb.commit();
    sqldb.close();

    return 1;
}

void Collaboriso::createGrubMenu(QTreeWidgetItem *item, QSqlQuery *sq, QString grubFile, QString menusDir)
{
    QFile file(grubFile);
    if (file.exists()) {
        int btn = QMessageBox::warning(this, tr("PANIC!"),
                                       tr("%1\n\nThe file already exists! Click OK to overwrite.").arg(grubFile),
                                       QMessageBox::Ok, QMessageBox::Cancel);
        if (btn == QMessageBox::Cancel) return;
    }

    QFile::OpenMode omode = QFile::WriteOnly | QFile::Text | QFile::Truncate;

    if (!file.open(omode)) {
        qDebug() << file.errorString();
        return;
    }

    // no error is thrown if UUIDRole doesn't exist. worst case
    // scenario it justs returns an empty string.
    QString devUUID = tree->rootItem->data(0, CU::UUIDRole).toString();

    CU utils;
    QTextStream out(&file);

    // if user only wants header on main cfg file then we test
    // whether grubFile is located in menusDir with contains()
    if (setDlg->grubHeadBhvr->currentIndex() == 2 ||
        (setDlg->grubHeadBhvr->currentIndex() == 1 && !grubFile.contains(menusDir))) {
        QFile grhd(utils.collabGrub);
        if (grhd.exists() && grhd.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString grhdTxt = grhd.readAll();
            out << "#BEGIN HEADER\n" << grhdTxt.replace("{{UUID}}", devUUID) << "\n#END HEADER\n\n";
            grhd.close();
            grhd.deleteLater();
        }
    }

    int itemMode;
    QFileInfo fileInfo;
    QString menuLabel, menuLabelClean, menuFileName, menuFilePath, hash, query, bootOptions, isoFileNameClean;

    for (int i = 0; i < item->childCount(); ++i) {
        bootOptions = "";
        menuLabel = menuLabelClean = item->child(i)->text(0);
        menuLabelClean.replace(QRegExp("\\W"), "_");
        menuFileName = tr("%1.cfg").arg(menuLabelClean);
        menuFilePath = menusDir + QDir::separator() + menuFileName;
        hash = item->child(i)->text(2);
        itemMode = item->child(i)->data(0, CU::ItemRole).toInt();

        if (itemMode == CU::Menu) {
            out << tr("menuentry \"%1\" {\n").arg(menuLabel)
                << tr("\tconfigfile %1%2%3\n").arg(utils.menuRoot).arg(QDir::separator()).arg(menuFileName)
                << tr("}\n");

            createGrubMenu(item->child(i), sq, menuFilePath, menusDir);
        } else {
            fileInfo.setFile(item->child(i)->text(1));
            isoFileNameClean = fileInfo.fileName().replace(QRegExp("\\W"), "_");

            query = "SELECT bootOptions FROM isos WHERE hash = '" + hash + "'";
            if (sq->exec(query) && countQueryResults(sq) > 0) {
                bootOptions = sq->value(0).toString().replace("{{UUID}}", devUUID);
                bootOptions.replace("{{ISOFOLDER}}", tr("%1%2%3").arg(utils.isoRoot).arg(QDir::separator()).arg(isoFileNameClean));
            } else {
                qDebug() << tr("The iso hash could not be found in the database.");
            }

            out << tr("menuentry \"%1\" {\n").arg(menuLabel)
                << tr("%1\n").arg(bootOptions)
                << tr("}\n");
        }
    }

    file.close();
    file.deleteLater();
}

void Collaboriso::addMenu()
{
    bool ok;
    QString mlabel = QInputDialog::getText(this, tr("Add Menu"),
                                         tr("Menu label:"), QLineEdit::Normal,
                                         tr("My New Menu"), &ok);
    if (!ok || mlabel.isEmpty()) return;

    tree->addMenu(mlabel);
}

void Collaboriso::addISO()
{
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select ISOs"),
                                                      QDir::homePath() + QDir::separator() + "Desktop",
                                                      "ISOs (*.iso)");
    int total = files.size();
    if (total < 1) return;

    for (int i = 0; i < total; ++i) {
        tree->addISO(files[i]);
    }
}

void Collaboriso::removeSelected()
{
    if (tree->isExplicit) tree->moveItems(CU::Unselected);

    tree->removeSelected();
}

void Collaboriso::bootISOAs(QTableWidgetItem* twi)
{
    QString title = twi->text();
    QString category = twi->data(CU::CategoryRole).toString();
    QString hash = twi->data(CU::HashRole).toString();
    QString icon = "caution48.png";

    QList<QTreeWidgetItem*> items = tree->selectedItems();

    // only one item can be selected at a time in order to
    // click the boot button. no need to test size.

    updateTreeItem(items[0], title, category, hash, icon, CU::Unknown);

    // remove unsupported menu if empty
    QList<QTreeWidgetItem*> list = tree->findItems("Unsupported", Qt::MatchRecursive);
    if (list.size() > 0 && list[0]->childCount() < 1) list[0]->parent()->removeChild(list[0]);

    booDlg->hide();
}

TreeWidget::TreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{

}

void TreeWidget::addISO(QString path)
{
    if (path.right(4) != ".iso") return; // only iso files supported

    // remove extra slashes
    path = QDir::cleanPath(path);
    /* replace slashes /->\ for display on windows */
    path = QDir::toNativeSeparators(path);
    QString fileName = QFileInfo(path).fileName();

    QList<QTreeWidgetItem*> list = this->findItems(fileName, Qt::MatchRecursive);
    if (list.size() > 0) return; // duplicate found so skip it

    QTreeWidgetItem *twi = new QTreeWidgetItem(this->rootItem, QStringList() << fileName << path << "");
    twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemNeverHasChildren);
    twi->setTextAlignment(0, Qt::AlignVCenter | Qt::AlignLeft);
    twi->setToolTip(0, path);
    twi->setData(0, CU::ItemRole, CU::Iso);
    twi->setData(0, CU::IsoRole, CU::Unsupported); // assume unsupported until verified

    this->addTopLevelItem(twi);
}

QTreeWidgetItem* TreeWidget::addMenu(QString category)
{    
    QList<QTreeWidgetItem*> list = this->findItems(category, Qt::MatchRecursive);
    if (list.size() > 0) return list[0]; // duplicate found

    QTreeWidgetItem *twi = new QTreeWidgetItem(this->rootItem, QStringList() << category << "" << "");
    twi->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    twi->setTextAlignment(0, Qt::AlignVCenter | Qt::AlignLeft);
    twi->setStatusTip(0, category);
    twi->setExpanded(1);
    twi->setIcon(0, QIcon(":/images/folder48.png"));
    twi->setData(0, CU::ItemRole, CU::Menu);

    this->addTopLevelItem(twi);
    return twi;
}

void TreeWidget::removeSelected()
{
    QList<QTreeWidgetItem*> items = this->selectedItems();
    for (int i = 0, total = items.size(); i < total; ++i) {
        items[i]->parent()->removeChild(items[i]);
    }
}

void TreeWidget::removeAll()
{
    while (this->rootItem->childCount() > 0) {
        this->rootItem->removeChild(this->rootItem->child(0));
    }
}

// 0 = move implicit selected, 1 = move explicit selected, 2 = move unselected
void TreeWidget::moveItems(CU::MoveMode mmode, QTreeWidgetItem *dest)
{
    // we can't move selected items without a destination
    if (mmode != CU::Unselected && !dest) return;

    int destIndex, selIndex, itemMode;
    QTreeWidgetItem *twiParent;
    QTreeWidgetItem *twiCloneSel = new QTreeWidgetItem();
    QTreeWidgetItem *twiCloneUnsel = new QTreeWidgetItem();
    QList<QTreeWidgetItem*> selItems = this->selectedItems();

    for (int i = 0, selTotal = selItems.size(); i < selTotal; ++i) {
        // we only clone highest level selection to avoid duplicates
        if (this->isParentSelected(selItems[i])) continue;

        itemMode = dest->data(0, CU::ItemRole).toInt();

        if (mmode == CU::Unselected) {
            // the branch will be built as unselected items are detected
            this->cloneBranch(twiCloneUnsel, selItems[i], 0);

            if (twiCloneUnsel->childCount() > 0) {
                // find an unselected parent and insert cloned branch
                twiParent = this->getUnselectedParent(selItems[i]);
                twiParent->addChildren(twiCloneUnsel->takeChildren());
            }
        } else if (mmode == CU::Selected) {
            // the branch will be built as selected items are detected
            this->cloneBranch(twiCloneSel, selItems[i], 1);

            // redundant. we know there will always be at least one child
            // because this loop can only be entered if there are selected
            // items.
            if (twiCloneSel->childCount() > 0) {
                if (itemMode == CU::Iso) {
                    // destination is iso. insert cloned branch before dest item.
                    destIndex = dest->parent()->indexOfChild(dest);
                    dest->parent()->insertChildren(destIndex, twiCloneSel->takeChildren());
                } else {
                    // destination is menu. insert cloned branch into dest item.
                    dest->addChildren(twiCloneSel->takeChildren());
                }
            }
        } else if (mmode == CU::Implicit) {
            selIndex = selItems[i]->parent()->indexOfChild(selItems[i]);
            if (itemMode == CU::Iso) {
                // destination is iso. insert selected branch before dest item.
                destIndex = dest->parent()->indexOfChild(dest);
                dest->parent()->insertChild(destIndex, selItems[i]->parent()->takeChild(selIndex));
            } else {
                // destination is menu. insert selected branch into dest item.
                dest->addChild(selItems[i]->parent()->takeChild(selIndex));
            }
        }
    }
}

bool TreeWidget::isParentSelected(QTreeWidgetItem* twi)
{
    QTreeWidgetItem *twiParent = twi->parent();
    while (twiParent != this->rootItem) {
        if (twiParent->isSelected()) return 1;
        twiParent = twiParent->parent();
    }
    return 0;
}

QTreeWidgetItem* TreeWidget::getUnselectedParent(QTreeWidgetItem* twi)
{
    // root can't be selected so it provides a guaranteed break from loop
    QTreeWidgetItem* twiParent = twi->parent();
    while (twiParent->isSelected()) {
        twiParent = twiParent->parent();
    }
    return twiParent;
}

QTreeWidgetItem* TreeWidget::copyItem(QTreeWidgetItem *twiParent, QTreeWidgetItem *twi)
{
    QTreeWidgetItem *twiCopy = new QTreeWidgetItem(twiParent, QStringList() << twi->text(0) << twi->text(1) << twi->text(2));
    twiCopy->setFlags(twi->flags());
    twiCopy->setTextAlignment(0, Qt::AlignVCenter | Qt::AlignLeft);
    twiCopy->setToolTip(0, twi->toolTip(0));
    twiCopy->setStatusTip(0, twi->statusTip(0));
    twiCopy->setData(0, CU::IsoRole, twi->data(0, CU::IsoRole));
    twiCopy->setData(0, CU::ItemRole, twi->data(0, CU::ItemRole));
    twiCopy->setIcon(0, twi->icon(0));
    twiCopy->setExpanded(1);

    return twiCopy;
}

void TreeWidget::cloneBranch(QTreeWidgetItem *twiClone, QTreeWidgetItem *twi, bool selected)
{
    QTreeWidgetItem *twiParent = twiClone;
    if (twi->isSelected() == selected) {
        twiParent = this->copyItem(twiClone, twi);
        twiClone->addChild(twiParent);
    }
    for (int i = 0; i < twi->childCount(); ++i) {
        this->cloneBranch(twiParent, twi->child(i), selected);
    }
}

void TreeWidget::dropEvent(QDropEvent *event)
{
    QModelIndex modelIndex = this->indexAt(event->pos());
    const QMimeData *mimeData = event->mimeData();
    event->setDropAction(Qt::IgnoreAction);

    if (!modelIndex.isValid() && !mimeData->hasUrls()) {
        event->ignore();
        return;
    }

    if (mimeData->hasUrls()) {
        QString path;
        QList<QUrl> urlList = mimeData->urls();
        for (int i = 0, total = urlList.size(); i < total; ++i) {
            path = urlList[i].toLocalFile();
            this->addISO(path);
        }
    } else {
        QTreeWidgetItem *dest = this->itemFromIndex(modelIndex);
        if (this->isExplicit) this->moveItems(CU::Unselected);
        this->moveItems((this->isExplicit) ? CU::Selected : CU::Implicit, dest);
        this->removeSelected();
    }

    //event->acceptProposedAction();
    event->accept();
}

void TreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void TreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void TreeWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}
