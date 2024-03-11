// drmultistatewidget.cpp
//
// Widget to display GPIO code state
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

#include "drmultistatewidget.h"

DRMultiStateWidget::DRMultiStateWidget(int router,int linenum,
				   Qt::Orientation orient,QWidget *parent)
  : QWidget(parent)
{
  state_orientation=orient;
  state_router=router;
  state_linenum=linenum+1;
}


QSize DRMultiStateWidget::sizeHint() const
{
  if(state_orientation==Qt::Horizontal) {
    return QSize(DRMULTISTATEWIDGET_LONG_EDGE+10,
		 DRMULTISTATEWIDGET_SHORT_EDGE+8);
  }
  return QSize(DRMULTISTATEWIDGET_SHORT_EDGE+10,
	       DRMULTISTATEWIDGET_LONG_EDGE+8);
}


QString DRMultiStateWidget::state() const
{
  return state_state;
}


void DRMultiStateWidget::setState(int router,int linenum,const QString &code)
{
  if((router==state_router)&&(linenum==state_linenum)&&
     (code.toLower()!=state_state)) {
    state_state=code.toLower();
    update();
  }
}


void DRMultiStateWidget::paintEvent(QPaintEvent *e)
{
  int h=size().height();
  int w=size().width();
  QPainter *p=new QPainter(this);

  QColor background_color("#CCCCCC");
  QColor frame_color("#000000");
  QColor on_color("#00FF00");
  QColor off_color("#444444");

  if(state_orientation==Qt::Horizontal) {
    //
    // Draw Background
    //
    p->setPen(background_color);
    p->setBrush(background_color);
    p->drawRoundedRect(0,0,size().width(),size().height(),2.5,2.5);
    p->fillRect((w-50)/2,
		(h-DRMULTISTATEWIDGET_SHORT_EDGE)/2,
		50,
		DRMULTISTATEWIDGET_SHORT_EDGE,
		background_color);

    //
    // Draw Frame
    //
    p->setPen(frame_color);
    p->setBrush(frame_color);
    p->drawRect((w-50)/2,
		(h-DRMULTISTATEWIDGET_SHORT_EDGE)/2,
		50,
		DRMULTISTATEWIDGET_SHORT_EDGE);

    for(int i=0;i<SWITCHYARD_GPIO_BUNDLE_SIZE;i++) {
      if(state().mid(i,1).toLower()=="l") {
	p->setPen(on_color);
	p->setBrush(on_color);
      }
      else {
	p->setPen(off_color);
	p->setBrush(off_color);
      }
      p->drawRect((w-50)/2+i*DRMULTISTATEWIDGET_SHORT_EDGE+2,
		  (h-DRMULTISTATEWIDGET_SHORT_EDGE)/2+2,
		  DRMULTISTATEWIDGET_SHORT_EDGE-4,
		  DRMULTISTATEWIDGET_SHORT_EDGE-4);
    }
  }

  if(state_orientation==Qt::Vertical) {
    //
    // Draw Background
    //
    p->setPen(background_color);
    p->setBrush(background_color);
    p->drawRoundedRect(0,0,size().width(),size().height(),2.5,2.5);
    p->fillRect((h-DRMULTISTATEWIDGET_SHORT_EDGE)/2,
		(w-50)/2,
		DRMULTISTATEWIDGET_SHORT_EDGE,
		50,
		background_color);

    //
    // Draw Frame
    //
    p->setPen(frame_color);
    p->setBrush(frame_color);
    p->drawRect((w-DRMULTISTATEWIDGET_SHORT_EDGE)/2,
		(h-50)/2,
		DRMULTISTATEWIDGET_SHORT_EDGE,
		50);

    for(int i=0;i<SWITCHYARD_GPIO_BUNDLE_SIZE;i++) {
      if(state().mid(i,1).toLower()=="l") {
	p->setPen(on_color);
	p->setBrush(on_color);
      }
      else {
	p->setPen(off_color);
	p->setBrush(off_color);
      }
      p->drawRect((w-DRMULTISTATEWIDGET_SHORT_EDGE)/2+2,
		  (h-50)/2+i*DRMULTISTATEWIDGET_SHORT_EDGE+2,
		  DRMULTISTATEWIDGET_SHORT_EDGE-4,
		  DRMULTISTATEWIDGET_SHORT_EDGE-4);
    }
  }

  delete p;
}
