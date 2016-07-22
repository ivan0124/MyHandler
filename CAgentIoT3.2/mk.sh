#/bin/bash
#
    
echo "build myTest module"
cp Makefile.am configure.ac ~ivan/CAgentIoT3.2
rm -rf ~ivan/CAgentIoT3.2/Modules/myTest/
cp -rf ./myTest ~ivan/CAgentIoT3.2/Modules/
cd ~ivan/CAgentIoT3.2/

#generating ./myTest/Makefile.in
automake
#generating ./CAgentIoT3.2/configure
autoconf

#build myTest module
make clean
make

echo "build myTest module done"
cp -f ~ivan/CAgentIoT3.2/Modules/myTest/.libs/libIoTSensor-3.2.6.551.so ~ivan/CAgentIoT3.2/Release/AgentService/module/

cp -f ~ivan/CAgentIoT3.2/Modules/myTest/mqtt_sub.conf ~ivan/CAgentIoT3.2/Release/AgentService/

#list build myTest result
echo "List build myTest module result..."
ls -al ~ivan/CAgentIoT3.2/Modules/myTest/.libs/

#cd /mnt/MyHandler/CAgentIoT3.2

