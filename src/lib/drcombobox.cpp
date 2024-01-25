// drcombobox.cpp
//
// ComboBox widget
//
//   (C) Copyright 2015 Fred Gleason <fredg@paravelsystems.com>
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

#include <stdio.h>

#include "drcombobox.h"

DRComboBox::DRComboBox(QWidget *parent)
  : QComboBox(parent)
{
  box_read_only=false;
}


void DRComboBox::setReadOnly(bool state)
{
  box_read_only=state;
}


QVariant DRComboBox::currentItemData(int role)
{
  return itemData(currentIndex(),role);
}


bool DRComboBox::setCurrentItemData(unsigned val)
{
  for(int i=0;i<count();i++) {
    if(itemData(i).toUInt()==val) {
      setCurrentIndex(i);
      return true;
    }
  }
  return false;
}


void DRComboBox::keyPressEvent(QKeyEvent *e)
{
  if(box_read_only) {
    e->accept();
  }
  else {
    QComboBox::keyPressEvent(e);
  }
}


void DRComboBox::mousePressEvent(QMouseEvent *e)
{
  if(box_read_only) {
    e->accept();
  }
  else {
    QComboBox::mousePressEvent(e);
  }
}
