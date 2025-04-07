// ackbutton.cpp
//
// Acknowledge an alert.
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

#include <QMessageBox>

#include <sy6/syconfig.h>

#include "ackbutton.h"

AckButton::AckButton(int router,int endpt,const QString &legend,
		     const QString &mask,const QChar &dir,QWidget *parent)
  : AutoPushButton(parent)
{
  c_router=router;
  c_endpt=endpt;
  c_mask=mask;

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

  //
  // The ProtocolJ Connection
  //
  connect(jparser,SIGNAL(connected(bool,DRJParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,DRJParser::ConnectionState)));
  if(dir==QChar('i')) {
    connect(jparser,SIGNAL(gpiStateChanged(int,int,const QString &)),
	    this,SLOT(setState(int,int,const QString &)));
  }
  else {
    connect(jparser,SIGNAL(gpoStateChanged(int,int,const QString &)),
	    this,SLOT(setState(int,int,const QString &)));
  }
}


QSize AckButton::sizeHint() const
{
  return QSize(80,40);
}


QSizePolicy AckButton::sizePolicy() const
{
  QSizePolicy pol(QSizePolicy::Fixed,QSizePolicy::Fixed);
  pol.setHeightForWidth(true);
  return pol;
}


QColor AckButton::textColor() const
{
  return c_text_color;
}


void AckButton::setTextColor(const QColor &color)
{
  c_text_color=color;
  setStyleSheet("color: "+color.name()+";");
}


void AckButton::changeConnectionState(bool state,
				      DRJParser::ConnectionState cstate)
{
  setEnabled(state);
}


void AckButton::setState(int router,int endpt,const QString &code)
{
  if(code.length()==SWITCHYARD_GPIO_BUNDLE_SIZE) {
    if((router==c_router)&&(endpt==c_endpt)) {
      if(code.at(c_mask_bit)==c_mask.at(c_mask_bit)) {
	emit clicked();
      }
    }
  }
  else {
    fprintf(stderr,"invalid GPIO update \"%s\" received from endpoint %d:%d\n",
	    code.toUtf8().constData(),router,endpt);
  }
}


void AckButton::processError(const QString &err_msg)
{
  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),err_msg);;
  exit(1);
}
