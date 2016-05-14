#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <string>
#include "platform.h"
#include "WSNType.h"
#include "TopologyTranslate.h"
#include "AdvLog.h"
typedef struct {
	int a;
	int b;
} Link;

bool QSortCompare(Link right, Link left)
{
	  if(left.a == -1 && left.b == -1) return 1;
	  if(right.a > left.a) return 0;
	  else if(right.a == left.a) {
		  if(right.b > left.b) return 0;
		  else if(right.b == left.b) return 0;
		  else return 1;
	  } else return 1;
}

void SNCALL SN_Translate(char *iface, Topology *mac, Topology *index) {
	char *set = NULL;
	std::set<Link,bool(*)(Link,Link)> link(QSortCompare);
	std::map<std::string,int> nodeMap;
	int count = 1;
	char mesh[4096] = {0};
	char maca[128] = {0};
	char macb[128] = {0};
	int temp;
	Link connect;
	nodeMap[iface] = 0;
	
	if(index->listlen < mac->listlen) {
		index->listlen = mac->listlen;
		index->list = (char *)realloc(index->list,index->listlen);
	}
	strcpy(index->list, mac->list);
	
	set = strtok(mac->list, ", ");
	while(set != NULL) {
		nodeMap[set] = count++;
		set = strtok(NULL, ", ");
	}
	
	set = strtok(mac->mesh, ", ");
	while(set != NULL) {
		sscanf(set,"%[^<]<=>%s",maca,macb);
		connect.a = nodeMap[maca];
		connect.b = nodeMap[macb];
		
		if(connect.a > connect.b) {
			temp = connect.a;
			connect.a = connect.b;
			connect.b = temp;
		}
		
		link.insert(connect);
		nodeMap[set] = count++;
		set = strtok(NULL, ", ");
	}

	set = mesh;
	for (std::set<Link,bool(*)(Link,Link)>::iterator it=link.begin(); it!=link.end();it++) {
		set+=sprintf(set,"%d-%d,", it->a, it->b);
		ADV_INFO("%d-%d,\n", it->a, it->b);
	}
	
	temp = strlen(mesh)-1;
	mesh[temp] = 0;
	
	if(index->meshlen < temp) {
		index->meshlen = temp;
		index->mesh = (char *)realloc(index->mesh,index->meshlen);
	}
	strcpy(index->mesh, mesh);
}