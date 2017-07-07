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

#include "boot.h"

BootSelector::BootSelector(QWidget *parent)
    : QDialog(parent)
{
    int winWidth = 300;
    int winHeight = 184;

    isoTable = new QTableWidget();
    isoTable->setColumnCount(1);
    isoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    isoTable->setSelectionMode(QAbstractItemView::SingleSelection);
    isoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    isoTable->verticalHeader()->hide();
    isoTable->horizontalHeader()->hide();
    isoTable->horizontalHeader()->setStretchLastSection(1);
    isoTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);

    QHBoxLayout *hboxLayout = new QHBoxLayout();
    hboxLayout->addWidget(isoTable, 1);

    QVBoxLayout *vboxLayout = new QVBoxLayout();
    vboxLayout->addLayout(hboxLayout, 1);
    setLayout(vboxLayout);

    setModal(1);
    setWindowModality(Qt::ApplicationModal);
    setWindowTitle("Boot As...");
    setWindowIcon(QIcon(":/images/boot48.png"));
    resize(winWidth, winHeight);
}

void BootSelector::getISOList()
{
    clearTable();

    int row;
    QString isoLabel, isoCat, isoHash, query;
    QTableWidgetItem *twi;

    CU utils;
    if (!utils.prepareDB()) return;

    QSqlDatabase sqldb = QSqlDatabase::database("ISODB");
    sqldb.setDatabaseName(utils.collabDB);
    if (!sqldb.open()) return;
    QSqlQuery sq(sqldb);

    // search sqlite for all entries with strictHash = 0
    query = "SELECT title, category, hash FROM isos WHERE strictHash = '0'";
    if (!sq.exec(query)) {
        qDebug() << sq.lastError().text();
        return;
    }

    while (sq.next()) {
        isoLabel = sq.value(0).toString();
        isoCat = sq.value(1).toString();
        isoHash = sq.value(2).toString();

        twi = new QTableWidgetItem(isoLabel);
        twi->setTextAlignment(Qt::AlignCenter);
        twi->setData(CU::CategoryRole, isoCat);
        twi->setData(CU::HashRole, isoHash);

        row = isoTable->rowCount();
        isoTable->insertRow(row);
        isoTable->setItem(row, 0, twi);
    }
}

void BootSelector::clearTable()
{
    int rows = isoTable->rowCount();
    for (int i = rows - 1; i >= 0; --i) {
        isoTable->removeRow(i);
    }
}

void BootSelector::showEvent(QShowEvent *event)
{
    getISOList();
    event->accept();
    //qApp->processEvents();
}

void BootSelector::hideEvent(QHideEvent *event)
{
    event->accept();
}
