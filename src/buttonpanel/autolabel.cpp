// autolabel.cpp
//
// Label widget with automatic text sizing
//
// (C) 2017-2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <QFontMetrics>
#include <QStringList>

#include "autolabel.h"

AutoLabel::AutoLabel(QWidget *parent)
  : QLabel(parent)
{
  auto_aspect_scale=1.0;
  auto_font_family=font().family();
  auto_minimum_point_size=font().pointSize();
}


double AutoLabel::aspectScale() const
{
  return auto_aspect_scale;
}


void AutoLabel::setAspectScale(double ratio)
{
  auto_aspect_scale=ratio;
}


void AutoLabel::setFontFamily(const QString &str)
{
  auto_font_family=str;
}


int AutoLabel::minimumPointSize() const
{
  return auto_minimum_point_size;
}


void AutoLabel::setMinimumPointSize(int size)
{
  auto_minimum_point_size=size;
  ComposeText();
}


void AutoLabel::setText(const QString &str)
{
  if(str.simplified()!=auto_plain_text) {
    auto_plain_text=str.simplified();
    ComposeText();
  }
}


void AutoLabel::show()
{
  ComposeText();
}


void AutoLabel::resizeEvent(QResizeEvent *e)
{
  ComposeText();
}


void AutoLabel::ComposeText()
{
  int lines;
  QStringList f0=auto_plain_text.split(" ",Qt::SkipEmptyParts);
  QFont font(auto_font_family,(double)size().height()/(2.0*auto_aspect_scale),
	     QFont::Bold);
  QString accum;
  QString text;
  int height;
  bool singleton;

  do {
    singleton=false;
    accum="";
    text="";
    font=QFont(font.family(),font.pointSize()-2,QFont::Bold);
    QFontMetrics fm(font);
    lines=1;
    for(int i=0;i<f0.size();i++) {
      if((fm.horizontalAdvance(accum+f0.at(i)+" "))>size().width()) {
	if(fm.horizontalAdvance(f0.at(i))>size().width()) {
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
  } while((singleton&&(font.pointSize()>auto_minimum_point_size))||
	  (((height>size().height()))&&(font.pointSize()>auto_minimum_point_size)));
  if(text.isEmpty()) {  // Back out if no solution found
    text=auto_plain_text;
  }
  QLabel::setText(text.trimmed());
  QLabel::setFont(font);
}
