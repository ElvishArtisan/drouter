// autopushbutton.cpp
//
// QPushButton with autosizing text
//
//   (C) Copyright 2017-2022 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
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

#include <QEvent>

#include "autopushbutton.h"

AutoPushButton::AutoPushButton(QWidget *parent)
  : QPushButton(parent)
{
  auto_font_family=font().family();
  auto_minimum_point_size=font().pointSize();
}


void AutoPushButton::setFontFamily(const QString &str)
{
  auto_font_family=str;
}


int AutoPushButton::minimumPointSize() const
{
  return auto_minimum_point_size;
}


void AutoPushButton::setMinimumPointSize(int size)
{
  auto_minimum_point_size=size;
  ComposeText();
}


void AutoPushButton::setText(const QString &str)
{
  auto_plain_text=str.simplified();
  ComposeText();
}


void AutoPushButton::resizeEvent(QResizeEvent *e)
{
  ComposeText();
}


void AutoPushButton::ComposeText()
{
  int lines;
  QStringList f0=auto_plain_text.split(" ",QString::SkipEmptyParts);
  QFont font(auto_font_family,(double)size().height()/2.0,QFont::Bold);
  QString accum;
  QString text;
  int height;
  bool singleton;
  int w=90*sizeHint().width()/100;
  int h=90*sizeHint().height()/100;

  do {
    singleton=false;
    accum="";
    text="";
    font=QFont(font.family(),font.pointSize()-2,QFont::Bold);
    QFontMetrics fm(font);
    lines=1;
    for(int i=0;i<f0.size();i++) {
      if((fm.width(accum+f0.at(i)+" "))>w) {
	if(fm.width(f0.at(i))>w) {
	  singleton=true;
	  break;
	}
	lines++;
	accum=f0.at(i)+" ";
	text+="\n";
      }
      else {
	accum+=f0.at(i)+" ";
      }
      text+=f0.at(i)+" ";
    }
    height=lines*fm.lineSpacing();
  } while(singleton||(((height>h))&&
		      (font.pointSize()>auto_minimum_point_size)));
  if(text.isEmpty()) {  // Back out if no solution found
    text=auto_plain_text;
  }
  QPushButton::setText(text.trimmed());
  QPushButton::setFont(font);
}
