// statebutton.cpp
//
// Set state of a single GPIO bit.
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

#include "statebutton.h"

StateButton::StateButton(int router,int endpt,const QString &legend,
			 const QString &mask,const QChar &dir,DRJParser *parser,
			 QWidget *parent)
  : AutoPushButton(parent)
{
  c_router=router;
  c_endpt=endpt;
  c_mask=mask;
  c_dir=dir;
  c_parser=parser;

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

  setText(legend);
  setFocusPolicy(Qt::NoFocus);

  connect(this,SIGNAL(pressed()),this,SLOT(pressedData()));
  connect(this,SIGNAL(released()),this,SLOT(releasedData()));

  //
  // The SA Connection
  //
  connect(c_parser,SIGNAL(connected(bool,DRJParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,DRJParser::ConnectionState)));
}


QSize StateButton::sizeHint() const
{
  return QSize(80,40);
}


QSizePolicy StateButton::sizePolicy() const
{
  QSizePolicy pol(QSizePolicy::Fixed,QSizePolicy::Fixed);
  pol.setHeightForWidth(true);
  return pol;
}


QColor StateButton::textColor() const
{
  return c_text_color;
}


void StateButton::setTextColor(const QColor &color)
{
  c_text_color=color;
  setStyleSheet("color: "+color.name()+";");
}


void StateButton::changeConnectionState(bool state,
					DRJParser::ConnectionState cstate)
{
  setEnabled(state);
}


void StateButton::pressedData()
{
  if(c_dir==QChar('i')) {
    c_parser->setGpiState(c_router,c_endpt,c_mask);
  }
  else {
    c_parser->setGpoState(c_router,c_endpt,c_mask);
  }
}


void StateButton::releasedData()
{
  if(c_dir==QChar('i')) {
    c_parser->setGpiState(c_router,c_endpt,c_inverted_mask);
  }
  else {
    c_parser->setGpoState(c_router,c_endpt,c_inverted_mask);
  }
}


void StateButton::processError(const QString &err_msg)
{
  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),err_msg);;
  exit(1);
}
