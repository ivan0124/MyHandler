#!/bin/bash
#
echo "install config file..."
cp -f ./common_config.mk configure.ac Makefile.am ../../../
cp -f ./module_config.xml ../../../build/Standard/config/module/linux/
cp -f ./agent_config.xml ../../../build/Standard/config/
cp -f libMqttDrv.so ../../../Release/AgentService/
echo "install config file done"
