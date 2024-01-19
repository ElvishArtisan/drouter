// statelight.h
//
// Show state of a single GPIO bit.
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

#ifndef STATELIGHT_H
#define STATELIGHT_H

#include "autolabel.h"
#include "jparser.h"

#define STATELIGHT_OFF_STYLESHEET "color: #444444; background-color: #111111;"

class StateLight : public AutoLabel
{
  Q_OBJECT
 public:
  StateLight(int router,int endpt,const QString &legend,const QString &mask,
	     const QChar &dir,JParser *parser,QWidget *parent=0);
  ~StateLight();
  QSize sizeHint() const;
  QSizePolicy sizePolicy() const;
  QColor backgroundColor() const;
  void setBackgroundColor(const QColor &color);
  QColor textColor() const;
  void setTextColor(const QColor &color);

 private slots:
  void changeConnectionState(bool state,JParser::ConnectionState cstate);
  void setState(int router,int endpt,const QString &code);

 protected:
  void processError(const QString &err_msg);

 private:
  bool c_state;
  int c_router;
  int c_endpt;
  QString c_mask;
  int c_mask_bit;
  QChar c_dir;
  QColor c_text_color;
  QColor c_background_color;
  QString c_on_stylesheet;
  JParser *c_parser;
};


#endif  // STATELIGHT_H
