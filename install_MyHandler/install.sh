#!/bin/bash
cp -f ./common_config.mk configure.ac Makefile.am ../CAgentIoT
cp -f ./module_config.xml ../CAgentIoT/build/Standard/config/module/linux/
cp -f ./agent_config.xml ../CAgentIoT/build/Standard/config/

if [ "$1" = "-handler" ] ; then
echo "install MyHandler"
cp -f MyHandler.tar.gz ../CAgentIoT/Modules/
cd ../CAgentIoT/Modules
tar xzvf MyHandler.tar.gz
fi

echo "install config file done"
