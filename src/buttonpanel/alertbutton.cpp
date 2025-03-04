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

AlertButton::AlertButton(int router,int endpt,const QString &legend,
			 const QString &mask,const QChar &dir,
			 const QString &snd_filename,DRJParser *parser,
			 SoundPlayer *player,QWidget *parent)
  : AutoPushButton(parent)
{
  c_router=router;
  c_endpt=endpt;
  c_mask=mask;
  c_dir=dir;
  c_sound_filename=snd_filename;
  c_sound_player=player;
  c_parser=parser;

  //
  // Verify The Sound File
  //
  QStringList paths=player->filePaths();
  bool found=false;
  for(int i=0;i<paths.size();i++) {
    if(QFile::exists(paths.at(i)+"/"+snd_filename)) {
      found=true;
    }
  }
  if(!found) {
    QMessageBox::warning(this,"ButtonPanel - "+tr("Warning"),
			 tr("Audio file")+" \""+snd_filename+"\" "+
			 tr("not found."));
  }
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

  connect(this,SIGNAL(clicked()),this,SLOT(clickedData()));

  c_flash_timer=new QTimer(this);
  connect(c_flash_timer,SIGNAL(timeout()),this,SLOT(flashData()));

  //
  // The ProtocolJ Connection
  //
  connect(c_parser,SIGNAL(connected(bool,DRJParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,DRJParser::ConnectionState)));
  if(c_dir.toLower()=='i') {
    connect(c_parser,SIGNAL(gpiStateChanged(int,int,const QString &)),
	    this,SLOT(setState(int,int,const QString &)));
  }
  if(c_dir.toLower()=='o') {
    connect(c_parser,SIGNAL(gpoStateChanged(int,int,const QString &)),
	    this,SLOT(setState(int,int,const QString &)));
  }
}


QSize AlertButton::sizeHint() const
{
  return QSize(80,40);
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
	setStyleSheet(c_stylesheets[true]);
	c_flash_timer->start(400);
	if(!c_sound_filename.isEmpty()) {
	  c_sound_player->play(c_sound_filename,true,&err_msg);
	}
      }
      else {
	setStyleSheet(c_stylesheets[false]);
	c_flash_timer->stop();
	if(!c_sound_filename.isEmpty()) {
	  c_sound_player->stop();
	}
      }
    }
  }
  else {
    fprintf(stderr,"invalid GPIO update \"%s\" received from endpoint %d:%d\n",
	    code.toUtf8().constData(),router,endpt);
  }
}


void AlertButton::clickedData()
{
  if(!c_sound_filename.isEmpty()) {
    c_sound_player->stop();
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
