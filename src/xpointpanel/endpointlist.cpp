// endpointlist.cpp
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

#include <stdio.h>

#include <QFontMetrics>
#include <QPainter>

#include "endpointlist.h"

EndpointList::EndpointList(EndpointList::Orientation orient,QWidget *parent)
  : QWidget(parent)
{
  list_orientation=orient;
  list_position=0;
}


EndpointList::~EndpointList()
{
}


QSize EndpointList::sizeHint() const
{
  return QSize(250,26*list_labels.size());
}


QSizePolicy EndpointList::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}


QList<int> EndpointList::endpoints() const
{
  QList<int> ret;

  for(QMap<int,QString>::const_iterator it=list_labels.begin();
      it!=list_labels.end();it++) {
    ret.push_back(it.key());
  }
  return ret;
}


void EndpointList::addEndpoint(int endpt,const QString &name)
{
  list_labels[endpt]=name;
  update();
}


void EndpointList::clearEndpoints()
{
  list_labels.clear();
  update();
}


int EndpointList::endpointQuantity() const
{
  return list_labels.size();
}


void EndpointList::setPosition(int pixels)
{
  list_position=pixels;
  repaint();
}


void EndpointList::paintEvent(QPaintEvent *e)
{
  QFontMetrics fm(font());
  int w=size().width();
  int text_y=(26-fm.height())/2+fm.height();
  QPainter *p=new QPainter(this);

  p->setFont(font());
  p->setPen(Qt::black);
  p->setBrush(Qt::black);
  
  if(list_orientation==EndpointList::Vertical) {
    p->translate(w-260,0);
    p->rotate(90.0);

    QMap<int,QString>::const_iterator it=list_labels.begin();
    for(int i=0;i<(26*endpointQuantity());i+=26) {
      if(it!=list_labels.end()) {
	p->drawLine(0,w-(26+i)+list_position,0,w-i+list_position);
	p->drawLine(250,w-(26+i)+list_position,250,w-i+list_position);
	p->drawLine(0,w-i+list_position,250,w-i+list_position);
	p->drawText((245-fm.width(it.value())),w-(text_y+i+250)+list_position,
		    it.value());
	it++;
      }
    }
  }
  else {
    QMap<int,QString>::const_iterator it=list_labels.begin();
    for(int i=0;i<(26*endpointQuantity());i+=26) {
      if(it!=list_labels.end()) {
	p->drawLine(0,26+i-list_position,0,i-list_position);
	p->drawLine(0,i-list_position,250,i-list_position);
	p->drawLine(250,26+i-list_position,250,i-list_position);
	p->drawText(245-fm.width(it.value()),text_y+i-list_position,it.value());
	it++;
      }
    }
  }
  delete p;
}
