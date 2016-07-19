#/bin/bash
#
if [ "$1" == "v1" ] ; then
    echo "build IoTGWHandler version"
    rm -rf ~ivan/CAgentIoT/Modules/IoTGWHandler/
    cp -rf ./IoTGWHandler ~ivan/CAgentIoT/Modules/
    cd ~ivan/CAgentIoT/Modules/IoTGWHandler/
    make
    echo "build IoTGWHandler done"
    cp ./IoTSensorHandler.so.3.1.32.5653 ~ivan/CAgentIoT/Release/AgentService/module/IoTSensorHandlerV2.so.3.1.30.5434
    cd /mnt/MyHandler/Yocto/
else
echo "build IoTGWHandlerV2 version"
    #rm -rf ~ivan/CAgentIoT/Modules/IoTGWHandlerV2/
    cp -rf ./IoTGWHandlerV2 ~ivan/CAgentIoT/Modules/
    cd ~ivan/CAgentIoT/Modules/IoTGWHandlerV2/
    make clean
    make
    cp ./IoTSensorHandlerV2.so.3.2.5.5990 ~ivan/CAgentIoT/Release/AgentService/module/
    cd /mnt/MyHandler/Yocto/
    echo "build IoTGWHandlerV2 version done"
fi
cp -rf ./module_config.xml ~ivan/CAgentIoT/Release/AgentService/module/
cp -rf ./agent_config.xml ~ivan/CAgentIoT/Release/AgentService/
#cp -f ./IoTGWHandlerV2/mqtt_sub.conf ~ivan/CAgentIoT/Release/AgentService/
