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

#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QScrollBar>

#include "endpointlist.h"
#include "xpointview.h"

XPointView::XPointView(QGraphicsScene *scene,QWidget *parent)
  : QGraphicsView(scene,parent)
{
  d_prev_hover_x=-1;
  d_prev_hover_y=-1;
  d_input_cursor=NULL;
  d_output_cursor=NULL;
  d_selection_color=QColor("#CCFFCC");
  d_x_slot_quantity=0;
  d_y_slot_quantity=0;
  if(palette().color(QPalette::Window).value()<128) {
    d_selection_color=QColor("#225522");
  }
  setMouseTracking(true);
}


QSize XPointView::sizeHint() const
{
  return QSize(3+ENDPOINTLIST_ITEM_HEIGHT*d_x_slot_quantity,
	       3+ENDPOINTLIST_ITEM_HEIGHT*d_y_slot_quantity);
}


int XPointView::xSlotQuantity() const
{
  return d_x_slot_quantity;
}


void XPointView::setXSlotQuantity(int quan)
{
  d_x_slot_quantity=quan;
}


int XPointView::ySlotQuantity() const
{
  return d_y_slot_quantity;
}


void XPointView::setYSlotQuantity(int quan)
{
  d_y_slot_quantity=quan;
}


void XPointView::mouseMoveEvent(QMouseEvent *e)
{
  int x_slot=
    1+(horizontalScrollBar()->value()+e->x()-1)/ENDPOINTLIST_ITEM_HEIGHT;
  int y_slot=1+(verticalScrollBar()->value()+e->y()-1)/ENDPOINTLIST_ITEM_HEIGHT;

  if((d_prev_hover_x!=x_slot)||(d_prev_hover_y!=y_slot)) {
    if((x_slot<=d_x_slot_quantity)&&(y_slot<=d_y_slot_quantity)) { 
      d_prev_hover_x=x_slot;
      d_prev_hover_y=y_slot;
      e->accept();
      if(d_input_cursor!=NULL) {
	scene()->removeItem(d_input_cursor);
	delete d_input_cursor;
      }
      if(d_output_cursor!=NULL) {
	scene()->removeItem(d_output_cursor);
	delete d_output_cursor;
      }
      if((x_slot>=0)&&(y_slot>=0)) {
	d_input_cursor=
	  scene()->addRect(0,ENDPOINTLIST_ITEM_HEIGHT*(y_slot-1),
			   ENDPOINTLIST_ITEM_HEIGHT*x_slot-2,24,
			   QPen(d_selection_color),
			   QBrush(d_selection_color));
	d_input_cursor->setZValue(-1);
	
	d_output_cursor=
	  scene()->addRect(ENDPOINTLIST_ITEM_HEIGHT*(x_slot-1),0,24,
			   ENDPOINTLIST_ITEM_HEIGHT*y_slot-2,
			   QPen(d_selection_color),
			   QBrush(d_selection_color));
	d_output_cursor->setZValue(-1);
      }
      emit crosspointSelected(x_slot,y_slot);
    }
    else {
      d_prev_hover_x=-1;
      d_prev_hover_y=-1;
      emit crosspointSelected(-1,-1);
    }
  }
}


void XPointView::leaveEvent(QEvent *e)
{
  d_prev_hover_x=-1;
  d_prev_hover_y=-1;
  if(d_input_cursor!=NULL) {
    scene()->removeItem(d_input_cursor);
    delete d_input_cursor;
    d_input_cursor=NULL;
  }
  if(d_output_cursor!=NULL) {
    scene()->removeItem(d_output_cursor);
    delete d_output_cursor;
    d_output_cursor=NULL;
  }
  emit crosspointSelected(-1,-1);
}


void XPointView::mouseDoubleClickEvent(QMouseEvent *e)
{
  emit crosspointDoubleClicked(1+(horizontalScrollBar()->value()+e->x()-1)/
			       ENDPOINTLIST_ITEM_HEIGHT,
			       1+(verticalScrollBar()->value()+e->y()-1)/
			       ENDPOINTLIST_ITEM_HEIGHT);
  QGraphicsView::mouseDoubleClickEvent(e);
}
