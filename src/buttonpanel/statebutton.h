// statebutton.h
//
// Set state of a single GPIO bit.
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

#ifndef STATEBUTTON_H
#define STATEBUTTON_H

#include "autopushbutton.h"
#include "drjparser.h"

class StateButton : public AutoPushButton
{
  Q_OBJECT
 public:
  StateButton(int router,int endpt,const QString &legend,const QString &mask,
	     const QChar &dir,DRJParser *parser,QWidget *parent=0);
  QSize sizeHint() const;
  QSizePolicy sizePolicy() const;
  QColor textColor() const;
  void setTextColor(const QColor &color);

 private slots:
  void changeConnectionState(bool state,DRJParser::ConnectionState cstate);
  void pressedData();
  void releasedData();

 protected:
  void processError(const QString &err_msg);

 private:
  int c_router;
  int c_endpt;
  QString c_mask;
  QString c_inverted_mask;
  int c_mask_bit;
  QChar c_dir;
  DRJParser *c_parser;
  QColor c_text_color;
};


#endif  // STATEBUTTON_H
