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

#include "about.h"

About::About(QWidget *parent)
    : QDialog(parent)
{
    int winWidth = 500;
    int winHeight = 500;

    htmlLabel = new QLabel();
    htmlLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    htmlLabel->setOpenExternalLinks(true);
    htmlLabel->setWordWrap(false);

    QFile file(":/docs/about.htm");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString about = file.readAll();
        if (!about.isEmpty()) {
            htmlLabel->setText(about);
        }
    }
    file.close();
    file.deleteLater();

    QVBoxLayout *scrollLayout = new QVBoxLayout();
    scrollLayout->addWidget(htmlLabel, 0, Qt::AlignHCenter);
    scrollLayout->setSizeConstraint(QLayout::SetFixedSize);

    QWidget *scrollWidget = new QWidget();
    scrollWidget->setLayout(scrollLayout);
    scrollWidget->setMinimumWidth(460);
    scrollWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    scrollWidget->setFixedSize(460, 460);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setWidgetResizable(false);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setAlignment(Qt::AlignHCenter);
    scrollArea->setFixedSize(460, 460);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(scrollArea, 0, Qt::AlignHCenter);
    setLayout(mainLayout);

    setWindowTitle("About");
    setWindowIcon(QIcon(":/images/about48.png"));
    resize(winWidth, winHeight);
}

void About::showEvent(QShowEvent *event)
{
    event->accept();
}

void About::hideEvent(QHideEvent *event)
{
    event->accept();
}
