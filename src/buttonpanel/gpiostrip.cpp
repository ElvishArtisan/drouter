// gpiostrip.cpp
//
// Strip container for GPIO controls.
//
//   (C) Copyright 2020-2025 Fred Gleason <fredg@paravelsystems.com>
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

#include "ackbutton.h"
#include "alertbutton.h"
#include "gpiostrip.h"
#include "multistatelabel.h"
#include "separator.h"
#include "statebutton.h"
#include "statelight.h"

GpioStrip::GpioStrip(int id,GpioParser *gpio_parser,QWidget *parent)
  : QWidget(parent)
{
  c_id=id;
  c_hint_width=0;
  c_hint_height=0;
  c_summary_alarm_state=false;

  //
  // Fonts
  //
  QFont title_font(font().family(),14,QFont::Bold);

  //
  // Color Maps
  //
  LoadColorMaps();

  //
  // Title
  //
  c_title_label=new QLabel(this);
  c_title_label->setFont(title_font);
  c_title_label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  c_title_label->hide();
  c_title_label->setText(gpio_parser->title());

  //
  // Create Widgets
  //
  for(int i=0;i<gpio_parser->widgetQuantity();i++) {
    if(gpio_parser->type(i)==GpioParser::Ack) {
      AckButton *w=NULL;
      w=new AckButton(gpio_parser->router(i),gpio_parser->endPoint(i),
		      gpio_parser->legend(i),gpio_parser->mask(i),
		      gpio_parser->direction(i),this);
      w->setText(gpio_parser->legend(i));
      connect(w,SIGNAL(clicked()),this,SIGNAL(acknowledgeRequested()));
      c_widgets.push_back(w);
      c_alarm_states.push_back(false);
      QString colorstr=gpio_parser->color(i);
      w->setTextColor(c_background_colors.value(colorstr));
    }

    if(gpio_parser->type(i)==GpioParser::Alert) {
      AlertButton *w=NULL;
      w=new AlertButton(c_widgets.size(),gpio_parser->router(i),
			gpio_parser->endPoint(i),gpio_parser->legend(i),
			gpio_parser->mask(i),this);
      connect(w,SIGNAL(alarmStateChanged(int,bool)),
	      this,SLOT(alarmStateChangedData(int,bool)));
      connect(w,SIGNAL(clicked()),this,SIGNAL(acknowledgeRequested()));
      c_widgets.push_back(w);
      c_alarm_states.push_back(false);
      QString colorstr=gpio_parser->color(i);
      w->setActiveColors(c_text_colors.value(colorstr),
			 c_background_colors.value(colorstr));
    }

    if(gpio_parser->type(i)==GpioParser::Lamp) {
      StateLight *w=NULL;
      w=new StateLight(gpio_parser->router(i),gpio_parser->endPoint(i),
		       gpio_parser->legend(i),gpio_parser->mask(i),
		       gpio_parser->direction(i),this);
      c_widgets.push_back(w);
      c_alarm_states.push_back(false);
      QString colorstr=gpio_parser->color(i);
      w->setTextColor(c_text_colors.value(colorstr));
      w->setBackgroundColor(c_background_colors.value(colorstr));
    }

    if(gpio_parser->type(i)==GpioParser::Button) {
      StateButton *w=NULL;
      w=new StateButton(gpio_parser->router(i),gpio_parser->endPoint(i),
			gpio_parser->legend(i),gpio_parser->mask(i),
			gpio_parser->direction(i),this);
      c_widgets.push_back(w);
      c_alarm_states.push_back(false);
      QString colorstr=gpio_parser->color(i);
      w->setTextColor(c_background_colors.value(colorstr));
    }

    if(gpio_parser->type(i)==GpioParser::Separator) {
      Separator *w=NULL;
      w=new Separator(this);
      c_widgets.push_back(w);
      c_alarm_states.push_back(false);
    }

    if(gpio_parser->type(i)==GpioParser::Label) {
      QLabel *w=NULL;
      w=new QLabel(gpio_parser->legend(i),this);
      w->setAlignment(Qt::AlignCenter);
      w->setFont(QFont(font().family(),font().pointSize(),QFont::Bold));
      c_widgets.push_back(w);
      c_alarm_states.push_back(false);
    }

    if(gpio_parser->type(i)==GpioParser::MultiState) {
      MultiStateLabel *w=NULL;
      w=new MultiStateLabel(gpio_parser->router(i),gpio_parser->endPoint(i)-1,
			    gpio_parser->legend(i),this);
      if(gpio_parser->direction(i)==QChar('i')) {
	connect(jparser,SIGNAL(gpiStateChanged(int,int,const QString &)),
		w,SLOT(setState(int,int,const QString &)));
      }
      else {
	connect(jparser,SIGNAL(gpoStateChanged(int,int,const QString &)),
		w,SLOT(setState(int,int,const QString &)));
      }
      c_widgets.push_back(w);
      c_alarm_states.push_back(false);
    }

    c_hint_width+=5+c_widgets.back()->sizeHint().width();
    if(c_widgets.back()->sizeHint().height()>c_hint_height) {
      c_hint_height=5+c_widgets.back()->sizeHint().height();
    }
    c_widgets.back()->hide();
  }

  c_hint_width-=5;    // Remove unused space after last widget
  if((c_title_label->sizeHint().width()+20)>c_hint_width) {
    c_hint_width=c_title_label->sizeHint().width()+20;
  }

  //
  // The Protocol J Connection
  //
  connect(jparser,SIGNAL(connected(bool,DRJParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,DRJParser::ConnectionState)));

  show();
}


GpioStrip::~GpioStrip()
{
  for(int i=0;i<c_widgets.size();i++) {
    delete c_widgets.at(i);
  }
  c_widgets.clear();
  delete c_title_label;
}


QSize GpioStrip::sizeHint() const
{
  int height=c_hint_height;
  if(!c_title_label->text().isEmpty()) {
    height+=22;  // Title Height
  }

  return QSize(c_hint_width,height);
}


QString GpioStrip::title() const
{
  return c_title_label->text();
}


void GpioStrip::setTitle(const QString &str)
{
  c_title_label->setText(str);
}


bool GpioStrip::summaryAlarmState() const
{
  return c_summary_alarm_state;
}


void GpioStrip::changeConnectionState(bool state,
				      DRJParser::ConnectionState cstate)
{
  c_title_label->setVisible(state);
  for(int i=0;i<c_widgets.size();i++) {
    c_widgets.at(i)->setVisible(state);
  }
}


void GpioStrip::alarmStateChangedData(int id,bool state)
{
  bool summary=false;
  
  c_alarm_states[id]=state;
  if(state) {
    summary=true;
  }
  else {
    for(int i=0;i<c_alarm_states.size();i++) {
      if(c_alarm_states.at(i)) {
	summary=true;
	break;
      }
    }
  }
  if(summary||(summary!=c_summary_alarm_state)) {
    c_summary_alarm_state=summary;
    emit summaryAlarmStateChanged(c_id,summary,state);
  }
}


void GpioStrip::processError(const QString &err_msg)
{
  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),err_msg);;
  exit(1);
}


void GpioStrip::resizeEvent(QResizeEvent *e)
{
  int label_height=22;

  if(c_title_label->text().isEmpty()) {
    label_height=0;
  }
  c_title_label->setGeometry(10,0,size().width(),20);

  int xpos=0;
  for(int i=0;i<c_widgets.size();i++) {
    QWidget *w=c_widgets.at(i);
    w->setGeometry(xpos,label_height,
		   w->sizeHint().width(),40);
    xpos+=w->sizeHint().width()+5;
  }
}


void GpioStrip::LoadColorMaps()
{
  c_text_colors["black"]="#FFFFFF";
  c_background_colors["black"]="#000000";

  c_text_colors["blue"]="#FFFFFF";
  c_background_colors["blue"]="#0000FF";

  c_text_colors["cyan"]="#000000";
  c_background_colors["cyan"]="#008888";

  c_text_colors["green"]="#FFFFFF";
  c_background_colors["green"]="#008800";

  c_text_colors["magenta"]="#FFFFFF";
  c_background_colors["magenta"]="#880088";

  c_text_colors["red"]="#FFFFFF";
  c_background_colors["red"]="#CC0000";

  c_text_colors["yellow"]="#FFFFFF";
  c_background_colors["yellow"]="#FFFF00";
}
