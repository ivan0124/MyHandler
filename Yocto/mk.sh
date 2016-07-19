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
    #rm -rf ~ivan/CAgentIoT/Modules/IoTGWHandlerV2/
    
    if [ "$2" != "3310" ] ; then
        echo "build IoTGWHandlerV2 version"
        cp -rf ./IoTGWHandlerV2 ~ivan/CAgentIoT/Modules/
        cd ~ivan/CAgentIoT/Modules/IoTGWHandlerV2/
        make clean
        make
        cp ./IoTSensorHandlerV2.so.3.2.5.5990 ~ivan/CAgentIoT/Release/AgentService/module/
        cd /mnt/MyHandler/Yocto/
        echo "build IoTGWHandlerV2 version done"
    fi
fi

cp -rf ./module_config.xml ~ivan/CAgentIoT/Release/AgentService/module/
cp -rf ./agent_config.xml ~ivan/CAgentIoT/Release/AgentService/

if [ "$2" == "3310" ] ; then
    cp -rf ~ivan/CAgentIoT/Release/AgentService /var/nfsshare
    echo "copy AgentService to /var/nfsshare done."
fi
