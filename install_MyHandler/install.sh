#!/bin/bash
#
echo "install config file..."
cp -f ./common_config.mk configure.ac Makefile.am ../../../
cp -f ./module_config.xml ../../../build/Standard/config/module/linux/
cp -f ./agent_config.xml ../../../build/Standard/config/
if [ -d ../../../Release/AgentServcie/ ] ; then
    cp -f libMqttDrv.so ../../../Release/AgentService/
fi
echo "install config file done"
