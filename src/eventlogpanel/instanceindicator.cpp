// instanceindicator.cpp
//
// InstanceIndicator widget
//
//   (C) Copyright 2022-2024 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
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

#include "instanceindicator.h"
#include <drouter/drsqlquery.h>

InstanceIndicator::InstanceIndicator(QWidget *parent)
  : QLabel(parent)
{
  setAlignment(Qt::AlignCenter);
  setFrameStyle(QFrame::Panel|QFrame::Sunken);
  setLineWidth(1);
  setMidLineWidth(0);

  d_timer=new QTimer(this);
  d_timer->setSingleShot(true);
  connect(d_timer,SIGNAL(timeout()),this,SLOT(timeoutData()));
  d_timer->start(1000);
}


QSize InstanceIndicator::sizeHint() const
{
  return QSize(400,300);
}


void InstanceIndicator::timeoutData()
{
  QString sql;
  DRSqlQuery *q=NULL;

  sql=QString("select ")+
    "`IS_ACTIVE` "+  // 00
    "from `TETHER`";
  q=new DRSqlQuery(sql);
  if(q->first()) {
    if(q->value(0).toString()=="Y") {
      setText(tr("Active"));
      setStyleSheet("color: #FFFFFF; background-color: #009900");
      setEnabled(true);
    }
    else {
      setText(tr("Inactive"));
      setStyleSheet("");
      //      setStyleSheet("color: #FFFFFF; background-color: #DD0000");
      setDisabled(true);
    }
  }
  delete q;

  d_timer->start(1000);
}
