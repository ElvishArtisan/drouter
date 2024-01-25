// gpiowidget.cpp
//
// Strip container for GPIO controls.
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

#include "gpiowidget.h"
#include "multistatelabel.h"
#include "separator.h"
#include "statebutton.h"
#include "statelight.h"

GpioWidget::GpioWidget(GpioParser *gpio_parser,DRJParser *sa_parser,
		       QWidget *parent)
  : QWidget(parent)
{
  c_parser=sa_parser;
  c_hint_width=0;
  c_hint_height=0;

  //
  // Fonts
  //
  QFont title_font(font().family(),14,QFont::Bold);

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
    if(gpio_parser->type(i)==GpioParser::Lamp) {
      StateLight *w=NULL;
      w=new StateLight(gpio_parser->router(i),gpio_parser->endPoint(i),
		       gpio_parser->legend(i),gpio_parser->mask(i),
		       gpio_parser->direction(i),c_parser,this);
      c_widgets.push_back(w);
      QString colorstr=gpio_parser->color(i);
      if(colorstr=="black") {
	w->setTextColor("#000000");
	w->setBackgroundColor("#FFFFFF");
      }
      if(colorstr=="blue") {
	w->setTextColor("#FFFFFF");
	w->setBackgroundColor("#0000FF");
      }
      if(colorstr=="cyan") {
	w->setTextColor("#FFFFFF");
	w->setBackgroundColor("#008888");
      }
      if(colorstr=="green") {
	w->setTextColor("#FFFFFF");
	w->setBackgroundColor("#008800");
      }
      if(colorstr=="magenta") {
	w->setTextColor("#FFFFFF");
	w->setBackgroundColor("#880088");
      }
      if(colorstr=="red") {
	w->setTextColor("#FFFFFF");
	w->setBackgroundColor("#CC0000");
      }
      if(colorstr=="yellow") {
	w->setTextColor("#000000");
	w->setBackgroundColor("#FFFF00");
      }
    }

    if(gpio_parser->type(i)==GpioParser::Button) {
      StateButton *w=NULL;
      w=new StateButton(gpio_parser->router(i),gpio_parser->endPoint(i),
			gpio_parser->legend(i),gpio_parser->mask(i),
			gpio_parser->direction(i),c_parser,this);
      c_widgets.push_back(w);
      QString colorstr=gpio_parser->color(i);

      if(colorstr=="black") {
	w->setTextColor("#000000");
      }
      if(colorstr=="blue") {
	w->setTextColor("#0000FF");
      }
      if(colorstr=="cyan") {
	w->setTextColor("#008888");
      }
      if(colorstr=="green") {
	w->setTextColor("#008800");
      }
      if(colorstr=="magenta") {
	w->setTextColor("#880088");
      }
      if(colorstr=="red") {
	w->setTextColor("#CC0000");
      }
      if(colorstr=="yellow") {
	w->setTextColor("#FFFF00");
      }
    }

    if(gpio_parser->type(i)==GpioParser::Separator) {
      Separator *w=NULL;
      w=new Separator(this);
      c_widgets.push_back(w);
    }

    if(gpio_parser->type(i)==GpioParser::Label) {
      QLabel *w=NULL;
      w=new QLabel(gpio_parser->legend(i),this);
      w->setAlignment(Qt::AlignCenter);
      w->setFont(QFont(font().family(),font().pointSize(),QFont::Bold));
      c_widgets.push_back(w);
    }

    if(gpio_parser->type(i)==GpioParser::MultiState) {
      MultiStateLabel *w=NULL;
      w=new MultiStateLabel(gpio_parser->router(i),gpio_parser->endPoint(i)-1,
			    gpio_parser->legend(i),this);
      if(gpio_parser->direction(i)==QChar('i')) {
	connect(sa_parser,SIGNAL(gpiStateChanged(int,int,const QString &)),
		w,SLOT(setState(int,int,const QString &)));
      }
      else {
	connect(sa_parser,SIGNAL(gpoStateChanged(int,int,const QString &)),
		w,SLOT(setState(int,int,const QString &)));
      }
      c_widgets.push_back(w);
    }

    c_hint_width+=5+c_widgets.back()->sizeHint().width();
    if(c_widgets.back()->sizeHint().height()>c_hint_height) {
      c_hint_height=c_widgets.back()->sizeHint().height();
    }
    c_widgets.back()->hide();
  }
  c_hint_width-=5;    // Remove unused space after last widget
  if((c_title_label->sizeHint().width()+20)>c_hint_width) {
    c_hint_width=c_title_label->sizeHint().width()+20;
  }

  //
  // The SA Connection
  //
  connect(c_parser,SIGNAL(connected(bool,DRJParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,DRJParser::ConnectionState)));

  show();
}


GpioWidget::~GpioWidget()
{
  for(int i=0;i<c_widgets.size();i++) {
    delete c_widgets.at(i);
  }
  c_widgets.clear();
  delete c_title_label;
}


QSize GpioWidget::sizeHint() const
{
  int height=c_hint_height;
  if(!c_title_label->text().isEmpty()) {
    height+=22;  // Title Height
  }

  return QSize(c_hint_width,height);
}


QString GpioWidget::title() const
{
  return c_title_label->text();
}


void GpioWidget::setTitle(const QString &str)
{
  c_title_label->setText(str);
}


void GpioWidget::processError(const QString &err_msg)
{
  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),err_msg);;
  exit(1);
}


void GpioWidget::resizeEvent(QResizeEvent *e)
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


void GpioWidget::changeConnectionState(bool state,
				       DRJParser::ConnectionState cstate)
{
  c_title_label->setVisible(state);
  for(int i=0;i<c_widgets.size();i++) {
    c_widgets.at(i)->setVisible(state);
  }
}
