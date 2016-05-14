/*

*/
#include <stdio.h> //printf
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <unistd.h>
#include <fcntl.h>

#include "serialmux.h"
#include "dn_socket.h"
#include "dn_lock.h"

typedef struct {
   // admin
   dn_hdlc_rxFrame_cbt  rxFrame_cb;
   // input
   uint8_t              lastRxByte;
   bool                 busyReceiving;
   //bool                 inputEscaping;
   //uint16_t             inputCrc;
   uint8_t              inputBufFill;
   //uint8_t              inputBuf[DN_HDLC_INPUT_BUFFER_SIZE];
   // output
   //uint16_t             outputCrc;
} dn_hdlc_vars_t;

dn_hdlc_vars_t dn_hdlc_vars;

typedef struct {
	int8_t headerToken[4];
	int16_t length;
	int16_t id;
	int8_t commandType;
	int8_t data[128]; // DN_HDLC_INPUT_BUFFER_SIZE
} serial_mux_pkt_t;

#define HEADHER_SZ 6

// callback handlers
void serialmux_rxByte(uint8_t rxbyte);

int g_sock = 0;
fd_set g_readset;
 
int serialmux_open(unsigned char *_ipaddr, unsigned int _iportnum)
{
    struct sockaddr_in server;

    //Create socket
    g_sock = socket(AF_INET , SOCK_STREAM, 0);
    if (g_sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    server.sin_addr.s_addr = inet_addr( _ipaddr );
    server.sin_family = AF_INET;
    server.sin_port = htons( _iportnum );
 
    //Connect to remote server
    if (connect(g_sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return -1;
    }

    return 0;
}

int serialmux_close()
{
	close(g_sock);
}

void serialmux_init(dn_hdlc_rxFrame_cbt rxFrame_cb)
{
	// reset local variables
	memset(&dn_hdlc_vars,   0, sizeof(dn_hdlc_vars));

	// store params
	dn_hdlc_vars.rxFrame_cb = rxFrame_cb;

	// initialize socket
	dn_socket_init(serialmux_rxByte);
}

int	serialmux_write(uint8_t _itxPacketId, uint8_t _icmdId, uint8_t _ilength, uint8_t *payload)
{
	serial_mux_pkt_t serialmux_pkt;
	unsigned char *pmsg = (unsigned char*)&serialmux_pkt;

    if(g_sock == 0)
		return -1;

    memset(pmsg, '\0', sizeof(serialmux_pkt));
	serialmux_pkt.headerToken[0] = 0xa7;
	serialmux_pkt.headerToken[1] = 0x40;
	serialmux_pkt.headerToken[2] = 0xa0;
	serialmux_pkt.headerToken[3] = 0xf5;
	serialmux_pkt.id = ntohs(_itxPacketId); 
	serialmux_pkt.commandType = _icmdId; // cmdId
    memcpy(serialmux_pkt.data, payload, _ilength);
    serialmux_pkt.length = ntohs(_ilength+3); // 3 is size of (id+commandType)

	if( send(g_sock , pmsg , ntohs(serialmux_pkt.length)+HEADHER_SZ , 0) < 0) {
		perror("Error sending");
		return -1;
	}

	return 0;
}

void serialmux_rxByte(uint8_t rxbyte) 
{
	struct timeval timeout;
	serial_mux_pkt_t serialmux_pkt;
	unsigned char *pmsg = (unsigned char*)&serialmux_pkt;

	// lock the module
	dn_lock();

	FD_ZERO(&g_readset);
	FD_SET(g_sock, &g_readset);

	timeout.tv_sec=0;
	timeout.tv_usec=100000;

	if(select(g_sock+1, &g_readset, NULL, NULL, &timeout) == -1)
		printf("Error on Select\n");

	if(FD_ISSET(g_sock, &g_readset)) {
		//Receive a reply from the server
		memset(pmsg, '\0', sizeof(serialmux_pkt));
		if( recv(g_sock , pmsg , sizeof(serialmux_pkt), 0) < 0) {
			printf("recv failed\n");
		}
		// hand over frame to upper layer
		dn_hdlc_vars.rxFrame_cb((uint8_t*)&serialmux_pkt, sizeof(serialmux_pkt));
	}

#if 0
	if( dn_hdlc_vars.busyReceiving==FALSE  &&
		dn_hdlc_vars.lastRxByte==DN_HDLC_FLAG &&
		rxbyte!=DN_HDLC_FLAG ) {
		// start of frame
      
		// I'm now receiving
		dn_hdlc_vars.busyReceiving       = TRUE;
 
		// create the HDLC frame
		dn_hdlc_inputOpen();

		// add the byte just received
		dn_hdlc_inputWrite(rxbyte);
	} else if ( dn_hdlc_vars.busyReceiving==TRUE   &&
		rxbyte!=DN_HDLC_FLAG ){
		// middle of frame
      
		// add the byte just received
		dn_hdlc_inputWrite(rxbyte);

		if (dn_hdlc_vars.inputBufFill+1>DN_HDLC_INPUT_BUFFER_SIZE) {
			// input buffer overflow
			dn_hdlc_vars.inputBufFill       = 0;
			dn_hdlc_vars.busyReceiving      = FALSE;
		}
	} else if ( dn_hdlc_vars.busyReceiving==TRUE   &&
         rxbyte==DN_HDLC_FLAG ) {
		// end of frame

		// finalize the HDLC frame
		dn_hdlc_inputClose();

		if (dn_hdlc_vars.inputBufFill==0) {
			// invalid HDLC frame
		} else {
			// hand over frame to upper layer
			dn_hdlc_vars.rxFrame_cb(&dn_hdlc_vars.inputBuf[0],dn_hdlc_vars.inputBufFill);

			// clear inputBuffer
			dn_hdlc_vars.inputBufFill=0;
		}

		dn_hdlc_vars.busyReceiving = FALSE;
	}

	dn_hdlc_vars.lastRxByte = rxbyte;
#endif
	// unlock the module
	dn_unlock();
}
