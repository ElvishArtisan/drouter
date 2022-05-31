// gpioparser.cpp
//
// Parse GPIO widget arguments
//
//   (C) Copyright 2020-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <QObject>

#include "gpioparser.h"

QString GpioParser::title() const
{
  return c_title;
}


int GpioParser::widgetQuantity() const
{
  return c_types.size();
}


GpioParser::Type GpioParser::type(int n) const
{
  return c_types.at(n);
}


QString GpioParser::color(int n) const
{
  return c_colors.at(n);
}


QChar GpioParser::direction(int n) const
{
  return c_directions.at(n);
}


int GpioParser::router(int n) const
{
  return c_routers.at(n);
}


int GpioParser::endPoint(int n) const
{
  return c_end_points.at(n);
}


QString GpioParser::legend(int n) const
{
  return c_legends.at(n);
}


QString GpioParser::mask(int n) const
{
  return c_masks.at(n);
}


GpioParser *GpioParser::fromString(const QString &str,QString *err_msg)
{
  QList<GpioParser::Type> types;
  QStringList colors;
  QList<QChar> dirs;
  QList<int> routers;
  QList<int> endpts;
  QStringList legends;
  QStringList masks;

  GpioParser::Type type=GpioParser::LastType;
  QString color;
  QChar dir;
  int router=-1;
  int endpt=-1;
  QString legend;
  QString mask;

  bool ok=false;
  QStringList f0=str.split("/",Qt::KeepEmptyParts);
  if(f0.size()<2) {
    *err_msg=QObject::tr("invalid --gpio argument");
    return NULL;
  }
  QString title=f0.at(1).trimmed();

  for(int i=2;i<f0.size();i++) {
    //
    // Get the type
    //
    QStringList f1=f0.at(i).split(":",Qt::KeepEmptyParts);
    if(!TypeFromString(f1.at(0).trimmed(),&type)) {
      *err_msg=QObject::tr("unknown widget type")+" \""+f1.at(0).trimmed()+"\"";
      return NULL;
    }
    if(f1.size()!=GpioParser::ArgQuantityFromType(type)) {
      *err_msg=QObject::tr("invalid --gpio argument")+
	" [\""+f0.at(i)+"\"]";
      return NULL;
    }

    //
    // Lamp or Button widget
    //
    if((type==GpioParser::Lamp)||(type==GpioParser::Button)) {

      //
      // Color
      //
      QStringList colornames;
      colornames.push_back("black");
      colornames.push_back("blue");
      colornames.push_back("cyan");
      colornames.push_back("green");
      colornames.push_back("magenta");
      colornames.push_back("red");
      colornames.push_back("yellow");
      color=f1.at(1).toLower().trimmed();
      if(!colornames.contains(f1.at(1).toLower())) {
	*err_msg=QObject::tr("invalid --gpio argument, unrecognized color")+
	  " \""+f1.at(1)+"\"";
	return NULL;
      }

      //
      // Direction
      //
      dir=f1.at(2).at(0);
      if((f1.at(2).length()!=1)||
	 ((f1.at(2).toLower().at(0)!=QChar('i'))&&
	  (f1.at(2).toLower().at(0)!=QChar('o')))) {
	*err_msg=QObject::tr("invalid --gpio argument direction")+
	  " \""+f1.at(2)+"\".";
	return NULL;
      }

      //
      // Router
      //
      router=f1.at(3).toInt(&ok);
      if((!ok)||(router<=0)) {
	*err_msg=QObject::tr("invalid --gpio argument router");
	return NULL;
      }

      //
      // Endpoint
      //
      endpt=f1.at(4).toInt(&ok);
      if((!ok)||(endpt<=0)) {
	*err_msg=QObject::tr("invalid --gpio argument endpoint");
	return NULL;
      }

      //
      // Legend
      //
      legend=f1.at(5).trimmed();

      //
      // GPIO Mask
      //
      mask=f1.at(6).toLower().trimmed();
      if(mask.length()!=5) {
	*err_msg=
	  QObject::tr("invalid --gpio argument mask")+" \""+f1.at(6)+"\".";
	return NULL;
      }

    }

    //
    // Separator widget
    //
    if(type==GpioParser::Separator) {
      // (nothing to be done
    }

    //
    // Label widget
    //
    if(type==GpioParser::Label) {
      //
      // Legend
      //
      legend=f1.at(1).trimmed();
    }

    //
    // MultiState widget
    //
    if(type==GpioParser::MultiState) {
      //
      // Direction
      //
      dir=f1.at(1).at(0);
      if((f1.at(1).length()!=1)||
	 ((f1.at(1).toLower().at(0)!=QChar('i'))&&
	  (f1.at(1).toLower().at(0)!=QChar('o')))) {
	*err_msg=QObject::tr("invalid --gpio argument direction")+
	  " \""+f1.at(2)+"\".";
	return NULL;
      }

      //
      // Router
      //
      router=f1.at(2).toInt(&ok);
      if((!ok)||(router<=0)) {
	*err_msg=QObject::tr("invalid --gpio argument router");
	return NULL;
      }

      //
      // Endpoint
      //
      endpt=f1.at(3).toInt(&ok);
      if((!ok)||(endpt<=0)) {
	*err_msg=QObject::tr("invalid --gpio argument endpoint");
	return NULL;
      }

      //
      // Legend
      //
      legend=f1.at(4).trimmed();
    }

    types.push_back(type);
    colors.push_back(color);
    dirs.push_back(dir);
    routers.push_back(router);
    endpts.push_back(endpt);
    legends.push_back(legend);
    masks.push_back(mask);
  }

  return new GpioParser(title,types,colors,dirs,routers,endpts,legends,masks);
}


QString GpioParser::typeString(GpioParser::Type type)
{
  QString ret="unknown";

  switch(type) {
  case GpioParser::Lamp:
    ret="lamp";
    break;

  case GpioParser::Button:
    ret="button";
    break;

  case GpioParser::Separator:
    ret="sep";
    break;

  case GpioParser::Label:
    ret="label";
    break;

  case GpioParser::MultiState:
    ret="multi";
    break;

  case GpioParser::LastType:
    break;
  }

  return ret;
}

GpioParser::GpioParser(const QString &title,
		       const QList<GpioParser::Type> &types,
		       const QStringList &colors,const QList<QChar> &dirs,
		       const QList<int> &routers,const QList<int> &endpts,
		       const QStringList &legends,const QStringList &masks)
{
  c_title=title;
  c_types=types;
  c_colors=colors;
  c_directions=dirs;
  c_routers=routers;
  c_end_points=endpts;
  c_legends=legends;
  c_masks=masks;
}


bool GpioParser::TypeFromString(const QString &str,Type *type)
{
  for(int i=0;i<GpioParser::LastType;i++) {
    if(str.toLower()==GpioParser::typeString((GpioParser::Type)i)) {
      *type=(GpioParser::Type)i;
      return true;
    }
  }

  return false;
}


int GpioParser::ArgQuantityFromType(GpioParser::Type type)
{
  int ret=0;

  switch(type) {
  case GpioParser::Lamp:
  case GpioParser::Button:
    ret=7;
    break;

  case GpioParser::Separator:
    ret=1;
    break;

  case GpioParser::Label:
    ret=2;
    break;

  case GpioParser::MultiState:
    ret=5;
    break;

  case GpioParser::LastType:
    break;
  }

  return ret;
}
