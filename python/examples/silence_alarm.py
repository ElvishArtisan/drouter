#!%PYTHON_BANGPATH%

# silence_alarm.py
#
# Send an e-mail upon receipt of a SILENCE alarm for a specified channel.
#
# (C) Copyright 2018-2019 Fred Gleason <fredg@paravelsystems.com>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License version 2 as
#   published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

import Drouter.StateEngine

#
# Use the standard Python modules for e-mail
#
import smtplib
from email.mime.text import MIMEText

#
# Change these to appropriate values for your setup
#
from_address="sender@example.com"
to_address="recipient@example.com"
smtp_server="smtp.example.com"
host_addr="10.11.12.13"
host_slot=0
host_port="OUTPUT"

# ############################################################################
#
# Callbacks
#
#  These are called by the 'StateEngine' object in response to specific events.
#

#
# Called whenever an audio alarm -- a SILENCE or a CLIP -- changes state.
#
def Alarm(engine,priv,alarm):
    if alarm.hostAddress()==host_addr and alarm.slotNumber()==host_slot and alarm.event()=="SILENCE" and alarm.port()==host_port:
        if alarm.state():
            msg=MIMEText("Silence at "+alarm.channel()+" channel of "+host_port+" port "+host_addr+":"+str(host_slot)+" has been detected!")
            msg['Subject']="Silence Alarm Activated"
            msg['From']=from_address
            msg['To']=to_address
        else:
            msg=MIMEText("Audio at "+alarm.channel()+" channel of "+host_port+" port "+host_addr+":"+str(host_slot)+" has been restored!")
            msg['Subject']="Silence Alarm Cleared"
            msg['From']=from_address
            msg['To']=to_address
        s=smtplib.SMTP(smtp_server)
        s.sendmail(from_address,to_address,msg.as_string())


# ############################################################################
#
# Event Loop
#
# Create a 'StateEngine' object to talk to the drouter service.
#
engine=Drouter.StateEngine.StateEngine()

#
# Set the callbacks so we receive notifications of changes.
#
engine.setAlarmCallback(Alarm)

#
# Start the engine, giving the hostname/address of the Drouter service.
#
engine.start("localhost")
