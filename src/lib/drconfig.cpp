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

#include "drconfig.h"

DRConfig::DRConfig()
{
  conf_tether_is_sane=false;
}


int DRConfig::clipAlarmThreshold() const
{
  return conf_clip_alarm_threshold;
}


int DRConfig::clipAlarmTimeout() const
{
  return conf_clip_alarm_timeout;
}


int DRConfig::silenceAlarmThreshold() const
{
  return conf_silence_alarm_threshold;
}


int DRConfig::silenceAlarmTimeout() const
{
  return conf_silence_alarm_timeout;
}


bool DRConfig::configureAudioAlarms(const QString &dev_name) const
{
  for(int i=0;i<conf_no_audio_alarm_devices.size();i++) {
    if(dev_name.toLower()==conf_no_audio_alarm_devices.at(i)) {
      return false;
    }
  }
  return true;
}


int DRConfig::retainEventRecordsDuration() const
{
  return conf_retain_event_records_duration;
}


QString DRConfig::alertAddress() const
{
  return conf_alert_address;
}


QString DRConfig::fromAddress() const
{
  return conf_from_address;
}


int DRConfig::ipcLogPriority() const
{
  return conf_ipc_log_priority;
}


int DRConfig::nodeLogPriority() const
{
  return conf_node_log_priority;
}


QString DRConfig::lwrpPassword() const
{
  return conf_lwrp_password;
}


int DRConfig::maxHeapTableSize() const
{
  return conf_max_heap_table_size;
}


int DRConfig::fileDescriptorLimit() const
{
  return conf_file_descriptor_limit;
}


QStringList DRConfig::nodesStartupLwrp(const QHostAddress &addr) const
{
  return conf_nodes_startup_lwrps.value(addr.toIPv4Address(),QStringList());
}


int DRConfig::matrixQuantity() const
{
  return conf_matrix_types.size();
}


DRConfig::MatrixType DRConfig::matrixType(int n) const
{
  return conf_matrix_types.at(n);
}


QHostAddress DRConfig::matrixHostAddress(int n) const
{
  return conf_matrix_host_addresses.at(n);
}


uint16_t DRConfig::matrixPort(int n) const
{
  return conf_matrix_ports.at(n);
}


bool DRConfig::livewireIsEnabled() const
{
  return conf_matrix_types.contains(DRConfig::LwrpMatrix);
}


bool DRConfig::tetherIsActivated() const
{
  return conf_tether_is_activated;
}


QHostAddress DRConfig::tetherSharedIpAddress() const
{
  return conf_tether_shared_ip_address;
}


QString DRConfig::tetherHostId(TetherRole role) const
{
  return conf_tether_host_ids[role];
}


QString DRConfig::tetherHostname(DRConfig::TetherRole role) const
{
  return conf_tether_hostnames[role];
}


QHostAddress DRConfig::tetherIpAddress(DRConfig::TetherRole role) const
{
  return conf_tether_ip_addresses[role];
}


QString DRConfig::tetherSerialDevice(DRConfig::TetherRole role) const
{
  return conf_tether_serial_devices[role];
}


QHostAddress DRConfig::tetherGpioIpAddress(DRConfig::TetherRole role) const
{
  return conf_tether_gpio_ip_addresses[role];
}


int DRConfig::tetherGpioSlot(DRConfig::TetherRole role) const
{
  return conf_tether_gpio_slots[role];
}


SyGpioBundleEvent::Type DRConfig::tetherGpioType(TetherRole role) const
{
  return conf_tether_gpio_types[role];
}


QString DRConfig::tetherGpioCode(DRConfig::TetherRole role) const
{
  return conf_tether_gpio_codes[role];
}


bool DRConfig::tetherIsSane() const
{
  return conf_tether_is_sane;
}


void DRConfig::load()
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
  conf_silence_alarm_threshold=
    p->intValue("Drouterd","SilenceAlarmThreshold",DROUTER_DEFAULT_SILENCE_THRESHOLD);
  conf_silence_alarm_timeout=
    p->intValue("Drouterd","SilenceAlarmTimeout",DROUTER_DEFAULT_SILENCE_TIMEOUT);
  QStringList f0=p->stringValue("Drouterd","NoAudioAlarmDevices").
    split(",",QString::SkipEmptyParts);
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
  DRConfig::MatrixType mtype=
    DRConfig::matrixType(p->stringValue(section,"Type","",&ok));
  while(ok) {
    conf_matrix_types.push_back(mtype);
    conf_matrix_host_addresses.
      push_back(p->addressValue(section,"HostAddress",""));
    conf_matrix_ports.push_back(p->intValue(section,"HostPort"));
    n++;
    section=QString::asprintf("Matrix%d",n+1);
    mtype=DRConfig::matrixType(p->stringValue(section,"Type","",&ok));
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
	conf_tether_host_ids[DRConfig::This]="A";
	conf_tether_host_ids[DRConfig::That]="B";
      }
      if(p->stringValue("Tether","SystemBHostname").toLower()==f0.first().toLower()) {
	conf_tether_host_ids[DRConfig::This]="B";
	conf_tether_host_ids[DRConfig::That]="A";
      }
      if(conf_tether_host_ids[DRConfig::This].isEmpty()) {
	syslog(LOG_WARNING,
	       "system name matches no configured tethered hostnames");
	conf_tether_is_sane=false;
	delete p;
	return;
      }
      syslog(LOG_DEBUG,"we are System%s",
	     (const char *)conf_tether_host_ids[DRConfig::This].toUtf8());
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


QHostAddress DRConfig::normalizedStreamAddress(const QHostAddress &addr)
{
  QHostAddress ret=addr;

  if(addr.isNull()||(addr.toString()=="255.255.255.255")) {
    ret.setAddress(DROUTER_NULL_STREAM_ADDRESS);
  }

  return ret;
}


QHostAddress DRConfig::normalizedStreamAddress(const QString &addr)
{
  return DRConfig::normalizedStreamAddress(QHostAddress(addr));
}


bool DRConfig::emailIsValid(const QString &addr)
{
  QStringList f0=addr.split("@",QString::KeepEmptyParts);

  if(f0.size()!=2) {
    return false;
  }
  QStringList f1=f0.last().split(".");
  if(f1.size()<2) {
    return false;
  }
  return true;
}


QString DRConfig::matrixTypeString(MatrixType type)
{
  QString ret="unknown";

  switch(type) {
  case DRConfig::LwrpMatrix:
    ret="LWRP";
    break;

  case DRConfig::Bt41MlrMatrix:
    ret="BT-41MLR";
    break;

  case DRConfig::Gvg7000Matrix:
    ret="GVG7000";
    break;

  case DRConfig::LastMatrix:
    break;
  }

  return ret;
}


DRConfig::MatrixType DRConfig::matrixType(const QString &str)
{
  for(int i=0;i<DRConfig::LastMatrix;i++) {
    if(str.toUpper()==DRConfig::matrixTypeString((DRConfig::MatrixType)i)) {
      return (DRConfig::MatrixType)i;
    }
  }

  return DRConfig::LastMatrix;
}
