// config.h
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

#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QStringList>

#define DROUTER_CONF_FILE "/etc/drouter.d/drouter.conf"
#define DROUTER_DEFAULT_CLIP_THRESHOLD -20
#define DROUTER_DEFAULT_CLIP_TIMEOUT 1000
#define DROUTER_DEFAULT_SILENCE_THRESHOLD -500
#define DROUTER_DEFAULT_SILENCE_TIMEOUT 10000

class Config
{
 public:
  Config();
  int clipAlarmThreshold() const;
  int clipAlarmTimeout() const;
  int silenceAlarmThreshold() const;
  int silenceAlarmTimeout() const;
  bool configureAudioAlarms(const QString &dev_name) const;
  QString lwrpPassword() const;
  void load();

 private:
  QString conf_lwrp_password;
  int conf_clip_alarm_threshold;
  int conf_clip_alarm_timeout;
  int conf_silence_alarm_threshold;
  int conf_silence_alarm_timeout;
  QStringList conf_no_audio_alarm_devices;
};


#endif  // CONFIG_H
