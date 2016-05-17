#/bin/bash
#
echo "install config file..."
cp -f ./common_config.mk configure.ac Makefile.am ../../../
cp -f ./module_config.xml ../../../build/Standard/config/module/linux/
cp -f ./agent_config.xml ../../../build/Standard/config/
cp -f ./mqtt_sub.conf /usr/lib/Advantech/iotgw/ 
cp -rf ../WISE_IOT/NetDevice/Mqtt/ ../../../Library/MQTTDrv

#echo "install libAdvCJSON.la, libAdvCJSON.so..."
#cp -f ./libAdvCJSON.la ../../../Library/Log/AdvLog/Tools/AdvJSON/src/libAdvCJSON.la
#cp -f ./libAdvCJSON.so ../../../Library/Log/AdvLog/Tools/AdvJSON/src/libAdvCJSON.so
#if [ -d ../../../Release/AgentServcie/ ] ; then
    #echo "install libMqttDrv.so..."
    #cp -f libMqttDrv.so ../../../Release/AgentService/
#fi
echo "install config file done"
