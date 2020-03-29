// endpointlist.h
//
// Input/Output labels for xpointpanel(1)
//
//   (C) Copyright 2017-2020 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef ENDPOINTLIST_H
#define ENDPOINTLIST_H

#include <QMap>
#include <QWidget>

#include "multistatewidget.h"

class EndpointList : public QWidget
{
  Q_OBJECT
 public:
  EndpointList(Qt::Orientation orient,QWidget *parent=0);
  ~EndpointList();
  QSize sizeHint() const;
  QSizePolicy sizePolicy() const;
  bool showGpio() const;
  void setShowGpio(bool state);
  int endpoint(int slot) const;
  QList<int> endpoints() const;
  int slot(int endpt) const;
  void addEndpoint(int router,int endpt,const QString &name);
  void addEndpoints(int router,const QMap<int,QString> &endpts);
  void clearEndpoints();
  int endpointQuantity() const;

 public slots:
  void setPosition(int pixels);
  void setGpioState(int router,int linenum,const QString &code);

 protected:
  void paintEvent(QPaintEvent *e);
  void resizeEvent(QResizeEvent *e);

 private:
  QMap<int,QString> list_labels;
  QMap<int,MultiStateWidget *> list_gpio_widgets;
  int list_position;
  Qt::Orientation list_orientation;
  bool list_show_gpio;
  int list_width;
};


#endif  // ENDPOINTLIST_H
