// gpiostrip.h
//
// Strip container for GPIO controls.
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

#ifndef GPIOSTRIP_H
#define GPIOSTRIP_H

#include <QLabel>
#include <QList>
#include <QStringList>

#include <drouter/drjparser.h>

#include "alertbutton.h"
#include "gpioparser.h"

#define GPIOSTRIP_CELL_WIDTH 90
#define GPIOSTRIP_CELL_HEIGHT 60

class GpioStrip : public QWidget
{
  Q_OBJECT
 public:
  GpioStrip(int id,GpioParser *gpio_parser,DRJParser *parser,QWidget *parent=0);
  ~GpioStrip();
  QSize sizeHint() const;
  QString title() const;
  void setTitle(const QString &str);
  bool summaryAlarmState() const;
  
 signals:
  void summaryAlarmStateChanged(int id,bool state,bool new_alert);
  void acknowledgeRequested();

 public slots:
  void acknowledge();

 private slots:
  void alarmStateChangedData(int id,bool state);
  void changeConnectionState(bool state,DRJParser::ConnectionState cstate);

 protected:
  void processError(const QString &err_msg);
  void resizeEvent(QResizeEvent *e);

 private:
  void LoadColorMaps();
  int c_router;
  int c_id;
  DRJParser *c_parser;
  bool c_summary_alarm_state;
  QLabel *c_title_label;
  QList<QWidget *> c_widgets;
  QList<bool> c_alarm_states;
  int c_hint_width;
  int c_hint_height;
  QMap<QString,QString> c_text_colors;
  QMap<QString,QString> c_background_colors;
};


#endif  // GPIOSTRIP_H
