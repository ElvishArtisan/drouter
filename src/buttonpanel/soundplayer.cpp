// soundplayer.cpp
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

#include "soundplayer.h"

int __SoundPlayer__PaStreamCallback(const void *input,void *output,
				    unsigned long frames,
				    const PaStreamCallbackTimeInfo *timeinfo,
				    PaStreamCallbackFlags flags,void *user_data)
{
  SoundPlayer *player=(SoundPlayer *)user_data;
  unsigned long n;

  memset((short *)output,0,frames*player->d_sf_info.channels*sizeof(short));
  if((n=sf_readf_short(player->d_sf_sndfile,(short *)output,frames))<frames) {
    if(player->d_loop) {
      sf_seek(player->d_sf_sndfile,0,SEEK_SET);
      sf_readf_short(player->d_sf_sndfile,
		     ((short *)output)+n*player->d_sf_info.channels,frames-n);
    }
    else {
      player->d_cb_playing=false;
    }
  }
  return paContinue;
}


SoundPlayer::SoundPlayer(int snd_dev,const QStringList &file_paths,
			 QObject *parent)
  : QObject(parent)
{
  d_sf_sndfile=NULL;
  d_sf_info.format=0;
  d_sound_device=snd_dev;
  d_file_paths=file_paths;
  d_pa_stream=NULL;
  d_loop=false;
  d_play_count=0;
  
  d_stop_timer=new QTimer(this);
  connect(d_stop_timer,SIGNAL(timeout()),this,SLOT(stopTimeoutData()));
}


SoundPlayer::~SoundPlayer()
{
  if(d_sf_sndfile!=NULL) {
    sf_close(d_sf_sndfile);
  }
}


int SoundPlayer::soundDevice() const
{
  return d_sound_device;
}


QStringList SoundPlayer::filePaths() const
{
  return d_file_paths;
}


QString SoundPlayer::playingFilename() const
{
  return d_playing_filename;
}


bool SoundPlayer::isPlaying() const
{
  return d_sf_sndfile!=NULL;
}


bool SoundPlayer::play(const QString &filename,bool loop,QString *err_msg)
{
  PaError pa_err=0;
  PaStreamParameters params;
  QStringList sound_dirs;
  d_loop=loop;

  //
  // Ref Count
  //
  d_play_count++;
  if(isPlaying()) {
    return true;
  }

  //
  // Open Sound File
  //
  for(int i=0;i<d_file_paths.size();i++) {
    QString pathname=d_file_paths.at(i)+"/"+filename;
    if((d_sf_sndfile=sf_open(pathname.toUtf8(),SFM_READ,&d_sf_info))!=NULL) {
      d_playing_filename=pathname;
      break;
    }
  }
  if(d_sf_sndfile==NULL) {
    *err_msg=sf_strerror(NULL);
    return false;
  }

  //
  // Open Sound Device
  //
  params.device=d_sound_device;
  params.channelCount=d_sf_info.channels;
  params.sampleFormat=paInt16;
  params.suggestedLatency=1.0;
  params.hostApiSpecificStreamInfo=NULL;
  if((pa_err=Pa_OpenStream(&d_pa_stream,NULL,&params,d_sf_info.samplerate,0,
		 paNoFlag,__SoundPlayer__PaStreamCallback,this))!=paNoError) {
    *err_msg=Pa_GetErrorText(pa_err);
    return false;
  }
  d_cb_playing=true;
  if((pa_err=Pa_StartStream(d_pa_stream))!=paNoError) {
    *err_msg=Pa_GetErrorText(pa_err);
    Pa_CloseStream(d_pa_stream);
    sf_close(d_sf_sndfile);
    d_sf_sndfile=NULL;
    d_sf_info.format=0;
    d_cb_playing=false;
    return false;
  }
  d_stop_timer->start(10);
  emit started();

  return true;
}


void SoundPlayer::stop()
{
  if(--d_play_count>0) {
    return;
  }
  if(d_pa_stream!=NULL) {
    Pa_StopStream(d_pa_stream);
    Pa_CloseStream(d_pa_stream);
    d_pa_stream=NULL;
  }
  if(d_sf_sndfile!=NULL) {
    sf_close(d_sf_sndfile);
    d_sf_sndfile=NULL;
    d_sf_info.format=0;
    emit stopped();
  }
  d_play_count=0;
}


int SoundPlayer::playCount() const
{
  return d_play_count;
}


void SoundPlayer::stopTimeoutData()
{
  if(!d_cb_playing) {
    d_stop_timer->stop();
    stop();
  }
}
