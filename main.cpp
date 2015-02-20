//w5100
#include "mbed.h"
#include "WIZnetInterface.h"
#include <string>

#define ECHO_SERVER_PORT   9999
#define	BLDG_SERVER_IP	   "192.168.1.106"
#define RUNASCLIENT

WIZnetInterface eth(SPI_MOSI, SPI_MISO, SPI_SCK,SPI_CS,PB_4); // spi, cs, reset
Serial pc(SERIAL_TX,SERIAL_RX);
// theres a conflict with LED1 on the Nucleo board it uses the same pin as SPI_SCK!
//DigitalOut led(LED1);
// This is the chip select for the sd card which shares the SPI bus on the Arduino shield.
DigitalOut SD_CS(PB_5);
DigitalOut LED(PB_3,0);


void f_ethernet_init(void);
void check_stream(char* buf);

const char * IP_Addr    = "192.168.1.210";
const char * IP_Subnet  = "255.255.255.0";
const char * IP_Gateway = "192.168.1.1";
char data[8];

int ret;
bool status;
int max_attempts = 4;

char paq_en[64];
char * str0 = "<h1>Suck it Trebeck!</h1>";

int main()
{
	LED = 0;
	wait(.5);
	LED = 1;
	wait(.5);
	LED = 0;
// force the chip select for the SD card high to avoid collisions. We're not using the sd card for this program    
    char buffer[64];
    char data_entry[64];
    char data[512];
    SD_CS=1;
    uint8_t mac[]={0x90,0xa2,0xDa,0x0d,0x42,0xe0};
    int attempt=0;    
    int entry=42;
    int length;
    f_ethernet_init();
#ifdef RUNASCLIENT
    TCPSocketConnection bldg_client;
#else
    TCPSocketServer server;
    server.bind(ECHO_SERVER_PORT);
    server.listen();
#endif

#ifdef RUNASCLIENT
    while(attempt < max_attempts){
    	pc.printf("\nAttempting to Connect to Server...\n\r");
    	ret=bldg_client.connect(BLDG_SERVER_IP,ECHO_SERVER_PORT);
    	if(!ret){
    		pc.printf("\nConnected to Building Server\n\r");
    	}else{
    		pc.printf("Connection Failed...This is attempt #%d of %d\n\r",attempt,max_attempts);
    		attempt++;
    	}
    	if(bldg_client.is_connected()){
    		int len = strlen(data);
    		//sprintf(data,"  %s\n\n\n\r",str0);
    		data[0] = (len & 0xFF00) >> 8;
    		data[1] = len & 0x00FF;
    		data[2] = 'S';
    		data[3] = 'T';
    		data[4] = 'U';
    		data[5] = 'F';
    		data[6] = 'F';
    		//pc.printf("%s",data);
    		bldg_client.send_all(data,sizeof(data));
    		wait(5);
    	}
    }
#else
    while(true){

    	pc.printf("\nWaiting for Connection\n\r");
    	TCPSocketConnection client;
    	server.accept(client);

    	pc.printf("Connection from: %s\n", client.get_address());
    	char buffer[256];

    	client.send(str0,strlen(str0));
    	sprintf(data,"<h2> MAC: %s <br> IP %s <\h2>",eth.getMACAddress(),eth.getIPAddress());
    	client.send(data,strlen(data));
    	wait(.1);
    	client.close();

    	while(true){

    		int n = client.receive(buffer, sizeof(buffer));
    		if (n<=0) break;

    		client.send_all(buffer,n);
    		if (n<=0) break;

    		check_stream(buffer);

    	}
    	wait(10);
    	pc.printf("\n***Closing Client***\n\r");
    	client.close();
    	//break;
    }
#endif
    
}

void f_ethernet_init()
{
    uint8_t mac[]={0x90,0xa2,0xDa,0x0d,0x42,0xe0};
    // mbed_mac_address((char *)mac); 
    pc.printf("\tStarting Ethernet Server ...\n\r");
    wait(1.0);
    ret = eth.init(mac);
    if(!ret)
    {
        pc.printf("Initialized, MAC= %s\n\r",eth.getMACAddress());
    }    
    else
    {
        pc.printf("Communication Failure  ... Restart devices ...\n\r");    
    }
    pc.printf("Connecting");
    wait(0.5);
    pc.printf(".");
    wait(0.5);
    pc.printf(".\n\r");
    wait(0.5);
    ret = eth.connect();
    if(!ret)
    {
        pc.printf("Connection Established!\n\n\r");
        wait(1);
        pc.printf("IP=%s\n\rMASK=%s\n\rGW=%s\n\r",eth.getIPAddress(), eth.getNetworkMask(), eth.getGateway());
    }    
    else
    {
        pc.printf("Communication Failure  ... Restart devices ...\n\r"); 
    }
}  
 
void check_stream(char* buf){

	for(int i = 0;i<sizeof(buf);i++){
		if(buf[i] == 'L' && buf[i+1] == 'E' && buf[i+2] == 'D'){
			LED = !LED;
		}else if(buf[i] == 'D' && buf[i+1] == 'A' && buf[i+2] == 'T' && buf[i+3] == 'A'){
			pc.printf("Data Received");
		}
	}
}
