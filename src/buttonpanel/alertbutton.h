// alertbutton.h
//
// Pushbutton for the alert widget
//
//   (C) Copyright 2020-2025 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef ALERTBUTTON_H
#define ALERTBUTTON_H

#include <drouter/drjparser.h>

#include "autopushbutton.h"
#include "soundplayer.h"

#define ALERTBUTTON_OFF_STYLESHEET ""

class AlertButton : public AutoPushButton
{
  Q_OBJECT
 public:
  AlertButton(int id,int router,int endpt,const QString &legend,
	      const QString &mask,QWidget *parent=0);
  QSize sizeHint() const;
  QSizePolicy sizePolicy() const;
  QColor activeColor() const;
  void setActiveColors(const QColor &text,const QColor &backgnd);
  bool alarmIsActive();

 signals:
  void alarmStateChanged(int id,bool state);

 private slots:
  void clickedData();
  void changeConnectionState(bool state,DRJParser::ConnectionState cstate);
  void setState(int router,int endpt,const QString &code);
  void setAlarmState(bool state);
  void flashData();
  
 protected:
  void processError(const QString &err_msg);

 private:
  int c_id;
  int c_router;
  int c_endpt;
  QString c_mask;
  QString c_inverted_mask;
  int c_mask_bit;
  QColor c_text_color;
  QColor c_active_color;
  QString c_stylesheets[2];
  QTimer *c_flash_timer;
  bool c_alarm_state;
};


#endif  // ALERTBUTTON_H
