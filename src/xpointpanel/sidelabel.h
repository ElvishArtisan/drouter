// sidelabel.h
//
// QLabel turned on its side.
//
//   (C) Copyright 2022 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef SIDELABEL_H
#define SIDELABEL_H

#include <QLabel>

class SideLabel : public QLabel
{
  Q_OBJECT
 public:
  SideLabel(const QString &text,QWidget *parent=0);
  ~SideLabel();
  QSize sizeHint() const;
  QSizePolicy sizePolicy() const;

 protected:
  void paintEvent(QPaintEvent *e);
  void resizeEvent(QResizeEvent *e);
};


#endif  // SIDELABEL_H
