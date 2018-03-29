// config.cpp
//
// Global configuration for DRouter
//
//   (C) Copyright 2018 Fred Gleason <fredg@paravelsystems.com>
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

#include <sy/syprofile.h>

#include "config.h"

Config::Config()
{
}


QString Config::lwrpPassword() const
{
  return conf_lwrp_password;
}


bool Config::configureAudioAlarms(const QString &dev_name) const
{
  for(int i=0;i<conf_no_audio_alarm_devices.size();i++) {
    if(dev_name.toLower()==conf_no_audio_alarm_devices.at(i)) {
      return false;
    }
  }
  return true;
}


void Config::load()
{
  SyProfile *p=new SyProfile();
  p->setSource(DROUTER_CONF_FILE);

  conf_lwrp_password=p->stringValue("Drouterd","LwrpPassword");
  QStringList f0=p->stringValue("Drouterd","NoAudioAlarmDevices").
    split(",",QString::SkipEmptyParts);
  for(int i=0;i<f0.size();i++) {
    conf_no_audio_alarm_devices.push_back(f0.at(i).toLower().trimmed());
  }

  delete p;
}
