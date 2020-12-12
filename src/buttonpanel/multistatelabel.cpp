// multistatelabel.cpp
//
// MultiStateWidget with a text label
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

#include "multistatelabel.h"

MultiStateLabel::MultiStateLabel(int router,int linenum,const QString &legend,
				 QWidget *parent)
  : QWidget(parent)
{
  c_label=new AutoLabel(this);
  c_label->setText(legend);
  c_label->setAlignment(Qt::AlignCenter);

  c_widget=new MultiStateWidget(router,linenum,Qt::Horizontal,this);
}


QSize MultiStateLabel::sizeHint() const
{
  return QSize(c_widget->sizeHint().width(),40);
}


QString MultiStateLabel::state() const
{
  return c_widget->state();
}


void MultiStateLabel::setState(int router,int linenum,const QString &code)
{
  c_widget->setState(router,linenum,code);
}


void MultiStateLabel::resizeEvent(QResizeEvent *e)
{
  int w=size().width();
  int h=size().height();

  c_widget->setGeometry(0,0,w,h/2);

  c_label->setGeometry(0,h/2,w,h/2);
}
