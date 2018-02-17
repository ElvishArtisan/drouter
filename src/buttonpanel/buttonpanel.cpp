// buttonpanel.cpp
//
// Button applet for controlling an lwpath output.
//
//   (C) Copyright 2002-2017 Fred Gleason <fredg@paravelsystems.com>
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

#include <QApplication>
#include <QMessageBox>

#include <sy/sycmdswitch.h>

#include "buttonpanel.h"

//
// Icons
//
//#include "../../icons/lwpath-16x16.xpm"

MainWidget::MainWidget(QWidget *parent)
  : QWidget(parent)
{
  panel_columns=0;
  panel_rows=1;
  panel_router=1;
  panel_hostname="localhost";

  bool ok=false;

  //
  // Read Command Options
  //
  SyCmdSwitch *cmd=
    new SyCmdSwitch(qApp->argc(),qApp->argv(),"buttonpanel",VERSION,
				   BUTTONPANEL_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--columns") {
      panel_columns=cmd->value(i).toUInt(&ok);
      if(!ok) {
	QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),
			     tr("Invalid --columns value specified!"));
	exit(1);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--hostname") {
      panel_hostname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--username") {
      panel_username=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--password") {
      panel_password=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--output") {  // Defer processing on these
      panel_router=1;
      QStringList f0=cmd->value(i).split(":");
      if(f0.size()>2) {
	QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),
			     tr("Invalid output")+" \""+cmd->value(i)+
			     "\" specified!");
	exit(1);
      }
      if(f0.size()==2) {
	panel_router=f0.at(0).toInt(&ok);
	if((!ok)||(panel_router<1)) {
	  QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),
			       tr("Invalid router specified!"));
	  exit(1);
	}
      }
      panel_output=f0.back().toInt(&ok)-1;
      if((!ok)||(panel_output<0)) {
	QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),
			     tr("Invalid output specified!"));
	exit(1);
      }      
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      QMessageBox::warning(this,"Buttonpanel - "+tr("Error"),
			   tr("Unknown option")+": "+cmd->key(i)+"!");
      exit(256);
    }
  }

  //
  // Create And Set Icon
  //
  //  setWindowIcon(QPixmap(lwpath_16x16_xpm));

  //
  // The SA Connection
  //
  panel_parser=new SaParser(this);
  connect(panel_parser,SIGNAL(connected(bool,SaParser::ConnectionState)),
	  this,SLOT(changeConnectionState(bool,SaParser::ConnectionState)));
  connect(panel_parser,
	  SIGNAL(outputCrosspointChanged(int,int,int)),
	  this,SLOT(changeOutputCrosspoint(int,int,int)));
  panel_button_mapper=new QSignalMapper(this);
  connect(panel_button_mapper,SIGNAL(mapped(int)),
	  this,SLOT(buttonClickedData(int)));

  //
  // Dialogs
  //
  panel_login_dialog=new LoginDialog("ButtonPanel",this);

  //
  // Fire up the SAP connection
  //
  if(panel_password.isEmpty()) {
    if(!panel_login_dialog->exec(&panel_username,&panel_password)) {
      exit(1);
    }
  }
  panel_parser->
    connectToHost(panel_hostname,9500,panel_username,panel_password);
}


MainWidget::~MainWidget()
{
}


QSize MainWidget::sizeHint() const
{
  if(panel_columns==0) {
    return QSize(10+100*panel_buttons.size(),70);
  }
  return QSize(10+100*panel_columns,20+50*panel_rows);
}


void MainWidget::buttonClickedData(int n)
{
  //  printf("setOutputCrosspoint(%d,%d,%d)\n",panel_router,panel_output+1,n+1);
  panel_parser->setOutputCrosspoint(panel_router,panel_output+1,n+1);
}


void MainWidget::changeConnectionState(bool state,
				       SaParser::ConnectionState cstate)
{
  //  printf("changeConnectionState(%d)\n",state);
  if(state) {
    for(QMap<int,AutoPushButton *>::const_iterator it=panel_buttons.begin();
	it!=panel_buttons.end();it++) {
      delete it.value();
    }
    panel_buttons.clear();

    if(panel_parser->outputName(panel_router,panel_output+1).isEmpty()) {
      QMessageBox::warning(this,"ButtonPanel - "+tr("Error"),
			   tr("No such output!"));
      exit(1);
    }
    setWindowTitle(panel_parser->outputName(panel_router,panel_output+1)+" "+
		   tr("Switcher"));
    for(int i=0;i<panel_parser->inputQuantity(panel_router);i++) {
      if(!panel_parser->inputName(panel_router,i+1).isEmpty()) {
	panel_buttons[i]=new AutoPushButton(this);
	panel_buttons[i]->setText(panel_parser->inputName(panel_router,i+1));
	panel_buttons[i]->show();
	connect(panel_buttons[i],SIGNAL(clicked()),
		panel_button_mapper,SLOT(map()));
	panel_button_mapper->setMapping(panel_buttons[i],i);
      }
    }
  }
  else {
    for(int i=0;i<panel_buttons.size();i++) {
      panel_buttons[i]->setDisabled(true);
    }
  }

  panel_rows=1;
  int col=0;
  for(int i=0;i<panel_buttons.size();i++) {
    if((panel_columns>0)&&(col==panel_columns)) {
      panel_rows++;
      col=0;
    }
    col++;
  }

  resize(sizeHint());
}


void MainWidget::changeOutputCrosspoint(int router,int output,int input)
{
  //  printf("changeOutputCrosspoint(%d,%d,%d)\n",router,output,input);
  if((router==panel_router)&&((output-1)==panel_output)) {
    for(QMap<int,AutoPushButton *>::const_iterator it=panel_buttons.begin();
	it!=panel_buttons.end();it++) {
      if((input-1)==it.key()) {
	it.value()->setStyleSheet(LWPANELBUTTON_ACTIVE_STYLESHEET);
      }
      else {
	it.value()->setStyleSheet("");
      }
    }
  }
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  int col=0;
  int row=0;
  
  if(panel_buttons.size()==0) {
    return;
  }
  int cell_w=(size().width()-20)/panel_buttons.size();
  if(panel_columns>0) {
    cell_w=(size().width()-10)/panel_columns;
  }
  int cell_h=(size().height()-20);
  if(panel_columns>0) {
    cell_h=(size().height()-20)/panel_rows;
  }

  if(panel_columns==0) {
    for(QMap<int,AutoPushButton *>::const_iterator it=panel_buttons.begin();
	it!=panel_buttons.end();it++) {
      it.value()->setGeometry(10+cell_w*col,10,cell_w-10,cell_h);
      col++;
    }
  }
  else {
    for(QMap<int,AutoPushButton *>::const_iterator it=panel_buttons.begin();
	it!=panel_buttons.end();it++) {
      it.value()->setGeometry(10+cell_w*col,10+cell_h*(row),
			      cell_w-10,cell_h);
      col++;
      if((panel_columns>0)&&(col==panel_columns)) {
	col=0;
	row++;
      }
    }
  }
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);

  MainWidget *w=new MainWidget();
  w->show();
  return a.exec();
}
