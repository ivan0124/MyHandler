#/bin/bash
#
if [ "$1" == "v1" ] ; then
    echo "build IoTGWHandler version"
    rm -rf ~ivan/CAgentIoT/Modules/IoTGWHandler/
    cp -rf ./IoTGWHandler ~ivan/CAgentIoT/Modules/
    cd ~ivan/CAgentIoT/Modules/IoTGWHandler/
    make
    echo "build IoTGWHandler done"
    cp ./IoTSensorHandler.so.3.1.30.5434 ~ivan/CAgentIoT/Release/AgentService/module/IoTSensorHandlerV2.so.3.1.30.5434
    cd /mnt/MyHandler/Yocto/
else
echo "build IoTGWHandlerV2 version"
    rm -rf ~ivan/CAgentIoT/Modules/IoTGWHandlerV2/
    cp -rf ./IoTGWHandlerV2 ~ivan/CAgentIoT/Modules/
    cd ~ivan/CAgentIoT/Modules/IoTGWHandlerV2/
    make
    cp ./IoTSensorHandlerV2.so.3.1.30.5434 ~ivan/CAgentIoT/Release/AgentService/module/
    cd /mnt/MyHandler/Yocto/
    echo "build IoTGWHandlerV2 version done"
fi
