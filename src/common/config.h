// config.h
//
// Global configuration for DRouter
//
//   (C) Copyright 2018-2023 Fred Gleason <fredg@paravelsystems.com>
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

#include <QHostAddress>
#include <QMap>
#include <QString>
#include <QStringList>

#include <sy5/sygpio_server.h>

#define DROUTER_CONF_FILE "/etc/drouter/drouter.conf"
#define DROUTER_NULL_STREAM_ADDRESS QString("239.192.0.0")
#define DROUTER_DEFAULT_CLIP_THRESHOLD -20
#define DROUTER_DEFAULT_CLIP_TIMEOUT 1000
#define DROUTER_DEFAULT_SILENCE_THRESHOLD -500
#define DROUTER_DEFAULT_SILENCE_TIMEOUT 10000
#define DEFAULT_DEFAULT_RETAIN_EVENT_RECORDS_DURATION 168
#define DROUTER_DEFAULT_IPC_LOG_PRIORITY -1
#define DROUTER_DEFAULT_NODE_LOG_PRIORITY -1
#define DROUTER_DEFAULT_MAX_HEAP_TABLE_SIZE 33554432
#define DROUTER_DEFAULT_FILE_DESCRIPTOR_LIMIT 1024

#define DROUTERDOGD_DEFAULT_DROUTER_ADDRESS "127.0.0.1"
#define DROUTERDOGD_DEFAULT_ROUTER_NUMBER 999
#define DROUTERDOGD_DEFAULT_GPIO_NUMBER 1
#define DROUTERDOGD_DEFAULT_SOURCE_NUMBER 31000
#define DROUTERDOGD_DEFAULT_INTERFACE_ADDRESS "127.0.0.1"
#define DROUTERDOGD_DEFAULT_INTERFACE_MASK "255.0.0.0"

#define DROUTER_TETHER_UDP_PORT 6245
#define DROUTER_TETHER_TTY_SPEED 9600
#define DROUTER_TETHER_TTY_PARITY TTYDevice::None
#define DROUTER_TETHER_TTY_WORD_LENGTH 8
#define DROUTER_TETHER_TTY_FLOW_CONTROL TTYDevice::FlowNone
#define DROUTER_TETHER_BASE_INTERVAL 10000
#define DROUTER_TETHER_WINDOW_INTERVAL 1000

class Config
{
 public:
  enum TetherRole {This=0,That=1};
  Config();
  int clipAlarmThreshold() const;
  int clipAlarmTimeout() const;
  int silenceAlarmThreshold() const;
  int silenceAlarmTimeout() const;
  bool configureAudioAlarms(const QString &dev_name) const;
  int retainEventRecordsDuration() const;
  QString alertAddress() const;
  QString fromAddress() const;
  int ipcLogPriority() const;
  int nodeLogPriority() const;
  QString lwrpPassword() const;
  int maxHeapTableSize() const;
  int fileDescriptorLimit() const;
  QStringList nodesStartupLwrp(const QHostAddress &addr) const;
  QHostAddress drouterdogdDrouterAddress() const;
  int drouterdogdRouterNumber() const;
  int drouterdogdGpioNumber() const;
  QHostAddress drouterdogdInterfaceAddress() const;
  QHostAddress drouterdogdInterfaceMask() const;
  bool drouterdogdUseInternalNode() const;
  bool tetherIsActivated() const;
  QHostAddress tetherSharedIpAddress() const;
  QString tetherHostId(TetherRole role) const;
  QString tetherHostname(TetherRole role) const;
  QHostAddress tetherIpAddress(TetherRole role) const;
  QString tetherSerialDevice(TetherRole role) const;
  QHostAddress tetherGpioIpAddress(TetherRole role) const;
  int tetherGpioSlot(TetherRole role) const;
  SyGpioBundleEvent::Type tetherGpioType(TetherRole role) const;
  QString tetherGpioCode(TetherRole role) const;
  bool tetherIsSane() const;
  void load();
  static QHostAddress normalizedStreamAddress(const QHostAddress &addr);
  static QHostAddress normalizedStreamAddress(const QString &addr);
  static bool emailIsValid(const QString &addr);

 private:
  QString conf_lwrp_password;
  int conf_clip_alarm_threshold;
  int conf_clip_alarm_timeout;
  int conf_silence_alarm_threshold;
  int conf_silence_alarm_timeout;
  int conf_ipc_log_priority;
  int conf_node_log_priority;
  int conf_retain_event_records_duration;
  QString conf_alert_address;
  QString conf_from_address;
  QStringList conf_no_audio_alarm_devices;
  int conf_max_heap_table_size;
  int conf_file_descriptor_limit;
  QMap<uint32_t,QStringList> conf_nodes_startup_lwrps;
  QHostAddress conf_drouterdogd_drouter_address;
  int conf_drouterdogd_router_number;
  int conf_drouterdogd_gpio_number;
  QHostAddress conf_drouterdogd_interface_address;
  QHostAddress conf_drouterdogd_interface_mask;
  bool conf_drouterdogd_use_internal_node;
  bool conf_tether_is_activated;
  QHostAddress conf_tether_shared_ip_address;
  QString conf_tether_host_ids[2];
  QString conf_tether_hostnames[2];
  QHostAddress conf_tether_ip_addresses[2];
  QString conf_tether_serial_devices[2];
  QHostAddress conf_tether_gpio_ip_addresses[2];
  int conf_tether_gpio_slots[2];
  SyGpioBundleEvent::Type conf_tether_gpio_types[2];
  QString conf_tether_gpio_codes[2];
  bool conf_tether_is_sane;
};


#endif  // CONFIG_H
