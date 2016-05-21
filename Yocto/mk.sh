#/bin/bash
#
rm -rf ../../IoTGWHandlerV2
cp -rf ./IoTGWHandlerV2 ../../
cd ../../IoTGWHandlerV2
make
cp ./IoTSensorHandlerV2.so.3.1.30.5434 ../../Release/AgentService/module/
cd ../MyHandler/Yocto/
echo "done"
