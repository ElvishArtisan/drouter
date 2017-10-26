// endpointlist.h
//
// Input/Output labels for xpointpanel(1)
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

class EndpointList : public QWidget
{
  Q_OBJECT
 public:
  enum Orientation {Horizontal=0,Vertical=1};
  EndpointList(Orientation orient,QWidget *parent=0);
  ~EndpointList();
  QSize sizeHint() const;
  QSizePolicy sizePolicy() const;
  QList<int> endpoints() const;
  void addEndpoint(int endpt,const QString &name);
  void addEndpoints(const QMap<int,QString> &endpts);
  void clearEndpoints();
  int endpointQuantity() const;

 public slots:
  void setPosition(int pixels);

 protected:
  void paintEvent(QPaintEvent *e);

 private:
  QMap<int,QString> list_labels;
  int list_position;
  Orientation list_orientation;
  int list_width;
};


#endif  // ENDPOINTLIST_H
