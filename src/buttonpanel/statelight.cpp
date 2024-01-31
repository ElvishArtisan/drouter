// statelight.cpp
//
// Show state of a single GPIO bit.
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

#include <QMessageBox>

#include "statelight.h"

StateLight::StateLight(int router,int endpt,const QString &legend,
		       const QString &mask,const QChar &dir,DRJParser *parser,
		       QWidget *parent)
  : AutoLabel(parent)
{
  c_router=router;
  c_endpt=endpt;
  c_dir=dir;
  c_parser=parser;
  c_mask=mask;
  c_on_stylesheet="";

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

  setText(legend);
  setAlignment(Qt::AlignCenter|Qt::AlignVCenter);

  //
  // The Protocol J Connection
  //
  connect(c_parser,SIGNAL(connected(bool,DRJParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,DRJParser::ConnectionState)));
  if(c_dir==QChar('i')) {
    connect(c_parser,SIGNAL(gpiStateChanged(int,int,const QString &)),
	    this,SLOT(setState(int,int,const QString &)));
  }
  else {
    connect(c_parser,SIGNAL(gpoStateChanged(int,int,const QString &)),
	    this,SLOT(setState(int,int,const QString &)));
  }
  setStyleSheet(STATELIGHT_OFF_STYLESHEET);
}


StateLight::~StateLight()
{
}


QSize StateLight::sizeHint() const
{
  return QSize(80,40);
}


QSizePolicy StateLight::sizePolicy() const
{
  QSizePolicy pol(QSizePolicy::Fixed,QSizePolicy::Fixed);
  pol.setHeightForWidth(true);
  return pol;
}


QColor StateLight::textColor() const
{
  return c_text_color;
}


void StateLight::setTextColor(const QColor &color)
{
  c_text_color=color;
  c_on_stylesheet="color: "+c_text_color.name()+"; background-color: "+
    c_background_color.name()+";";
}


QColor StateLight::backgroundColor() const
{
  return c_background_color;
}


void StateLight::setBackgroundColor(const QColor &color)
{
  c_background_color=color;
  c_on_stylesheet="color: "+c_text_color.name()+"; background-color: "+
    c_background_color.name()+";";
}


void StateLight::changeConnectionState(bool state,
				       DRJParser::ConnectionState cstate)
{
  setEnabled(state);
  updateGeometry();
}


void StateLight::setState(int router,int endpt,const QString &code)
{
  if((router==c_router)&&(endpt==c_endpt)) {
    if(code.at(c_mask_bit)==c_mask.at(c_mask_bit)) {
      setStyleSheet(c_on_stylesheet);
    }
    else {
      setStyleSheet(STATELIGHT_OFF_STYLESHEET);
    }
  }
}


void StateLight::processError(const QString &err_msg)
{
  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),err_msg);;
  exit(1);
}
