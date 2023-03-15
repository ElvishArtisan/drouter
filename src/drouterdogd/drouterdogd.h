// drouterdogd.h
//
// drouterdogd(8) Drouter watchdog monitor
//
//   (C) Copyright 2023 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
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

#ifndef DROUTERDOGD_H
#define DROUTERDOGD_H

#include <QObject>
#include <QTimer>

#include <sy5/sylwrp_client.h>

#include "config.h"
#include "saparser.h"
#include "vgpionode.h"

#define DROUTERDOGD_STEP_INTERVAL 2000
#define DROUTERDOGD_TIMEOUT_INTERVAL 6000
#define DROUTERDOGD_USAGE "[options]\n"

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  MainObject(QObject *parent=0);

 private slots:
  void nodeConnectedData(unsigned id,bool state);
  void lwrpGpiChangedData(unsigned id,int slotnum,const SyNode &node,
			  const SyGpioBundle &bundle);

  void startTest();
  void saConnectedData(bool state,SaParser::ConnectionState code);
  void saGpiChangedData(int router,int input,const QString &code);
  void stepTimeoutData();

 private:
  void ProcessTestResult(Config::WatchdogError werr);
  void StartStateChangeTest();
  QString NextTestCode(const QString &prev_code) const;
  QString GetRandomCode() const;
  SaParser *d_sa_parser;
  QTimer *d_step_timer;
  QTimer *d_timeout_timer;
  QString d_current_code;
  int d_istate;
  Config::WatchdogError d_current_error;

  VGpioNode *d_gpio_node;
  SyLwrpClient *d_lwrp_client;
  Config *d_config;
};


#endif  // DROUTERDOGD_H
