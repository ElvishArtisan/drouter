// dowselector.cpp
//
// Day of the week selector
//
//   (C) Copyright 2021-2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <QDateTime>

#include "dowselector.h"

DowSelector::DowSelector(QWidget *parent)
  : QWidget(parent)
{
  QFont bold_font(font().family(),font().pixelSize(),QFont::Bold);

  d_group_box=new QGroupBox(tr("Active Days"),this);
  d_group_box->setFont(bold_font);

  QDate monday=QDate::currentDate();
  while(monday.dayOfWeek()!=1) {
    monday=monday.addDays(1);
  }

  for(int i=0;i<7;i++) {
    d_labels[i]=new QLabel(monday.addDays(i).toString("dddd"),this);
    d_labels[i]->setFont(font());
    d_checks[i]=new QCheckBox(this);
    d_checks[i]->setFont(font());
  }
}


QSize DowSelector::sizeHint() const
{
  return QSize(500,64);
}


QSizePolicy DowSelector::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}


bool DowSelector::dayOfWeekEnabled(int dow)
{
  return d_checks[dow]->isChecked();
}


void DowSelector::toAction(QMap<QString,QVariant> &fields) const
{
  fields["monday"]=d_checks[0]->isChecked();
  fields["tuesday"]=d_checks[1]->isChecked();
  fields["wednesday"]=d_checks[2]->isChecked();
  fields["thursday"]=d_checks[3]->isChecked();
  fields["friday"]=d_checks[4]->isChecked();
  fields["saturday"]=d_checks[5]->isChecked();
  fields["sunday"]=d_checks[6]->isChecked();
}


void DowSelector::fromAction(const QMap<QString,QVariant> &fields)
{
  d_checks[0]->setChecked(fields.value("monday").toBool());
  d_checks[1]->setChecked(fields.value("tuesday").toBool());
  d_checks[2]->setChecked(fields.value("wednesday").toBool());
  d_checks[3]->setChecked(fields.value("thursday").toBool());
  d_checks[4]->setChecked(fields.value("friday").toBool());
  d_checks[5]->setChecked(fields.value("saturday").toBool());
  d_checks[6]->setChecked(fields.value("sunday").toBool());
}


void DowSelector::clear()
{
  for(int i=0;i<7;i++) {
    d_checks[i]->setChecked(false);
  }
}


void DowSelector::enableDayOfWeek(int dow,bool state)
{
  d_checks[dow]->setChecked(state);
}


void DowSelector::resizeEvent(QResizeEvent *e)
{
  d_group_box->setGeometry(0,0,width(),height());

  d_checks[0]->setGeometry(10,21,20,20);
  d_labels[0]->setGeometry(30,21,115,20);
  
  d_checks[1]->setGeometry(105,21,20,20);
  d_labels[1]->setGeometry(125,21,115,20);
  
  d_checks[2]->setGeometry(205,21,20,20);
  d_labels[2]->setGeometry(225,21,115,20);
  
  d_checks[3]->setGeometry(325,21,20,20);
  d_labels[3]->setGeometry(345,21,115,20);
  
  d_checks[4]->setGeometry(430,21,20,20);
  d_labels[4]->setGeometry(450,21,40,20);
  
  d_checks[5]->setGeometry(120,43,20,20);
  d_labels[5]->setGeometry(140,43,60,20);
  
  d_checks[6]->setGeometry(290,43,20,20);
  d_labels[6]->setGeometry(310,43,60,20);
}
