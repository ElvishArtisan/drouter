// autolabel.h
//
// Label widget with automatic text sizing
//
// (C) 2017-2020 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef AUTOLABEL_H
#define AUTOLABEL_H

#include <QLabel>

class AutoLabel : public QLabel
{
  Q_OBJECT;
 public:
  AutoLabel(QWidget *parent=0);
  double aspectScale() const;
  void setAspectScale(double ratio);
  void setFontFamily(const QString &str);
  int minimumPointSize() const;
  void setMinimumPointSize(int size);

 public slots:
  void setText(const QString &str);
  void show();

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  void ComposeText(); 
  QString auto_plain_text;
  QString auto_font_family;
  int auto_minimum_point_size;
  double auto_aspect_scale;
};


#endif  // AUTOLABEL_H
