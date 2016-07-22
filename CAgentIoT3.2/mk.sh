#/bin/bash
#

my_module=IoTGWHandlerV2    
echo "build IoTGWHandlerV2 module"
cp Makefile.am configure.ac ~ivan/CAgentIoT3.2
#rm -rf ~ivan/CAgentIoT3.2/Modules/myTest/
rm -rf ~ivan/CAgentIoT3.2/Modules/IoTGWHandlerV2/
cp -rf /mnt/MyHandler/Yocto/IoTGWHandlerV2 ~ivan/CAgentIoT3.2/Modules
cp -rf ./myTest/Makefile.am ~ivan/CAgentIoT3.2/Modules/IoTGWHandlerV2/
cp -rf ./myTest/common.c ~ivan/CAgentIoT3.2/Modules/IoTGWHandlerV2/
cp -rf ./myTest/common.h ~ivan/CAgentIoT3.2/Modules/IoTGWHandlerV2/
cp -rf ./myTest/platform.h ~ivan/CAgentIoT3.2/Modules/IoTGWHandlerV2/
#cp -rf ./myTest ~ivan/CAgentIoT3.2/Modules/
cd ~ivan/CAgentIoT3.2/

#generating ./IoTGWHandlerV2/Makefile.in
automake
#generating ./CAgentIoT3.2/configure
autoconf

#build IoTGWHanlerV2 module
make clean
make

echo "build myTest module done"
cp -f ~ivan/CAgentIoT3.2/Modules/IoTGWHandlerV2/.libs/IoTSensorHandlerV2-3.2.6.551.so ~ivan/CAgentIoT3.2/Release/AgentService/module/

cd ~ivan/CAgentIoT3.2/Release/AgentService/module/
ln -s IoTSensorHandlerV2-3.2.6.551.so IoTSensorHandlerV2.so

cp -f ~ivan/CAgentIoT3.2/Modules/IoTGWHandlerV2/mqtt_sub.conf ~ivan/CAgentIoT3.2/Release/AgentService/

cp -rf /mnt/MyHandler/Yocto/module_config.xml ~ivan/CAgentIoT3.2/Release/AgentService/module/
cp -rf /mnt/MyHandler/Yocto/agent_config.xml ~ivan/CAgentIoT3.2/Release/AgentService/
#list build IoTGWHandlerV2 result
echo "List build myTest module result..."
ls -al ~ivan/CAgentIoT3.2/Modules/IoTGWHandlerV2/.libs/

#cd /mnt/MyHandler/CAgentIoT3.2

