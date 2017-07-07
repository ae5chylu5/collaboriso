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

#ifndef PROGRESS_H
#define PROGRESS_H

#include <QDebug>
#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QToolButton>
#include <QtEvents>

class ProgressMeter : public QDialog
{
    Q_OBJECT

public:
    ProgressMeter(QWidget *parent = 0);
    void setProgressMsg1(QString msg);
    void setProgressMsg2(QString msg);
    void setProgressRange(int current, int total); // for progress bar 1 only
    void setProgressPercent(qint64 current, qint64 total); // controls both progress bars
    void resetProgressDialog();

private:
    int bar1RangeCurrent;
    int bar1RangeTotal;
    QLabel *progressLabel1;
    QLabel *progressLabel2;
    QProgressBar *progressBar1;
    QProgressBar *progressBar2;
    QPushButton *cancelButton;
    void connectEvents();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
};

#endif // PROGRESS_H
