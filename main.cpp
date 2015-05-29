/*
*
*  AFRL Door Control Program
*
*/
#include "mbed.h"
#include "WIZnetInterface.h"
#include "stm32f1xx_hal_iwdg.h"
#include "strings.h"
#include <string.h>
#include "stdio.h"
#include <stdlib.h>

////Defines////
#define ECHO_SERVER_PORT    9999
//#define BLDG_SERVER_IP    "10.10.1.7"
#define BLDG_SERVER_IP      "192.168.1.103"
#define USE_DHCP

//#define TEST
//#define panicLock0
//#define panicLock1
//#define panicLock2
//#define panicLock3
//#define panicLock4
//#define panicLock5
//#define panicLock6
//#define panicLock7
#define panicLock8
//#define panicLock9
//#define panicLock10


#if defined(panicLock0)
uint8_t mac[]={0x00,0x08,0xDC,0x1E,0x72,0x1F };
const char * IP_Addr    = "10.10.1.100";
#define NUM_ALARMS	2
#elif defined(panicLock1)
uint8_t mac[]={0x00,0x08,0xDC,0x1E,0x72,0x1F };
const char * IP_Addr    = "10.10.1.101";
#define NUM_ALARMS	2
#elif defined(panicLock2)
uint8_t mac[]={0x00,0x08,0xDC,0x1E,0x72,0x47 };
const char * IP_Addr    = "10.10.1.102";
#define NUM_ALARMS	2
#elif defined(panicLock3)
uint8_t mac[]={0x00,0x08,0xDC,0x1E,0x72,0x2B };
const char * IP_Addr    = "10.10.1.103";
#define NUM_ALARMS	3
#elif defined(panicLock4)
uint8_t mac[]={0x00,0x08,0xDC,0x1E,0x72,0x3D };
const char * IP_Addr    = "10.10.1.104";
#define NUM_ALARMS	1
#elif defined(panicLock5)
uint8_t mac[]={0x00,0x08,0xDC,0x1E,0x72,0xE3 };
const char * IP_Addr    = "10.10.1.105";
#define NUM_ALARMS	2
#elif defined(panicLock6)
uint8_t mac[]={0x00,0x08,0xDC,0x1E,0x72,0xE8 };
const char * IP_Addr    = "10.10.1.106";
#define NUM_ALARMS	3
#elif defined(panicLock7)
uint8_t mac[]={0x00,0x08,0xDC,0x1E,0x72,0xDA };
const char * IP_Addr    = "10.10.1.107";
#define NUM_ALARMS	1
#elif defined(panicLock8)
uint8_t mac[]={0x00,0x08,0xDC,0x1E,0x72,0xCF };
const char * IP_Addr    = "10.10.1.108";
#define NUM_ALARMS	3
#elif defined(panicLock9)
uint8_t mac[]={0x00,0x08,0xDC,0x1E,0x73,0x1B };
const char * IP_Addr    = "10.10.1.109";
#define NUM_ALARMS	2
#elif defined(panicLock10)
uint8_t mac[]={0x00,0x08,0xDC,0x1E,0x73,0x1E };
const char * IP_Addr    = "10.10.1.110";
#define NUM_ALARMS	2
#endif
#ifdef TEST
const char * IP_Addr = "192.168.1.200";
#define NUM_ALARMS	2
#endif

const char * IP_Subnet  = "255.255.255.0";
const char * IP_Gateway = "10.10.1.254";

////Prototypes////
void check_stream(char* buf);
void f_ethernet_init();
void set_doors(int a,int b,int c, int d,int e,int f,int g,int h);
void read_egress(void);
void egress_timer(void);
void button_check(void);
void read_alarm(void);
void send_data(void);

////Pin Declarations////
DigitalOut DOORS[8]={PC_6,PC_7,PC_8,PC_9,PA_8,PA_15,PC_10,PC_11};	//D0,D1,D2,D3,D4D5,D6,D7
DigitalIn EGRESS[8]={PC_12,PD_2,PB_3,PB_4,PB_5,PB_6,PB_7,PB_8};		//D8,D9,D10,D11,D12,D13,D14,D15
DigitalIn ALARM[4]={PA_7,PA_6,PA_5,PA_4};

DigitalOut Debug_LED_0(PC_4,1);
DigitalOut Debug_LED_1(PC_5,1);

////Constructors///
SPI spi(PB_15,PB_14,PB_13); 			// timer interface -> mosi, miso, sclk
WIZnetInterface eth(&spi, PB_12, PB_0); // WIZnet interface -> spi, cs, reset
Serial pc(PA_9,PA_10);					// USART interface -> RX, TX
IWDG_HandleTypeDef hiwdg;				//Watchdog timer
Timer t[8];								//timer
Timer data_timer;						//data timer
Ticker flipper;							//Interrupt ticker
TCPSocketServer door_server;			//TCP Socket Server
TCPSocketConnection bldg_client;		//TCP Socket Connection

////Variables////
bool door_status[8]={false,false,false,false,false,false,false,false};
bool door_egress[8]={false,false,false,false,false,false,false,false};
bool alarm_status[4]={false,false,false,false};
bool connection_status = false;
bool alarm_signal = false;
int attempt=1;
int max_attempts = 10;
int ret;
int watchTimeMs = 10000;
char buffer[512];
//char data[512];

 
int main()
{
	//DOOR:11111111
	while (1){
		f_ethernet_init();
		door_server.bind(ECHO_SERVER_PORT);
		door_server.listen();
		bldg_client.set_blocking(false,50);
		door_server.set_blocking(false,50);
		data_timer.start();
		while(1){
			button_check();
			read_alarm();
			connection_status = door_server.accept(bldg_client);
			Debug_LED_0=0;
			if(!connection_status){
				pc.printf("Received Client Connection\n\r");
				while(bldg_client.is_connected()){
					Debug_LED_1=0;

					char data[512];
					int n;
					read_alarm();
					button_check();
					n = bldg_client.receive(data,512);
					if (n>0) {
						pc.printf("%s sent %s\n\r",bldg_client.get_address(),data);
						check_stream(data);
					}
					if(data_timer.read()>5){
						pc.printf("Sending Data\n\r");
						send_data();
						data_timer.reset();
						pc.printf("Connected...ALARM:%d%d%d%d\n\r",ALARM[0].read(),ALARM[1].read(),ALARM[2].read(),ALARM[3].read());
					}

				}
				Debug_LED_1=1;
			}else{
				if(data_timer.read()>5){
					pc.printf("No Connection...ALARM:%d%d%d%d\n\r",ALARM[0].read(),ALARM[1].read(),ALARM[2].read(),ALARM[3].read());
					data_timer.reset();
				}
			}
		}
	}
}

void f_ethernet_init()
{

    // mbed_mac_address((char *)mac); 
    pc.printf("\n\r####Starting Ethernet Server#### \n\r");
    wait(1.0);
	#ifdef USE_DHCP
		int ret = eth.init(mac);
	#else
		int ret = eth.init(mac,IP_Addr,IP_Subnet,IP_Gateway);
	#endif
    if(!ret)
    {
        pc.printf("Initialized, MAC= %s\n\r",eth.getMACAddress());
    }    
    else
    {
        pc.printf("Communication Failure  ... Restart devices ...\n\r");    
    }
    pc.printf("Connecting to building server @ %s on port %d...\n\r",BLDG_SERVER_IP, ECHO_SERVER_PORT );
    ret = eth.connect();
    if(!ret)
    {
        pc.printf("Connection Established!\n\n\r");
        wait(.5);
        pc.printf("IP=%s\n\rMASK=%s\n\rGW=%s\n\r",eth.getIPAddress(), eth.getNetworkMask(), eth.getGateway());
    }    
    else
    {
        pc.printf("Communication Failure  ... Restart devices ...\n\r"); 
    }
}  

void check_stream(char* buf)
{

    for(int i = 0; i<strlen(buf); i++) {

        if(buf[i] == 'D' && buf[i+1] == 'O' && buf[i+2] == 'O' && buf[i+3] == 'R' && buf[i+4] == ':') {
            set_doors(buf[i+5]-'0',buf[i+6]-'0',buf[i+7]-'0',buf[i+8]-'0',buf[i+9]-'0',buf[i+10]-'0',buf[i+11]-'0',buf[i+12]-'0');
        }
        if(buf[i] == 'D' && buf[i+1] == 'O' && buf[i+2] == 'O' && buf[i+3] == 'R' && buf[i+4] == ':' && buf[i+5] == 'L' && buf[i+6] == 'O' && buf[i+7] == 'C' && buf[i+8] == 'K') {
            set_doors(1,1,1,1,1,1,1,1);
        }
        if(buf[i] == 'D' && buf[i+1] == 'O' && buf[i+2] == 'O' && buf[i+3] == 'R' && buf[i+4] == ':' && buf[i+5] == 'U' && buf[i+6] == 'N' && buf[i+7] == 'L' && buf[i+8] == 'O' && buf[i+9] == 'C' && buf[i+10] == 'k') {
            set_doors(0,0,0,0,0,0,0,0);
        }
        if(buf[i] == 'A' && buf[i+1] == 'L' && buf[i+2] == 'A' && buf[i+3] == 'R' && buf[i+4] == 'M' && buf[i+5] == ':' && buf[i+6] == 'A' && buf[i+7] == 'C' && buf[i+8] == 'K') {
        	for(int i = 0;i<NUM_ALARMS;i++){
        		if(alarm_status[i]==true){
        			alarm_status[i]==false;
        		}
        	}
        }
    }
}

void set_doors(int a, int b, int c, int d, int e, int f, int g, int h){
    
    for (int var = 0; var < 8; ++var) {
    	if(door_status[var] == true && door_egress[var] == false){
    		DOORS[var] = true;
    	}else if (door_status[var] == true && door_egress[var] == true) {
			DOORS[var] = false;
		}else{
			DOORS[var] = false;
		}
	}

    door_status[0] = a;
    door_status[1] = b;
    door_status[2] = c;
    door_status[3] = d;
    door_status[4] = e;
    door_status[5] = f;
    door_status[6] = g;
    door_status[7] = h;

    for(int i = 0;i < 8;i++){
    	if(DOORS[i]!=false && (data_timer.read()>5)){
    		sprintf(buffer,"STATUS:%d%d%d%d%d%d%d%d\n\r",door_status[0],door_status[1],door_status[2],door_status[3],door_status[4],door_status[5],door_status[6],door_status[7]);
    		pc.printf(buffer);
    		bldg_client.send(buffer,strlen(buffer));
    		data_timer.reset();
    		break;
    	}
    }

}

void read_egress(void){

	for(int i = 0; i<8;i++){

		if(door_status[i] && EGRESS[i].read()){
			door_egress[i] = true;
			t[i].start();
			pc.printf("Timer started for door: %d\r\n",(i+1));
		}
	}

}

void egress_timer(void){

	for(int i = 0; i<8;i++){

		if (t[i].read() >= 5) {
			t[i].stop();
			t[i].reset();
			door_egress[i] = false;
			pc.printf("Timer %d has reached 5 seconds\r\n",(i+1));
		}

	}

}

void button_check(void){
	read_egress();
	egress_timer();
	set_doors(door_status[0],door_status[1],door_status[2],door_status[3],door_status[4],door_status[5],door_status[6],door_status[7]);
}

void read_alarm(void){

//	for(int i = 0;i<1;i++){
//		if((ALARM[i].read()==0) && (alarm_status[i]==false)){
//			alarm_status[i]=true;
//			alarm_signal = true;
//			pc.printf("Alarm %i Detected\n\r",i+1);
//		}else if(ALARM[i].read()==1){
//			alarm_status[i]=false;
//		}
//	}

	for(int i = 0;i<NUM_ALARMS;i++){
		if((ALARM[i].read()==0) && (alarm_status[i]==false)){
			pc.printf("Alarm %i Detected\n\r",i+1);
			alarm_status[i]=true;
		}
		if((ALARM[i].read()==1) && (alarm_status[i]==true)){
			pc.printf("Alarm %i RESET\n\r",i+1);
			alarm_status[i]=false;
		}
	}
}

void send_data(void){
	read_alarm();
	char buf[] = "ALARM:1234\n\r\0";
	buf[6]=('0' + int(alarm_status[0]));buf[7]=('0'+int(alarm_status[1]));buf[8]=('0' + int(alarm_status[2]));buf[9]=('0' + int(alarm_status[3]));
	bldg_client.send(buf,strlen(buf));
	pc.printf("Printing Data to server: %s\n\r",buf);
	char buf_2[] = "DOOR:12345678\n\r\0";
	buf_2[5] = ('0' + int(door_status[0]));buf_2[6]=('0' + int(door_status[1]));buf_2[7] = ('0' + int(door_status[2]));buf_2[8]=('0' + int(door_status[3]));
	buf_2[9] = ('0' + int(door_status[4]));buf_2[10]=('0' + int(door_status[5]));buf_2[11] = ('0' + int(door_status[6]));buf_2[12]=('0' + int(door_status[7]));
	bldg_client.send(buf_2,strlen(buf_2));
	pc.printf("Printing Data to server: %s\n\r",buf_2);

}
