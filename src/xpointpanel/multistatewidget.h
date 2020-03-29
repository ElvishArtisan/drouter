// multistatewidget.h
//
// Widget to display GPIO code state
//
//   (C) Copyright 2020 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef MULTISTATEWIDGET_H
#define MULTISTATEWIDGET_H

#include <qwidget.h>

#include <sy/syconfig.h>

#define MULTISTATEWIDGET_SHORT_EDGE 10
#define MULTISTATEWIDGET_LONG_EDGE 50

class MultiStateWidget : public QWidget
{
  Q_OBJECT
 public:
  MultiStateWidget(int router,int linenum,Qt::Orientation orient,
		   QWidget *parent=0);
  QSize sizeHint() const;
  QString state() const;

 public slots:
  void setState(int router,int linenum,const QString &code);

 protected:
  void paintEvent(QPaintEvent *e);

 private:
  Qt::Orientation state_orientation;
  int state_router;
  int state_linenum;
  QString state_state;
};


#endif  // MULTISTATEWIDGET_H
