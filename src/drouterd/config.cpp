// drconfig.cpp
//
// Global configuration for DRouter
//
//   (C) Copyright 2018-2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#include <QStringList>

#include <sy5/syprofile.h>

#include "config.h"

Config::Config()
{
  conf_tether_is_sane=false;
}


int Config::clipAlarmThreshold() const
{
  return conf_clip_alarm_threshold;
}


int Config::clipAlarmTimeout() const
{
  return conf_clip_alarm_timeout;
}


int Config::dbKeepaliveInterval() const
{
  return conf_db_keepalive_interval;
}


bool Config::enableProtocolD() const
{
  return conf_enable_protocol_d;
}


bool Config::enableProtocolJ() const
{
  return conf_enable_protocol_j;
}


bool Config::enableProtocolSA() const
{
  return conf_enable_protocol_sa;
}


int Config::silenceAlarmThreshold() const
{
  return conf_silence_alarm_threshold;
}


int Config::silenceAlarmTimeout() const
{
  return conf_silence_alarm_timeout;
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


int Config::retainEventRecordsDuration() const
{
  return conf_retain_event_records_duration;
}


QString Config::alertAddress() const
{
  return conf_alert_address;
}


QString Config::fromAddress() const
{
  return conf_from_address;
}


int Config::ipcLogPriority() const
{
  return conf_ipc_log_priority;
}


int Config::nodeLogPriority() const
{
  return conf_node_log_priority;
}


QString Config::lwrpPassword() const
{
  return conf_lwrp_password;
}


int Config::maxHeapTableSize() const
{
  return conf_max_heap_table_size;
}


int Config::fileDescriptorLimit() const
{
  return conf_file_descriptor_limit;
}


QStringList Config::nodesStartupLwrp(const QHostAddress &addr) const
{
  return conf_nodes_startup_lwrps.value(addr.toIPv4Address(),QStringList());
}


int Config::matrixQuantity() const
{
  return conf_matrix_types.size();
}


Config::MatrixType Config::matrixType(int n) const
{
  return conf_matrix_types.at(n);
}


QHostAddress Config::matrixHostAddress(int n) const
{
  return conf_matrix_host_addresses.at(n);
}


uint16_t Config::matrixPort(int n) const
{
  return conf_matrix_ports.at(n);
}


bool Config::livewireIsEnabled() const
{
  return conf_matrix_types.contains(Config::LwrpMatrix);
}


bool Config::tetherIsActivated() const
{
  return conf_tether_is_activated;
}


QHostAddress Config::tetherSharedIpAddress() const
{
  return conf_tether_shared_ip_address;
}


QString Config::tetherHostId(TetherRole role) const
{
  return conf_tether_host_ids[role];
}


QString Config::tetherHostname(Config::TetherRole role) const
{
  return conf_tether_hostnames[role];
}


QHostAddress Config::tetherIpAddress(Config::TetherRole role) const
{
  return conf_tether_ip_addresses[role];
}


QString Config::tetherSerialDevice(Config::TetherRole role) const
{
  return conf_tether_serial_devices[role];
}


QHostAddress Config::tetherGpioIpAddress(Config::TetherRole role) const
{
  return conf_tether_gpio_ip_addresses[role];
}


int Config::tetherGpioSlot(Config::TetherRole role) const
{
  return conf_tether_gpio_slots[role];
}


SyGpioBundleEvent::Type Config::tetherGpioType(TetherRole role) const
{
  return conf_tether_gpio_types[role];
}


QString Config::tetherGpioCode(Config::TetherRole role) const
{
  return conf_tether_gpio_codes[role];
}


bool Config::tetherIsSane() const
{
  return conf_tether_is_sane;
}


void Config::load()
{
  char hostname[HOST_NAME_MAX];
  SyProfile *p=new SyProfile();
  bool ok=false;
  p->setSource(DROUTER_CONF_FILE);

  //
  // [Drouterd] Section
  //
  conf_clip_alarm_threshold=
    p->intValue("Drouterd","ClipAlarmThreshold",DROUTER_DEFAULT_CLIP_THRESHOLD);
  conf_clip_alarm_timeout=
    p->intValue("Drouterd","ClipAlarmTimeout",DROUTER_DEFAULT_CLIP_TIMEOUT);
  conf_db_keepalive_interval=
    p->intValue("Drouterd","DbKeepaliveInterval",
		DROUTER_DEFAULT_DB_KEEPALIVE_INTERVAL);
  conf_enable_protocol_d=p->boolValue("Drouterd","EnableProtocolD",
				      DROUTER_DEFAULT_ENABLE_PROTOCOL_D);
  conf_enable_protocol_j=p->boolValue("Drouterd","EnableProtocolJ",
				      DROUTER_DEFAULT_ENABLE_PROTOCOL_J);
  conf_enable_protocol_sa=p->boolValue("Drouterd","EnableProtocolSA",
				      DROUTER_DEFAULT_ENABLE_PROTOCOL_SA);
  conf_silence_alarm_threshold=
    p->intValue("Drouterd","SilenceAlarmThreshold",
		DROUTER_DEFAULT_SILENCE_THRESHOLD);
  conf_silence_alarm_timeout=
    p->intValue("Drouterd","SilenceAlarmTimeout",
		DROUTER_DEFAULT_SILENCE_TIMEOUT);
  QStringList f0=p->stringValue("Drouterd","NoAudioAlarmDevices").
    split(",",Qt::SkipEmptyParts);
  for(int i=0;i<f0.size();i++) {
    conf_no_audio_alarm_devices.push_back(f0.at(i).toLower().trimmed());
  }
  conf_retain_event_records_duration=
    p->intValue("Drouterd","RetainEventRecordsDuration",
		DEFAULT_DEFAULT_RETAIN_EVENT_RECORDS_DURATION);
  conf_alert_address=p->stringValue("Drouterd","AlertAddress");
  conf_from_address=p->stringValue("Drouterd","FromAddress");
  conf_ipc_log_priority=
    p->intValue("Drouterd","IpcLogPriority",DROUTER_DEFAULT_IPC_LOG_PRIORITY);
  conf_node_log_priority=
    p->intValue("Drouterd","NodeLogPriority",DROUTER_DEFAULT_NODE_LOG_PRIORITY);
  conf_lwrp_password=p->stringValue("Drouterd","LwrpPassword");

  conf_tether_is_activated=p->boolValue("Tether","IsActivated",false);

  conf_max_heap_table_size=p->intValue("Drouterd","MaxHeapTableSize",
				       DROUTER_DEFAULT_MAX_HEAP_TABLE_SIZE);
  conf_file_descriptor_limit=p->intValue("Drouterd","FileDescriptorLimit",
					 DROUTER_DEFAULT_FILE_DESCRIPTOR_LIMIT);

  //
  // [Nodes] Section
  //
  int n=0;
  QHostAddress host_addr=
    p->addressValue("Nodes",QString::asprintf("HostAddress%d",n+1),"",&ok);
  while(ok) {
    QString lwrp=
      p->stringValue("Nodes",QString::asprintf("StartupLwrp%d",n+1),"",&ok).
      trimmed();
    if(!lwrp.isEmpty()) {
      QStringList f0=lwrp.split(",");
      for(int i=0;i<f0.size();i++) {
	f0[i]=f0.at(i).trimmed();
      }
      conf_nodes_startup_lwrps[host_addr.toIPv4Address()]=f0;
    }
    n++;
    host_addr=
      p->addressValue("Nodes",QString::asprintf("HostAddress%d",n+1),"",&ok);
  }

  //
  // [Matrix<n>] Sections
  //
  n=0;
  QString section=QString::asprintf("Matrix%d",n+1);
  Config::MatrixType mtype=
    Config::matrixType(p->stringValue(section,"Type","",&ok));
  while(ok) {
    conf_matrix_types.push_back(mtype);
    conf_matrix_host_addresses.
      push_back(p->addressValue(section,"HostAddress",""));
    conf_matrix_ports.push_back(p->intValue(section,"HostPort"));
    n++;
    section=QString::asprintf("Matrix%d",n+1);
    mtype=Config::matrixType(p->stringValue(section,"Type","",&ok));
  }

  //
  // [Tether] Section
  //
  if(conf_tether_is_activated) {
    conf_tether_shared_ip_address.
      setAddress(p->stringValue("Tether","SharedIpAddress"));
    if(gethostname(hostname,HOST_NAME_MAX)==0) {
      QStringList f0=QString(hostname).split(".");
      if(p->stringValue("Tether","SystemAHostname").toLower()==f0.first().toLower()) {
	conf_tether_host_ids[Config::This]="A";
	conf_tether_host_ids[Config::That]="B";
      }
      if(p->stringValue("Tether","SystemBHostname").toLower()==f0.first().toLower()) {
	conf_tether_host_ids[Config::This]="B";
	conf_tether_host_ids[Config::That]="A";
      }
      if(conf_tether_host_ids[Config::This].isEmpty()) {
	syslog(LOG_WARNING,
	       "system name matches no configured tethered hostnames");
	conf_tether_is_sane=false;
	delete p;
	return;
      }
      syslog(LOG_DEBUG,"we are System%s",
	     (const char *)conf_tether_host_ids[Config::This].toUtf8());
      conf_tether_is_sane=!conf_tether_shared_ip_address.toString().isEmpty();
      for(int i=0;i<2;i++) {
	conf_tether_hostnames[i]=
	  p->stringValue("Tether","System"+conf_tether_host_ids[i]+"Hostname");
	conf_tether_is_sane=
	  conf_tether_is_sane&&(!conf_tether_hostnames[i].isEmpty());
	conf_tether_ip_addresses[i].
	  setAddress(p->stringValue("Tether","System"+conf_tether_host_ids[i]+
				    "IpAddress"));
	conf_tether_is_sane=
	  conf_tether_is_sane&&(!conf_tether_ip_addresses[i].toString().
				isEmpty());
	conf_tether_serial_devices[i]=
	  p->stringValue("Tether","System"+conf_tether_host_ids[i]+
			 "SerialDevice");
	conf_tether_is_sane=
	  conf_tether_is_sane&&(!conf_tether_serial_devices[i].isEmpty());
	conf_tether_gpio_ip_addresses[i].
	  setAddress(p->stringValue("Tether","System"+conf_tether_host_ids[i]+"GpioIpAddress"));
	conf_tether_gpio_slots[i]=
	  p->intValue("Tether","System"+conf_tether_host_ids[i]+"GpioSlot")-1;
	if(p->stringValue("Tether","System"+conf_tether_host_ids[i]+"GpioType").
	   toUpper()=="GPO") {
	  conf_tether_gpio_types[i]=SyGpioBundleEvent::TypeGpo;
	}
	else {
	  conf_tether_gpio_types[i]=SyGpioBundleEvent::TypeGpi;
	}
	conf_tether_gpio_codes[i]=
	  p->stringValue("Tether","System"+conf_tether_host_ids[i]+"GpioCode",
			 "xxxxx");
      }
      if(!conf_tether_is_sane) {
	syslog(LOG_WARNING,
	       "tether data is not sane, disabling cluster support");
      }
    }
    else {
      syslog(LOG_WARNING,"unable to read hostname [%s]",strerror(errno));
      conf_tether_is_sane=false;
    }
    
    delete p;
  }
  else {
    conf_tether_is_sane=false;
    syslog(LOG_INFO,"tethering is deactivated");
  }
}


QHostAddress Config::normalizedStreamAddress(const QHostAddress &addr)
{
  QHostAddress ret=addr;

  if(addr.isNull()||(addr.toString()=="255.255.255.255")) {
    ret.setAddress(DROUTER_NULL_STREAM_ADDRESS);
  }

  return ret;
}


QHostAddress Config::normalizedStreamAddress(const QString &addr)
{
  return Config::normalizedStreamAddress(QHostAddress(addr));
}


bool Config::emailIsValid(const QString &addr)
{
  QStringList f0=addr.split("@",Qt::KeepEmptyParts);

  if(f0.size()!=2) {
    return false;
  }
  QStringList f1=f0.last().split(".");
  if(f1.size()<2) {
    return false;
  }
  return true;
}


QString Config::matrixTypeString(MatrixType type)
{
  QString ret="unknown";

  switch(type) {
  case Config::LwrpMatrix:
    ret="LWRP";
    break;

  case Config::Gvg7000Matrix:
    ret="GVG7000";
    break;

  case Config::LastMatrix:
    break;
  }

  return ret;
}


Config::MatrixType Config::matrixType(const QString &str)
{
  for(int i=0;i<Config::LastMatrix;i++) {
    if(str.toUpper()==Config::matrixTypeString((Config::MatrixType)i)) {
      return (Config::MatrixType)i;
    }
  }

  return Config::LastMatrix;
}
