// sidelabel.cpp
//
// QLabel turned on its side.
//
//   (C) Copyright 2022 Fred Gleason <fredg@paravelsystems.com>
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

#include "sidelabel.h"

SideLabel::SideLabel(const QString &text,QWidget *parent)
  : QLabel(text,parent)
{
}


SideLabel::~SideLabel()
{
}


QSize SideLabel::sizeHint() const
{
  return QLabel::sizeHint();
}


QSizePolicy SideLabel::sizePolicy() const
{
  return QLabel::sizePolicy();
}


void SideLabel::paintEvent(QPaintEvent *e)
{
  QFrame::paintEvent(e);

  QPainter *p=new QPainter(this);

  p->rotate(90);
  p->setPen(Qt::black);
  p->setBrush(Qt::black);
  p->setFont(font());

  p->drawText((size().height()-p->fontMetrics().width(text()))/2,-6,text());

  p->end();
  delete p;
}


void SideLabel::resizeEvent(QResizeEvent *e)
{
  QLabel::resizeEvent(e);
}
