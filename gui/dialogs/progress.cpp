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

#include "progress.h"

ProgressMeter::ProgressMeter(QWidget *parent)
    : QDialog(parent)
{
	int winWidth = 350;
	int winHeight = 220;

    bar1RangeCurrent = 0;
    bar1RangeTotal = 0;

    progressLabel1 = new QLabel();
	progressLabel1->setAlignment(Qt::AlignCenter);
    progressLabel1->setWordWrap(1);
    progressLabel1->setStyleSheet("QLabel { font-size:11px; font-weight:400; text-align:center; }");

    progressLabel2 = new QLabel();
	progressLabel2->setAlignment(Qt::AlignCenter);
    progressLabel2->setWordWrap(1);
    progressLabel2->setStyleSheet("QLabel { font-size:11px; font-weight:400; text-align:center; }");

    progressBar1 = new QProgressBar();
    progressBar1->setRange(0, 100);
    progressBar1->setStyleSheet("QProgressBar { border:3px solid rgb(128, 128, 128); border-radius:5px; text-align:center; }");

    progressBar2 = new QProgressBar();
    progressBar2->setRange(0, 100);
    progressBar2->setStyleSheet("QProgressBar { border:3px solid rgb(128, 128, 128); border-radius:5px; text-align:center; }");

    cancelButton = new QPushButton();
    cancelButton->setIconSize(QSize(32, 32));
    cancelButton->setIcon(QIcon(":/images/cancel48.png"));
    cancelButton->setFixedSize(QSize(32, 32));
    cancelButton->setToolTip(tr("Cancel operation"));
    cancelButton->setCursor(QCursor(Qt::PointingHandCursor));
    cancelButton->setStyleSheet("QPushButton { border:none; background:none; color:transparent; }");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(progressLabel1, 1);
    mainLayout->addWidget(progressBar1);
    mainLayout->addWidget(progressLabel2, 1);
    mainLayout->addWidget(progressBar2);
    mainLayout->addWidget(cancelButton, 0, Qt::AlignRight);
    setLayout(mainLayout);

    setModal(1);
    setWindowModality(Qt::ApplicationModal);
    setWindowTitle("Progress...");
    setWindowIcon(QIcon(":/images/progress48.png"));
    setStyleSheet("QProgressBar::chunk { background-color: rgb(5, 184, 204); }");
	resize(winWidth, winHeight);

	connectEvents();
    resetProgressDialog();
}

void ProgressMeter::connectEvents()
{
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(hide()));
}

void ProgressMeter::setProgressMsg1(QString msg)
{
	progressLabel1->setText(msg);
}

void ProgressMeter::setProgressMsg2(QString msg)
{
	progressLabel2->setText(msg);
}

void ProgressMeter::setProgressRange(int current, int total)
{
    bar1RangeCurrent = current;
    bar1RangeTotal = total;
}

void ProgressMeter::setProgressPercent(qint64 current, qint64 total)
{
    int percent1 = bar1RangeCurrent * 100 / bar1RangeTotal;
    int percent2 = current * 100 / total;
    percent1 += percent2 / bar1RangeTotal;
    progressBar1->setValue(percent1);
    progressBar2->setValue(percent2);
}

void ProgressMeter::resetProgressDialog()
{
    progressLabel1->setText(tr(""));
    progressLabel2->setText(tr(""));
	progressBar1->setValue(0);
	progressBar2->setValue(0);
}

void ProgressMeter::showEvent(QShowEvent *event)
{
	event->accept();
}

void ProgressMeter::hideEvent(QHideEvent *event)
{
    resetProgressDialog();
    event->accept();
}
