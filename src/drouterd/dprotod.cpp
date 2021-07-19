// dprotod.cpp
//
// Protocol dispatcher for drouterd(8)
//
//   (C) Copyright 2018-2021 Fred Gleason <fredg@paravelsystems.com>
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
#include <linux/un.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <QCoreApplication>

#include <sy5/sycmdswitch.h>

#include "dprotod.h"
#include "protocol_d.h"
#include "protocol_sa.h"
#include "protoipc.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  main_protocol=NULL;
  main_protocol_d=false;
  main_protocol_sa=false;
  bool systemd=false;
  int protocols_defined=0;
  QString err_msg;

  SyCmdSwitch *cmd=new SyCmdSwitch("dprotod",VERSION,DPROTOD_USAGE);
  for(int i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--protocol-d") {
      main_protocol_d=true;
      protocols_defined++;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--protocol-sa") {
      main_protocol_sa=true;
      protocols_defined++;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--systemd") {
      systemd=true;
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"dprotod: unrecognized option\n");
      exit(1);
    }
  }
  if(protocols_defined==0) {
    fprintf(stderr,"dprotod: no --protocol specified\n");
    exit(1);
  }
  if(protocols_defined>1) {
    fprintf(stderr,
	    "dprotod: only one --protocol may be specified per instance\n");
    exit(1);
  }

  if(systemd) {
    if(!StartIpc(&err_msg)) {
      fprintf(stderr,"dprotod: %s\n",(const char *)err_msg.toUtf8());
      exit(1);
    }
  }
  else {
    if(main_protocol_d) {
      main_protocol=new ProtocolD(-1,this);
    }
    if(main_protocol_sa) {
      main_protocol=new ProtocolSa(-1,this);
    }
  }
}


bool MainObject::StartIpc(QString *err_msg)
{
  int sock;
  struct sockaddr_un sa;
  struct msghdr msg;
  struct cmsghdr *cmsg=NULL;
  char buf[CMSG_SPACE(sizeof(int))];
  //  int *fdptr;
  int proto_sock=-1;

  //
  // Connect to DRouter process
  //
  if((sock=socket(AF_UNIX,SOCK_SEQPACKET,0))<0) {
    *err_msg=QString("unable to start protocol ipc [")+strerror(errno)+"]";
    return false;
  }
  memset(&sa,0,sizeof(sa));
  sa.sun_family=AF_UNIX;
  strncpy(sa.sun_path+1,DROUTER_IPC_ADDRESS,UNIX_PATH_MAX-1);
  if(::connect(sock,(struct sockaddr *)(&sa),sizeof(sa))<0) {
    *err_msg=QString("unable to attach to drouter service [")+
      strerror(errno)+"]";
    return false;
  }
  if(main_protocol_d) {
    write(sock,"SEND_D_SOCK\r\n",13);
  }
  if(main_protocol_sa) {
    write(sock,"SEND_SA_SOCK\r\n",14);
  }

  //
  // Receive socket
  //
  memset(&msg,0,sizeof(msg));
  msg.msg_control=buf;
  msg.msg_controllen=sizeof(buf);
  cmsg=CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level=SOL_SOCKET;
  cmsg->cmsg_type=SCM_RIGHTS;
  cmsg->cmsg_len=CMSG_LEN(sizeof(int));
  msg.msg_controllen=cmsg->cmsg_len;
  if(recvmsg(sock,&msg,0)<0) {
    *err_msg=strerror(errno);
    return false;
  }
  proto_sock=((int *)(CMSG_DATA(cmsg)))[0];

  if(main_protocol_d) {
    main_protocol=new ProtocolD(proto_sock,this);
  }
  if(main_protocol_sa) {
    main_protocol=new ProtocolSa(proto_sock,this);
  }
  close(sock);

  return true;
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);

  new MainObject();
  return a.exec();
}
