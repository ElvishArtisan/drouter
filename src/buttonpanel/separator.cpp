// separator.cpp
//
// Separator widget for a GPIO strip
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

#include <QPainter>

#include "separator.h"

Separator::Separator(QWidget *parent)
  : QWidget(parent)
{
}


Separator::~Separator()
{
}


QSize Separator::sizeHint() const
{
  return QSize(10,40);
}


void Separator::paintEvent(QPaintEvent *e)
{
  int w=size().width();
  int h=size().height();
  QPainter *p=new QPainter(this);

  p->setPen(Qt::black);
  p->setBrush(Qt::black);

  p->drawLine(w/2,0,w/2,h);

  delete p;
}
