// multistatelabel.h
//
// MultiStateWidget with a text label
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

#ifndef MULTISTATELABEL_H
#define MULTISTATELABEL_H

#include "autolabel.h"
#include "drmultistatewidget.h"

class MultiStateLabel : public QWidget
{
  Q_OBJECT
 public:
  MultiStateLabel(int router,int linenum,const QString &legend,
		  QWidget *parent=0);
  QSize sizeHint() const;
  QString state() const;

 public slots:
  void setState(int router,int linenum,const QString &code);

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  AutoLabel *c_label;
  DRMultiStateWidget *c_widget;
};


#endif  // MULTISTATELABEL_H
