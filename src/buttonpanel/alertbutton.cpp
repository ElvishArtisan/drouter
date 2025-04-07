// alertbutton.cpp
//
// Pushbutton for the alert widget
//
//   (C) Copyright 2025 Fred Gleason <fredg@paravelsystems.com>
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

#include <QFile>
#include <QMessageBox>

#include <sy6/syconfig.h>

#include "alertbutton.h"
#include "buttonwidget.h"

AlertButton::AlertButton(int id,int router,int endpt,const QString &legend,
			 const QString &mask,QWidget *parent)
  : AutoPushButton(parent)
{
  c_id=id;
  c_router=router;
  c_endpt=endpt;
  c_mask=mask;
  c_alarm_state=false;
  
  //
  // Sanity Check the GPIO Mask
  //
  if(c_mask.count("x")<4) {
    processError(tr("gpio mask is not unique")+" ["+c_mask+"]");
  }
  c_mask_bit=-1;
  for(int i=0;i<c_mask.length();i++) {
    if(c_mask.at(i)!=QChar('x')) {
      c_mask_bit=i;
      break;
    }
  }
  if(c_mask_bit<0) {
    processError(tr("invalid gpio mask")+" ["+c_mask+"]");
  }
  c_inverted_mask=c_mask;
  if(c_inverted_mask.at(c_mask_bit)==QChar('l')) {
    c_inverted_mask.replace("l","h");
  }
  else {
    c_inverted_mask.replace("h","l");
  }

  connect(this,SIGNAL(clicked()),this,SLOT(clickedData()));
  
  setText(legend);
  setFocusPolicy(Qt::NoFocus);

  c_flash_timer=new QTimer(this);
  connect(c_flash_timer,SIGNAL(timeout()),this,SLOT(flashData()));

  //
  // The ProtocolJ Connection
  //
  connect(jparser,SIGNAL(connected(bool,DRJParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,DRJParser::ConnectionState)));
  connect(jparser,SIGNAL(gpiStateChanged(int,int,const QString &)),
	  this,SLOT(setState(int,int,const QString &)));
}


QSize AlertButton::sizeHint() const
{
  return QSize(BUTTONWIDGET_CELL_WIDTH-5,40);
}


QSizePolicy AlertButton::sizePolicy() const
{
  QSizePolicy pol(QSizePolicy::Fixed,QSizePolicy::Fixed);
  pol.setHeightForWidth(true);
  return pol;
}


QColor AlertButton::activeColor() const
{
  return c_active_color;
}


void AlertButton::setActiveColors(const QColor &text,const QColor &backgnd)
{
  c_stylesheets[false]="color: "+backgnd.name()+";";
  c_stylesheets[true]=
    "color: "+text.name()+";background-color: "+backgnd.name()+";";
  setStyleSheet(c_stylesheets[false]);
}


bool AlertButton::alarmIsActive()
{
  return c_alarm_state;
}


void AlertButton::clickedData()
{
  jparser->setGpoState(c_router,c_endpt,c_mask,300);
}


void AlertButton::changeConnectionState(bool state,
					DRJParser::ConnectionState cstate)
{
  setEnabled(state);
}


void AlertButton::setState(int router,int endpt,const QString &code)
{
  QString err_msg;

  if(code.length()==SWITCHYARD_GPIO_BUNDLE_SIZE) {
    if((router==c_router)&&(endpt==c_endpt)) {
      if(code.at(c_mask_bit)==c_mask.at(c_mask_bit)) {
	if(!c_flash_timer->isActive()) {
	  setStyleSheet(c_stylesheets[true]);
	  c_flash_timer->start(400);
	  setAlarmState(true);
	}
      }
      else {
	if(c_flash_timer->isActive()) {
	  setStyleSheet(c_stylesheets[false]);
	  c_flash_timer->stop();
	  setAlarmState(false);
	}
      }
    }
  }
  else {
    fprintf(stderr,"invalid GPIO update \"%s\" received from endpoint %d:%d\n",
	    code.toUtf8().constData(),router,endpt);
  }
}


void AlertButton::flashData()
{
  if(styleSheet()==c_stylesheets[false]) {
    setStyleSheet(c_stylesheets[true]);
  }
  else {
    setStyleSheet(c_stylesheets[false]);
  }
}


void AlertButton::processError(const QString &err_msg)
{
  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),err_msg);;
  exit(1);
}


void AlertButton::setAlarmState(bool state)
{
  if(state!=c_alarm_state) {
    c_alarm_state=state;
    emit alarmStateChanged(c_id,c_alarm_state);
  }
}

