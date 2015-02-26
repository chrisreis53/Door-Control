//w5100
#include "mbed.h"
#include "WIZnetInterface.h"
#include "string.h"
#include "strings.h"


#define ECHO_SERVER_PORT	9999
#define	BLDG_SERVER_IP		"192.168.1.104"
#define RUNASCLIENT			//Changes code to connect to a server or run standalone

WIZnetInterface eth(SPI_MOSI, SPI_MISO, SPI_SCK,SPI_CS,PB_4); // spi, cs, reset
Serial pc(SERIAL_TX,SERIAL_RX);
// theres a conflict with LED1 on the Nucleo board it uses the same pin as SPI_SCK!
//DigitalOut led(LED1);
// This is the chip select for the sd card which shares the SPI bus on the Arduino shield.
DigitalOut SD_CS(PB_5);

DigitalOut LED_1(PA_0,0);
DigitalOut LED_2(PA_1,0);
DigitalOut LED_3(PA_4,0);
DigitalIn EGRESS(PB_0);

void f_ethernet_init(void);
void check_stream(char* buf);
void egress_button(void);

const char * IP_Addr    = "192.168.1.210";
const char * IP_Subnet  = "255.255.255.0";
const char * IP_Gateway = "192.168.1.1";
char data[8];

int ret;
bool status;
int max_attempts = 10;
int count = 0;

char paq_en[64];
char str_bldg[128];
char str_room[128];
char str_doors[128];
char * str_MAC = "MAC";
uint8_t mac[]={0x90,0xa2,0xDa,0x0d,0x42,0xe0};

int main()
{
	LED_1 = 1;
	LED_2 = 1;
	LED_3 = 1;
	wait(1);
	LED_1 = 0;
	LED_2 = 0;
	LED_3 = 0;
// force the chip select for the SD card high to avoid collisions. We're not using the sd card for this program    
    char buffer[64];
    char data_entry[64];
    char data[512];
    SD_CS=1;
    //uint8_t mac[]={0x90,0xa2,0xDa,0x0d,0x42,0xe0};
    int attempt=1;
    int entry=42;
    int length;
    f_ethernet_init();
#ifdef RUNASCLIENT
    TCPSocketConnection bldg_client;
    bldg_client.set_blocking(true, 500);
#else
    TCPSocketServer server;
    server.bind(ECHO_SERVER_PORT);
    server.listen();
#endif

#ifdef RUNASCLIENT
    while(attempt <=  max_attempts){
    	egress_button();
    	pc.printf("\nAttempting to Connect to Server...\n\r");
    	ret=bldg_client.connect(BLDG_SERVER_IP,ECHO_SERVER_PORT);
    	if(!ret){
    		pc.printf("\nConnected to Building Server\n\r");
    	}else{
    		pc.printf("Connection Failed...This is attempt #%d of %d\n\r",attempt,max_attempts);
    		attempt++;
    	}
    	while(bldg_client.is_connected()){
    		int n;
    		max_attempts = 0;
    		pc.printf("Sending Data\n\r");

    		bldg_client.send("ID",2);
    		bldg_client.send(eth.getMACAddress(),strlen(eth.getMACAddress()));
    		pc.printf("Receiving Data\n\r");
    		n=bldg_client.receive_all(str_bldg,64);
    		if(n <= 0) break;
    		n=bldg_client.receive_all(str_room,64);
    		if(n <= 0) break;
    		n=bldg_client.receive_all(str_doors,64);
    		if(n <= 0) break;
    		pc.printf("Bldg: %s, Room: %s, Doors: %s\n\r", str_bldg, str_room, str_doors);
    		//wait(1);

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
    pc.printf("\n\r####Starting Ethernet Server#### \n\r");
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
			LED_1 = !LED_1;
		}else if(buf[i] == 'D' && buf[i+1] == 'A' && buf[i+2] == 'T' && buf[i+3] == 'A'){
			pc.printf("Data Received");
		}
	}
}

void egress_button(void){
	if(EGRESS){
		pc.printf("EGRESS DEPRESSED");
		LED_1=1;
	}
}
