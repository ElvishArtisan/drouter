// config.h
//
// Global configuration for DRouter
//
//   (C) Copyright 2018-2025 Fred Gleason <fredg@paravelsystems.com>
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

#include <sy6/sygpio_server.h>

#include <drouter/drendpointmap.h>

#define DROUTER_CONF_FILE "/etc/drouter/drouter.conf"
#define DROUTER_NULL_STREAM_ADDRESS QString("239.192.0.0")
#define DROUTER_DEFAULT_CLIP_THRESHOLD -20
#define DROUTER_DEFAULT_CLIP_TIMEOUT 1000
#define DROUTER_DEFAULT_DB_KEEPALIVE_INTERVAL 900
#define DROUTER_DEFAULT_ENABLE_PROTOCOL_D true
#define DROUTER_DEFAULT_ENABLE_PROTOCOL_J true
#define DROUTER_DEFAULT_ENABLE_PROTOCOL_SA true
#define DROUTER_DEFAULT_SILENCE_THRESHOLD -500
#define DROUTER_DEFAULT_SILENCE_TIMEOUT 10000
#define DEFAULT_DEFAULT_RETAIN_EVENT_RECORDS_DURATION 168
#define DROUTER_DEFAULT_IPC_LOG_PRIORITY 7
#define DROUTER_DEFAULT_NODE_LOG_PRIORITY 7
#define DROUTER_DEFAULT_MAX_HEAP_TABLE_SIZE 33554432
#define DROUTER_DEFAULT_FILE_DESCRIPTOR_LIMIT 1024

class Config
{
 public:
  Config();
  int clipAlarmThreshold() const;
  int clipAlarmTimeout() const;
  bool enableProtocolD() const;
  bool enableProtocolJ() const;
  bool enableProtocolSA() const;
  int dbKeepaliveInterval() const;
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

  int matrixQuantity() const;
  DREndPointMap::MatrixType matrixType(int n) const;
  QHostAddress matrixHostAddress(int n) const;
  uint16_t matrixPort(int n) const;
  bool livewireIsEnabled() const;

  void load();
  static QHostAddress normalizedStreamAddress(const QHostAddress &addr);
  static QHostAddress normalizedStreamAddress(const QString &addr);
  static bool emailIsValid(const QString &addr);

 private:
  QString conf_lwrp_password;
  int conf_clip_alarm_threshold;
  int conf_clip_alarm_timeout;
  int conf_db_keepalive_interval;
  bool conf_enable_protocol_d;
  bool conf_enable_protocol_j;
  bool conf_enable_protocol_sa;
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
  QList<DREndPointMap::MatrixType> conf_matrix_types;
  QList<QHostAddress> conf_matrix_host_addresses;
  QList<uint16_t> conf_matrix_ports;
};


#endif  // CONFIG_H
