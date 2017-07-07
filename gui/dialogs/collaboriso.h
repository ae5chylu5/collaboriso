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

#ifndef COLLABORISO_H
#define COLLABORISO_H

#include <QAction>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QIODevice>
#include <QLayout>
#include <QMainWindow>
#include <QMessageBox>
#include <QMimeData>
#include <QModelIndex>
#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QXmlStreamWriter>
#include <QtEvents>

#include "device.h"
#include "boot.h"
#include "progress.h"
#include "settings.h"
#include "about.h"
#include "cutils.h"

class TreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    TreeWidget(QWidget *parent = 0);
    QTreeWidgetItem *rootItem;
    void addISO(QString path);
    QTreeWidgetItem* addMenu(QString category);
    void removeSelected();
    void moveItems(CU::MoveMode mmode, QTreeWidgetItem *dest = 0);
    bool isExplicit = 0;

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

private slots:
    void removeAll();

private:
    bool isParentSelected(QTreeWidgetItem* twi);
    QTreeWidgetItem* getUnselectedParent(QTreeWidgetItem* twi);
    void cloneBranch(QTreeWidgetItem *twiClone, QTreeWidgetItem *twi, bool selected);
    QTreeWidgetItem* copyItem(QTreeWidgetItem *twiParent, QTreeWidgetItem *twi);
};

class Collaboriso : public QMainWindow
{
    Q_OBJECT

public:
    Collaboriso(QWidget *parent = 0);
    ~Collaboriso();
    QString cliPath;
    QString suApp;

private:
    void createToolBars();
    void createStatusBar();
    void createTreeWidget();
    void connectEvents();
    int countMenus();
    QString getISOHash(QString path);
    QString prepareQuery(QString str);
    QStringList hasApps(QStringList apps, QStringList pathsToSearch = QStringList(), bool returnFound = false);
    int countQueryResults(QSqlQuery *query);
    void writeXMLItem(QXmlStreamWriter *xml, QTreeWidgetItem *item);
    void createGrubMenu(QTreeWidgetItem *item, QSqlQuery *sq, QString grubFile, QString menusDir);
    void updateTreeItem(QTreeWidgetItem *twi, QString title, QString category, QString hash, QString icon, CU::IsoMode imode);
    void sort(QTreeWidgetItem *twi, Qt::SortOrder sorder = Qt::AscendingOrder);
    void presort(Qt::SortOrder sorder = Qt::AscendingOrder);
    void moveOne(bool moveUp = true);
    void download(QUrl url);
    void getDeviceInfo();
    QString getLatestTimestamp();

    DeviceSelector *devDlg;
    BootSelector *booDlg;
    ProgressMeter *proDlg;
    Settings *setDlg;
    About *abtDlg;

    QNetworkReply *netReply;
    QNetworkAccessManager *naMgr;
    QNetworkConfigurationManager *ncMgr;

    TreeWidget *tree;

    QToolBar *topToolBar;
    QToolBar *botToolBar;
    QStatusBar *statusBar;

    QAction *selAction;
    QAction *verAction;
    QAction *treeAction;
    QAction *grubAction;
    QAction *usbAction;
    QAction *updAction;
    QAction *setAction;
    QAction *abtAction;

    QAction *addMenuAction;
    QAction *addISOAction;
    QAction *removeAction;
    QAction *removeAllAction;
    QAction *bootAction;
    QAction *sortAZAction;
    QAction *sortZAAction;
    QAction *moveUpAction;
    QAction *moveDownAction;

private slots:
    void setRootItem(QTableWidgetItem *twi);
    void verifyISOFiles();
    bool saveXML(QString outputPath = "");
    bool saveGrub(QString outputDir = "");
    void addMenu();
    void addISO();
    void removeSelected();
    void bootISOAs(QTableWidgetItem* twi);
    void toggleButtonAccess();
    void setSelectionBehavior(int index);
    void sortAsc();
    void sortDesc();
    void moveOneUp();
    void moveOneDown();
    void downloadFinished(QNetworkReply *reply);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void update();
    void downloadError(QNetworkReply::NetworkError code);
    void downloadRedirected(const QUrl &url);
    void downloadSslErrors(const QList<QSslError> &errors);
    void generateUSB();
};

#endif // COLLABORISO_H
