#!/bin/bash

# Configure test fixtures
#
#   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2 of
#   the License, or (at your option) any later version.
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

echo "Configure the test fixtures for the ProtocolJ test suite."
echo

read -a MAP_NODE_IP -p "IPv4 address of the test node: "

#
# SA Maps
#
cat config/maps.d/01-Test_Audio.map.src | sed s/@MAP_NODE_IP@/$MAP_NODE_IP/ > config/maps.d/01-Test_Audio.map

cat config/maps.d/02-Test_GPIO.map.src | sed s/@MAP_NODE_IP@/$MAP_NODE_IP/ > config/maps.d/02-Test_GPIO.map

#
# Test Cases
#
cat tests_action.tst.src | sed s/@MAP_NODE_IP@/$MAP_NODE_IP/ > tests_action.tst
cat tests_audio.tst.src | sed s/@MAP_NODE_IP@/$MAP_NODE_IP/ > tests_audio.tst
cat tests_gpio.tst.src | sed s/@MAP_NODE_IP@/$MAP_NODE_IP/ > tests_gpio.tst

