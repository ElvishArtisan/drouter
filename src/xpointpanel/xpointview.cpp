// xpointview.cpp
//
// QGraphicsScene viewer for xpointpanel(1)
//
//   (C) Copyright 2017-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <QMouseEvent>
#include <QScrollBar>

#include "xpointview.h"

XPointView::XPointView(QGraphicsScene *scene,QWidget *parent)
  : QGraphicsView(scene,parent)
{
  d_prev_hover_x=-1;
  d_prev_hover_y=-1;
  setMouseTracking(true);
}


void XPointView::mouseMoveEvent(QMouseEvent *e)
{
  int x_slot=1+(horizontalScrollBar()->value()+e->x()-1)/26;
  int y_slot=1+(verticalScrollBar()->value()+e->y()-1)/26;

  if((d_prev_hover_x!=x_slot)||(d_prev_hover_y!=y_slot)) {
    d_prev_hover_x=x_slot;
    d_prev_hover_y=y_slot;
    e->accept();
    emit crosspointSelected(x_slot,y_slot);
  }
}


void XPointView::leaveEvent(QEvent *e)
{
  d_prev_hover_x=-1;
  d_prev_hover_y=-1;
  emit crosspointSelected(-1,-1);
}


void XPointView::mouseDoubleClickEvent(QMouseEvent *e)
{
  emit crosspointDoubleClicked(1+(horizontalScrollBar()->value()+e->x()-1)/26,
			       1+(verticalScrollBar()->value()+e->y()-1)/26);
  QGraphicsView::mouseDoubleClickEvent(e);
}
