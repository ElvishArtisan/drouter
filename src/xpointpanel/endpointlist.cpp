// endpointlist.cpp
//
// Input/Output labels for xpointpanel(1)
//
//   (C) Copyright 2017-2024 Fred Gleason <fredg@paravelsystems.com>
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
#include <unistd.h>

#include <QApplication>
#include <QClipboard>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

#include "endpointlist.h"

EndpointList::EndpointList(Qt::Orientation orient,QWidget *parent)
  : QWidget(parent)
{
  list_orientation=orient;
  list_show_gpio=false;
  list_position=0;
  list_width=0;
  list_mouse_endpoint=-1;
  list_move_endpoint=-1;
  list_router=0;
  list_selected_rownum=-1;
  list_selected_font=QFont(font().family(),font().pointSize(),QFont::Bold);

  setMouseTracking(true);

  //
  // Dialogs
  //
  switch(orient) {
  case Qt::Horizontal:
    list_gpio_type=DREndPointMap::Input;
    break;

  case Qt::Vertical:
    list_gpio_type=DREndPointMap::Output;
    break;
  }

  list_mouse_menu=new QMenu(this);
  list_state_dialog_action=list_mouse_menu->
    addAction(tr("Set GPIO State"),this,SLOT(showStateDialogData()));
  list_mouse_menu->addSeparator();
  list_connect_via_http_action=list_mouse_menu->
    addAction("",this,SLOT(connectViaHttpData()));
  list_connect_via_lwrp_action=list_mouse_menu->
    addAction("",this,SLOT(connectViaLwrpData()));
  list_mouse_menu->addSeparator();
  list_copy_source_number_action=list_mouse_menu->
    addAction(tr("Copy Source Number"),this,SLOT(copySourceNumberData()));
  list_copy_source_address_action=list_mouse_menu->
    addAction(tr("Copy Source Stream Address"),
	      this,SLOT(copySourceStreamAddressData()));
  list_copy_host_address_action=list_mouse_menu->
    addAction(tr("Copy Node Address"),this,SLOT(copyNodeAddressData()));
  list_copy_slot_number_action=list_mouse_menu->
    addAction(tr("Copy Slot Number"),this,SLOT(copySlotNumberData()));  
  connect(list_mouse_menu,SIGNAL(aboutToShow()),
	  this,SLOT(aboutToShowMenuData()));
}


EndpointList::~EndpointList()
{
}


QSize EndpointList::sizeHint() const
{
  int width=15+list_width;

  if(list_show_gpio) {
    width+=ENDPOINTLIST_GPIO_WIDTH;
  }
  if(width<ENDPOINTLIST_MIN_INPUT_WIDTH) {
    width=ENDPOINTLIST_MIN_INPUT_WIDTH;
  }

  return QSize(width,ENDPOINTLIST_ITEM_HEIGHT*list_labels.size());
}


QSizePolicy EndpointList::sizePolicy() const
{
  return QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}


int EndpointList::router() const
{
  return list_router;
}


void EndpointList::setRouter(int router)
{
  if(list_router!=router) {
    list_router=router;
    switch(list_orientation) {
    case Qt::Horizontal:
      list_model=list_parser->inputModel(list_router);
      break;

    case Qt::Vertical:
      list_model=list_parser->outputModel(list_router);
      break;
    }
  }
}


void EndpointList::setParser(DRJParser *psr)
{
  list_parser=psr;
}


bool EndpointList::showGpio() const
{
  return list_show_gpio;
}


void EndpointList::setShowGpio(bool state)
{
  list_show_gpio=state;
}


int EndpointList::slot(int endpt) const
{
  int ret=0;

  for(QMap<int,QString>::const_iterator it=list_labels.begin();
      it!=list_labels.end();it++) {
    if((it.key()+1)==endpt) {
      return ret;
    }
    ret++;
  }
  return -1;
}


void EndpointList::addEndpoint(int router,int endpt,const QString &name)
{
  list_labels[endpt]=name;
  list_gpio_widgets[endpt]=
    new DRMultiStateWidget(router,endpt,list_orientation,this);
  list_gpio_widgets.value(endpt)->setVisible(list_show_gpio);

  QFontMetrics fm(list_selected_font);
  for(QMap<int,QString>::const_iterator it=list_labels.begin();
      it!=list_labels.end();it++) {
    if(fm.width(it.value())>list_width) {
      list_width=fm.width(it.value());
    }
  }

  update();
}

void EndpointList::clearEndpoints()
{
  list_labels.clear();
  list_width=0;

  for(QMap<int,DRMultiStateWidget *>::const_iterator it=list_gpio_widgets.begin();
      it!=list_gpio_widgets.end();it++) {
    delete it.value();
  }
  list_gpio_widgets.clear();

  for(QMap<int,StateDialog *>::const_iterator it=list_state_dialogs.begin();
      it!=list_state_dialogs.end();it++) {
    delete it.value();
  }
  list_state_dialogs.clear();

  update();
}


int EndpointList::endpointQuantity() const
{
  return list_labels.size();
}


void EndpointList::selectCrosspoint(int slot_x,int slot_y)
{
  int slot=slot_x;
  if(list_orientation==Qt::Horizontal) {
    slot=slot_y;
  }
  if(slot!=list_selected_rownum) {
    list_selected_rownum=slot;
    if(slot<0) {
      list_selected_rownum=-1;
    }
    else {
      list_selected_rownum=slot;
    }
    update();
  }
}


void EndpointList::setPosition(int pixels)
{
  list_position=pixels;
  repaint();
}


void EndpointList::setGpioState(int router,int linenum,const QString &code)
{
  for(QMap<int,DRMultiStateWidget *>::const_iterator it=list_gpio_widgets.begin();
      it!=list_gpio_widgets.end();it++) {
    it.value()->setState(router,linenum,code);
  }
}


void EndpointList::aboutToShowMenuData()
{
  QMap<QString,QVariant> mdata;

  list_state_dialog_action->setEnabled(list_show_gpio);
  switch(list_orientation) {
  case Qt::Horizontal:
    mdata=list_parser->inputModel(list_router)->
      endPointMetadata(list_mouse_endpoint);
    list_connect_via_http_action->
      setText(tr("Connect to")+" "+
	      mdata.value("hostName").toString()+" "+
	      tr("via HTTP"));
    list_connect_via_lwrp_action->
      setText(tr("Connect to")+" "+
	      mdata.value("hostName").toString()+" "+
	      tr("via LWRP"));
    break;

  case Qt::Vertical:
    mdata=list_parser->outputModel(list_router)->
      endPointMetadata(list_mouse_endpoint);
    list_connect_via_http_action->
      setText(tr("Connect to")+" "+
	      mdata.value("hostName").toString()+" "+
	      tr("via HTTP"));
    list_connect_via_lwrp_action->
      setText(tr("Connect to")+" "+
	      mdata.value("hostName").toString()+" "+
	      tr("via LWRP"));
    break;
  }
  list_connect_via_http_action->setEnabled(true);
  list_connect_via_lwrp_action->setEnabled(true);
  list_copy_source_number_action->setEnabled(list_orientation==Qt::Horizontal);
  list_copy_source_address_action->setEnabled(list_orientation==Qt::Horizontal);
  list_copy_host_address_action->setEnabled(true);
  list_copy_slot_number_action->setEnabled(true);
}


void EndpointList::showStateDialogData()
{
  QRect geo;

  if(list_state_dialogs.value(list_mouse_endpoint)==NULL) {
    list_state_dialogs[list_mouse_endpoint]=
      new StateDialog(list_router,list_mouse_endpoint,list_gpio_type,
		      list_parser,this);
    geo=QRect(list_mouse_position.x(),list_mouse_position.y(),
	   list_state_dialogs.value(list_mouse_endpoint)->sizeHint().width(),
	   list_state_dialogs.value(list_mouse_endpoint)->sizeHint().height());
  }
  else {
    geo=list_state_dialogs.value(list_mouse_endpoint)->geometry();
  }
  list_state_dialogs.value(list_mouse_endpoint)->show();
  if(!geo.isNull()) {
    list_state_dialogs.value(list_mouse_endpoint)->setGeometry(geo);
  }
}


void EndpointList::connectViaHttpData()
{
  char c_str[256];
  QMap<QString,QVariant> mdata;

  switch(list_orientation) {
  case Qt::Horizontal:
    mdata=list_parser->inputModel(list_router)->
      endPointMetadata(list_mouse_endpoint);
    strncpy(c_str,mdata.value("nodeAddress").toString().toUtf8().constData(),
	    255);
    if(fork()==0) {
      execlp("firefox","firefox",c_str,(char *)NULL);
      exit(0);
    }
    break;

  case Qt::Vertical:
    mdata=list_parser->outputModel(list_router)->
      endPointMetadata(list_mouse_endpoint);
    strncpy(c_str,mdata.value("nodeAddress").toString().toUtf8().constData(),
	    255);
    if(fork()==0) {
      execlp("firefox","firefox",c_str,(char *)NULL);
      exit(0);
    }
    break;
  }
}


void EndpointList::connectViaLwrpData()
{
  char c_str[256];
  QMap<QString,QVariant> mdata;

  switch(list_orientation) {
  case Qt::Horizontal:
    mdata=list_parser->inputModel(list_router)->
      endPointMetadata(list_mouse_endpoint);
    strncpy(c_str,mdata.value("nodeAddress").toString().toUtf8().constData(),
	    255);
    if(fork()==0) {
      execlp("lwmon","lwmon","--mode=lwrp",c_str,(char *)NULL);
      exit(0);
    }
    break;

  case Qt::Vertical:
    mdata=list_parser->outputModel(list_router)->
      endPointMetadata(list_mouse_endpoint);
    strncpy(c_str,mdata.value("nodeAddress").toString().toUtf8().constData(),
	    255);
    if(fork()==0) {
      execlp("lwmon","lwmon","--mode=lwrp",c_str,(char *)NULL);
      exit(0);
    }
    break;
  }
}


void EndpointList::copySourceNumberData()
{
  QClipboard *cb=QApplication::clipboard();
  QMap<QString,QVariant> mdata=list_parser->inputModel(list_router)->
    endPointMetadata(list_mouse_endpoint);

  cb->setText(QString::asprintf("%d",mdata.value("sourceNumber").toInt()));
}


void EndpointList::copySourceStreamAddressData()
{
  QClipboard *cb=QApplication::clipboard();
  QMap<QString,QVariant> mdata=list_parser->inputModel(list_router)->
    endPointMetadata(list_mouse_endpoint);

  cb->setText(mdata.value("streamAddress").toString());
}


void EndpointList::copyNodeAddressData()
{
  QClipboard *cb=QApplication::clipboard();
  QMap<QString,QVariant> mdata;

  switch(list_orientation) {
  case Qt::Horizontal:
    mdata=list_parser->inputModel(list_router)->
      endPointMetadata(list_mouse_endpoint);
    cb->setText(mdata.value("hostAddress").toString());
    break;

  case Qt::Vertical:
    mdata=list_parser->outputModel(list_router)->
      endPointMetadata(list_mouse_endpoint);
    cb->setText(mdata.value("hostAddress").toString());
    break;
  }
}


void EndpointList::copySlotNumberData()
{
  QClipboard *cb=QApplication::clipboard();
  QMap<QString,QVariant> mdata;

  switch(list_orientation) {
  case Qt::Horizontal:
    mdata=list_parser->inputModel(list_router)->
      endPointMetadata(list_mouse_endpoint);
    cb->setText(QString::asprintf("%d",mdata.value("slot").toInt()));
    break;

  case Qt::Vertical:
    mdata=list_parser->outputModel(list_router)->
      endPointMetadata(list_mouse_endpoint);
    cb->setText(QString::asprintf("%d",mdata.value("slot").toInt()));
    break;
  }
}


void EndpointList::mousePressEvent(QMouseEvent *e)
{
  if((list_mouse_endpoint=LocalEndpoint(e))>=0) {
    list_mouse_position=e->globalPos();
    list_mouse_menu->popup(e->globalPos());
  }
  QWidget::mousePressEvent(e);
}


void EndpointList::mouseMoveEvent(QMouseEvent *e)
{
  //
  // Get the Endpoint
  //
  int rownum=(e->pos().x()+list_position)/ENDPOINTLIST_ITEM_HEIGHT;
  QPoint pos(e->globalPos().x(),e->globalPos().y()-ENDPOINTLIST_ITEM_HEIGHT-13);

  if(list_orientation==Qt::Horizontal) {
    rownum=(e->pos().y()+list_position)/ENDPOINTLIST_ITEM_HEIGHT;
  }

  //
  // Get Endpoint
  //
  if(rownum>=list_model->rowCount()) {
    return;
  }
  int endpt=list_model->endPointNumber(rownum);

  if(endpt!=list_move_endpoint) {
    list_move_endpoint=endpt;
    list_selected_rownum=rownum;
    update();
    emit hoveredEndpointChanged(list_router,rownum);
  }
}


void EndpointList::leaveEvent(QEvent *event)
{
  if(list_move_endpoint>=0) {
    list_move_endpoint=-1;
    list_selected_rownum=-1;
    update();
    emit hoveredEndpointChanged(list_router,list_move_endpoint);
  }
}


void EndpointList::paintEvent(QPaintEvent *e)
{
  int w=size().width();
  QPainter *p=new QPainter(this);
  int gpio_offset=0;

  if(list_show_gpio)  {
    gpio_offset=ENDPOINTLIST_GPIO_WIDTH;
  }
  p->setFont(font());
  int text_y=(ENDPOINTLIST_ITEM_HEIGHT-p->fontMetrics().height())/2+
    p->fontMetrics().height();
  p->setPen(palette().color(QPalette::WindowText));
  p->setBrush(palette().color(QPalette::WindowText));
  
  if(list_orientation==Qt::Vertical) {
    //
    // Vertical Orientation (Destinations, Outputs)
    //
    p->translate(w-(list_width+15+10),0);
    p->rotate(90.0);

    QMap<int,QString>::const_iterator it=list_labels.begin();
    for(int i=0;i<(ENDPOINTLIST_ITEM_HEIGHT*endpointQuantity());i+=ENDPOINTLIST_ITEM_HEIGHT) {
      if(it.key()==list_selected_rownum) {
	p->setFont(list_selected_font);
      }
      else {
	p->setFont(font());
      }
      if(it!=list_labels.end()) {
	p->drawLine(0,w-(ENDPOINTLIST_ITEM_HEIGHT+i)+list_position-(list_width+15+10),
		    0,w-i+list_position-(list_width+15+10));
	p->drawLine(0,w-i+list_position-(list_width+15+10),
		    list_width+15+gpio_offset,w-i+list_position-(list_width+15+10));
	p->drawText(((list_width+15-5)-p->fontMetrics().width(it.value())),w-(text_y+i+list_width+15)+list_position,
		    it.value());
	it++;
      }
    }
    p->drawLine(0,w-ENDPOINTLIST_ITEM_HEIGHT*endpointQuantity()+list_position-(list_width+15+10),
		list_width+15+gpio_offset,w-ENDPOINTLIST_ITEM_HEIGHT*endpointQuantity()+list_position-(list_width+15+10));
  }
  else {
    //
    // Horizontal Orientation (Sources, Inputs)
    //
    QMap<int,QString>::const_iterator it=list_labels.begin();
    for(int i=0;i<(ENDPOINTLIST_ITEM_HEIGHT*endpointQuantity());i+=ENDPOINTLIST_ITEM_HEIGHT) {
      if(it!=list_labels.end()) {
	if(it.key()==list_selected_rownum) {
	  p->setFont(list_selected_font);
	}
	else {
	  p->setFont(font());
	}
	p->drawLine(0,
		    ENDPOINTLIST_ITEM_HEIGHT+i-list_position,
		    0,
		    i-list_position);
	p->drawLine(0,
		    i-list_position,
		    w+gpio_offset,
		    i-list_position);
	p->drawText(w-p->fontMetrics().width(it.value())-gpio_offset-5,
		    text_y+i-list_position,
		    it.value());
	it++;
      }
    }
    p->drawLine(0,
		ENDPOINTLIST_ITEM_HEIGHT*endpointQuantity()-list_position,
		w,
		ENDPOINTLIST_ITEM_HEIGHT*endpointQuantity()-list_position);
  }
  delete p;

  resizeEvent(NULL);
}


void EndpointList::resizeEvent(QResizeEvent *e)
{
  int w=size().width();
  int h=size().height();
  int ypos=-list_position;

  if(list_orientation==Qt::Horizontal) {
    for(QMap<int,DRMultiStateWidget *>::const_iterator it=
	  list_gpio_widgets.begin();it!=list_gpio_widgets.end();it++) {
      it.value()->setGeometry(w-65,
			      ypos+4,
			      60,
			      18);
      ypos+=ENDPOINTLIST_ITEM_HEIGHT;
    }
  }

  if(list_orientation==Qt::Vertical) {
    for(QMap<int,DRMultiStateWidget *>::const_iterator it=
	  list_gpio_widgets.begin();it!=list_gpio_widgets.end();it++) {
      it.value()->setGeometry(ypos+4,
			      h-65,
			      18,
			      60);
      ypos+=ENDPOINTLIST_ITEM_HEIGHT;
    }
  }
}


int EndpointList::LocalEndpoint(QMouseEvent *e) const
{
  int rownum=-1;
  int endpt=-1;

  if(e->button()==Qt::RightButton) {
    switch(list_orientation) {
    case Qt::Horizontal:
      rownum=(e->pos().y()+list_position)/ENDPOINTLIST_ITEM_HEIGHT;
      break;

    case Qt::Vertical:
      rownum=(e->pos().x()+list_position)/ENDPOINTLIST_ITEM_HEIGHT;
      break;
    }
    if((rownum<0)||(rownum>=list_model->rowCount())) {
      return -1;
    }
    endpt=list_model->endPointNumber(rownum);
  }
  else {
    endpt=-1;
  }

  return endpt;
}
