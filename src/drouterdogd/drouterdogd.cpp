// drouterdogd.cpp
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

#include <stdlib.h>

#include <QCoreApplication>

#include "drouterdogd.h"

// #define DROUTERDOGD_DEBUG 1

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  d_current_code="hhhhh";
  d_sa_parser=NULL;
  d_gpio_node=NULL;

  //
  // Initialize Random Number Generator
  //
  srandom(QDateTime::currentDateTime().toTime_t());

  //
  // Read Configuration
  //
  d_config=new Config();
  d_config->load();

  //
  // GPIO Node to Test With
  //
  if(d_config->drouterdogdUseInternalNode()) {
    d_gpio_node=new VGpioNode(1,d_config->drouterdogdInterfaceAddress(),
			      d_config->drouterdogdInterfaceMask(),this);
  }

  //
  // LWRP Connection to Test Node
  //
  d_lwrp_client=new SyLwrpClient(0,this);
  connect(d_lwrp_client,SIGNAL(connected(unsigned,bool)),
	  this,SLOT(nodeConnectedData(unsigned,bool)));
  connect(d_lwrp_client,
	  SIGNAL(gpiChanged(unsigned,int,const SyNode &,const SyGpioBundle &)),
	  this,SLOT(lwrpGpiChangedData(unsigned,int,const SyNode &,
				       const SyGpioBundle &)));
  d_lwrp_client->connectToHost(QHostAddress("127.0.0.1"),SWITCHYARD_LWRP_PORT,
			       d_config->lwrpPassword());

  //
  // Test System
  //
  d_step_timer=new QTimer(this);
  d_step_timer->setSingleShot(true);
  connect(d_step_timer,SIGNAL(timeout()),this,SLOT(startTest()));

  d_timeout_timer=new QTimer(this);
  d_timeout_timer->setSingleShot(true);
  connect(d_timeout_timer,SIGNAL(timeout()),this,SLOT(stepTimeoutData()));

  d_step_timer->start(DROUTERDOGD_STEP_INTERVAL);
}


void MainObject::nodeConnectedData(unsigned id,bool state)
{
#ifdef DROUTERDOGD_DEBUG
  printf("nodeConnectedData(%u,%u)\n",id,state);
#endif  // DROUTERDOGD_DEBUG
}


void MainObject::lwrpGpiChangedData(unsigned id,int slotnum,const SyNode &node,
				    const SyGpioBundle &bundle)
{
#ifdef DROUTERDOGD_DEBUG
  printf("gpiChangedData(%u,%d,%s,%s)\n",id,slotnum,
	 node.hostAddress().toString().toUtf8().constData(),
	 bundle.code().toUtf8().constData());
#endif  // DROUTERDOGD_DEBUG
}


void MainObject::startTest()
{
#ifdef DROUTERDOGD_DEBUG
  printf("startTest()\n");
#endif  // DROUTERDOGD_DEBUG

  d_timeout_timer->stop();

  //
  // Connect to Drouter
  //
  d_istate=0;  // Start Test

  if(d_sa_parser!=NULL) {
    delete d_sa_parser;
    d_sa_parser=NULL;
  }
  d_sa_parser=new SaParser(this);
  connect(d_sa_parser,SIGNAL(connected(bool,SaParser::ConnectionState)),
	  this,SLOT(saConnectedData(bool,SaParser::ConnectionState)));
  connect(d_sa_parser,SIGNAL(gpiStateChanged(int,int,const QString &)),
	  this,SLOT(saGpiChangedData(int,int,const QString &)));
  d_sa_parser->connectToHost(d_config->drouterdogdDrouterAddress().toString(),
			     9500,"drouterdogd","");
  d_timeout_timer->start(DROUTERDOGD_TIMEOUT_INTERVAL);
}


void MainObject::saConnectedData(bool state,SaParser::ConnectionState code)
{
#ifdef DROUTERDOGD_DEBUG
  printf("saConnected(%u,%u)\n",state,code);
#endif  // DROUTERDOGD_DEBUG

  if((d_istate==0)&&state) {
    StartStateChangeTest();
  }
}


void MainObject::saGpiChangedData(int router,int input,const QString &code)
{
#ifdef DROUTERDOGD_DEBUG
  printf("saGpiChangedData(%d,%d,%s)\n",router,input,code.toUtf8().constData());
#endif  // DROUTERDOGD_DEBUG

  if((router==d_config->drouterdogdRouterNumber())&&
     (input==d_config->drouterdogdGpioNumber())) {
    if(code==d_current_code) {  // Success
      printf("%s: PASS!\n",
	     QTime::currentTime().toString("hh:mm:ss").toUtf8().constData());
    }
    else {
      if(d_istate<2) {
	d_istate++;
	StartStateChangeTest();
	return;
      }
      else {
	printf("%s: GPIO State Change Failed - Wrong Pattern Returned!\n",
	       QTime::currentTime().toString("hh:mm:ss").toUtf8().constData());
      }
    }
    d_step_timer->start(DROUTERDOGD_STEP_INTERVAL);
  }
}


void MainObject::stepTimeoutData()
{
#ifdef DROUTERDOGD_DEBUG
  printf("stepTimeoutData()\n");
#endif  // DROUTERDOGD_DEBUG

  switch(d_istate) {
  case 0:
    printf("%s: Service Login Failed!\n",
	   QTime::currentTime().toString("hh:mm:ss").toUtf8().constData());
    d_step_timer->start(DROUTERDOGD_STEP_INTERVAL);  // Restart the test
    break;

  case 1:
    StartStateChangeTest();  // Try again
    break;

  case 2:
    printf("%s: GPIO State Change Failed - No Response!\n",
	   QTime::currentTime().toString("hh:mm:ss").toUtf8().constData());
    d_step_timer->start(DROUTERDOGD_STEP_INTERVAL);  // Restart the test
    break;
  }
}


void MainObject::StartStateChangeTest()
{
  d_timeout_timer->stop();
  d_istate++;
  d_current_code=NextTestCode(d_current_code);
  d_sa_parser->setGpiState(d_config->drouterdogdRouterNumber(),
			   d_config->drouterdogdGpioNumber(),
			   d_current_code);
  d_timeout_timer->start(DROUTERDOGD_TIMEOUT_INTERVAL);
}


QString MainObject::NextTestCode(const QString &prev_code) const
{
  QString code=GetRandomCode();

  while(code==prev_code) {
    code=GetRandomCode();
  }

  return code;
}


QString MainObject::GetRandomCode() const
{
  QString code="";

  for(int i=0;i<SWITCHYARD_GPIO_BUNDLE_SIZE;i++) {
    if(random()>(RAND_MAX/2)) {
      code+="h";
    }
    else {
      code+="l";
    }
  }

  return code;
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();
  return a.exec();
}
