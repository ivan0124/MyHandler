#!/bin/bash
VERBOSE=0

MAKE_START_TIME=`date +%s`

case "$1" in
	rebuild)
	echo "make building..."
	if [ "$VERBOSE" == "0" ] ; then
             make clean
   	     make > make.log 2>&1
	else
             make clean
   	     make
	fi ;;
	install)
	sudo cp -rf ./MyHandler.so.3.1.30.5318 ../../Release/AgentService/module/
	cd .. ;;
	test)
	cd ../../Release/AgentService/
	sudo ./cagent
        cd ../../Modules/MyHandler/
        sleep 1
        ps aux | grep cagent
        ;;
        stop)
	sudo killall cagent
        sleep 2
        ps aux | grep cagent;;
	*)
	echo "step1: 'mk.sh rebuild' to rebuild code."
	echo "step2: 'mk.sh install' to install the MyHandler.so.3.1.30"
	echo "step3: 'mk.sh test' to test."
	echo "step3: 'mk.sh stop' to kill all cagent process"
	exit -1 ;;
esac

if [ "$?" != "0" ] ; then
    echo "make $1 Fail!!!"
    echo "please check the make.log to debug"
else
    echo "make $1 OK."
    if [ "$1" = "rebuild" ] ; then
       sudo cp -rf ./MyHandler.so.3.1.30.5318 ../../Release/AgentService/module/
        
       sudo rm -rf ../../Library/MQTTDrv 
       sudo cp -rf ./WISE_IOT/NetDevice/Mqtt/ ../../Library/MQTTDrv
       cd ../../Library/MQTTDrv/
       sudo make
       sudo cp ./src/.libs/libMqttDrv.so ../../Release/AgentService/
       cd ../../Modules/MyHandler
    fi
fi

MAKE_END_TIME=`date +%s`
duration=$((MAKE_END_TIME-MAKE_START_TIME))
echo "$(($duration / 60)) minutes and $(($duration % 60)) seconds elapsed."
