// soundplayer.h
//
// Simple audio file player
//
//   (C) Copyright 2025 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef SOUNDPLAYER_H
#define SOUNDPLAYER_H

#include <stdio.h>

#include <QObject>
#include <QTimer>

#include <portaudio.h>
#include <sndfile.h>

class SoundPlayer : public QObject
{
  Q_OBJECT
 public:
  SoundPlayer(int snd_dev,const QStringList &file_paths,QObject *parent);
  ~SoundPlayer();
  int soundDevice() const;
  QStringList filePaths() const;
  QString playingFilename() const;
  bool isPlaying() const;
  bool play(const QString &filename,bool loop,QString *err_msg);
  void stop();
  int playCount() const;
  
 signals:
  void started();
  void stopped();

 private slots:
  void stopTimeoutData();   

 private:
  QString d_playing_filename;
  PaDeviceIndex d_sound_device;
  QStringList d_file_paths;
  PaStream *d_pa_stream;
  SNDFILE *d_sf_sndfile;
  SF_INFO d_sf_info;
  bool d_loop;
  friend int __SoundPlayer__PaStreamCallback(const void *input,void *output,
					     unsigned long frames,
					     const PaStreamCallbackTimeInfo *timeinfo,
					     PaStreamCallbackFlags flags,
					     void *user_data);
  bool d_cb_playing;
  QTimer *d_stop_timer;
  int d_play_count;
};


#endif  // SOUNDPLAYER_H
