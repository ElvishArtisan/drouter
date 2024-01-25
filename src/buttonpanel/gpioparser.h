// gpioparser.h
//
// Parse GPIO widget arguments
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

#ifndef GPIOPARSER_H
#define GPIOPARSER_H

#include <QChar>
#include <QList>
#include <QStringList>

#include "drsaparser.h"

class GpioParser
{
 public:
  enum Type {Lamp=0,Button=1,Separator=2,Label=3,MultiState=4,LastType=5};
  QString title() const;
  int widgetQuantity() const;
  Type type(int n) const;
  QString color(int n) const;
  QChar direction(int n) const;
  int router(int n) const;
  int endPoint(int n) const;
  QString legend(int n) const;
  QString mask(int n) const;
  static GpioParser *fromString(const QString &str,QString *err_msg);
  static QString typeString(Type type);

 private:
  GpioParser(const QString &title,const QList<Type> &types,
	     const QStringList &colors,const QList<QChar> &dirs,
	     const QList<int> &routers,const QList<int> &endpts,
	     const QStringList &legends,const QStringList &masks);
  static bool TypeFromString(const QString &str,Type *type);
  static int ArgQuantityFromType(Type type);
  QString c_title;
  QList<Type> c_types;
  QStringList c_colors;
  QList<QChar> c_directions;
  QList<int> c_routers;
  QList<int> c_end_points;
  QStringList c_legends;
  QStringList c_masks;
};


#endif  // GPIOPARSER_H
