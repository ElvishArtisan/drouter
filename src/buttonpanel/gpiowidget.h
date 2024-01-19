// gpiowidget.h
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

#ifndef GPIOWIDGET_H
#define GPIOWIDGET_H

#include <QLabel>
#include <QList>
#include <QStringList>

#include "gpioparser.h"
#include "jparser.h"

#define GPIOWIDGET_CELL_WIDTH 90
#define GPIOWIDGET_CELL_HEIGHT 60

class GpioWidget : public QWidget
{
  Q_OBJECT
 public:
  GpioWidget(GpioParser *gpio_parser,JParser *sa_parser,QWidget *parent=0);
  ~GpioWidget();
  QSize sizeHint() const;
  QString title() const;
  void setTitle(const QString &str);

 protected:
  void processError(const QString &err_msg);
  void resizeEvent(QResizeEvent *e);

 private slots:
  void changeConnectionState(bool state,JParser::ConnectionState cstate);

 private:
  int c_router;
  JParser *c_parser;
  QLabel *c_title_label;
  QList<QWidget *> c_widgets;
  int c_hint_width;
  int c_hint_height;
};


#endif  // GPIOWIDGET_H
