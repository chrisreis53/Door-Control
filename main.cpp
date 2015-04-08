/*
*
*  AFRL Door Control Program
*
*/
#include "mbed.h"
#include "WIZnetInterface.h"
#include "stm32f1xx_hal_iwdg.h"
#include <string.h>

////Defines////
#define ECHO_SERVER_PORT    9999
//#define BLDG_SERVER_IP      "10.10.1.7"
#define BLDG_SERVER_IP      "192.168.1.103"

////Prototypes////
void check_stream(char* buf);
void f_ethernet_init();
void set_doors(int a,int b,int c, int d,int e,int f,int g,int h);
void read_egress(void);
void egress_timer(void);
void button_check(void);
void read_alarm(void);
void send_data(void);
void watchdog_init(void);
void watchdog_start(void);
void watchdog_refresh(void);
void watchdog_status(void);

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
		attempt = 1;

		watchdog_init();
		watchdog_start();
		watchdog_refresh();
		f_ethernet_init();
		door_server.bind(ECHO_SERVER_PORT);
		door_server.listen();
		bldg_client.set_blocking(false,50);
		door_server.set_blocking(false,50);
		data_timer.start();
		while(1){
			watchdog_refresh();
			button_check();
			connection_status = door_server.accept(bldg_client);
			Debug_LED_0=0;
			if(!connection_status){
				pc.printf("Received Client Connection\n\r");
				while(bldg_client.is_connected()){
					Debug_LED_1=0;
					watchdog_refresh();
					char data[512];
					int n;
					button_check();
					n = bldg_client.receive(data,512);
					if (n>0) {
						pc.printf("%s sent %s\n\r",bldg_client.get_address(),data);
						check_stream(data);
						if(data_timer.read()>5){
							send_data();
							data_timer.reset();
						}
					}

				}
				Debug_LED_1=1;
			}else{
				if(data_timer.read()>5){
					pc.printf("No Connection\n\r");
					data_timer.reset();
				}
			}
		}

	}
}

void f_ethernet_init()
{
	watchdog_refresh();
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
    pc.printf("Connecting to building server @ %s on port %d...\n\r",BLDG_SERVER_IP, ECHO_SERVER_PORT );
    ret = eth.connect();
    watchdog_refresh();
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
    watchdog_refresh();
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
    	if(DOORS[i]!=false){
    		sprintf(buffer,"STATUS:%d%d%d%d%d%d%d%d\n\r",door_status[0],door_status[1],door_status[2],door_status[3],door_status[4],door_status[5],door_status[6],door_status[7]);
    		pc.printf(buffer);
    		bldg_client.send(buffer,strlen(buffer));
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

	for(int i = 0;i<4;i++){
		if(ALARM[i].read()==1){
			alarm_status[i]=true;
			alarm_signal = true;
		}
	}

	if (alarm_signal){
		send_data();
	}


}

void send_data(void){

	char buf[64];
	sprintf(buf,"ALARM:%i%i%i%i\n\r\0",alarm_status[0],alarm_status[1],alarm_status[2],alarm_status[3]);
	bldg_client.send(buf,strlen(buf));
	char buf_2[64];
	sprintf(buf_2,"DOOR:%i%i%i%i%i%i%i%i\n\r\0",door_status[0],door_status[1],door_status[2],door_status[3],door_status[4],door_status[5],door_status[6],door_status[7]);
	bldg_client.send(buf_2,strlen(buf_2));

}

void watchdog_init(void){

	hiwdg.Instance = IWDG;
	hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
	hiwdg.Init.Reload = 2050; //10 seconds (1350)
	HAL_IWDG_Init(&hiwdg);

}

void watchdog_start(void){
	HAL_IWDG_Start(&hiwdg);
}

void watchdog_refresh(void){
	HAL_IWDG_Refresh(&hiwdg);
	Debug_LED_0 != Debug_LED_0;
}

void watchdog_status(void){
	switch (HAL_IWDG_GetState(&hiwdg)){
		case HAL_IWDG_STATE_RESET:
			pc.printf("IWDG not yet initialized or disabled\r\n");
			break;
		case HAL_IWDG_STATE_READY:
			pc.printf("IWDG initialized and ready for use\r\n");
			break;
		case HAL_IWDG_STATE_BUSY:
			pc.printf("IWDG internal process is ongoing\r\n");
			break;
		case HAL_IWDG_STATE_TIMEOUT:
			pc.printf("IWDG timeout state\r\n");
			break;
		case HAL_IWDG_STATE_ERROR:
			pc.printf("IWDG error state\r\n");
			break;
		default:
			pc.printf("Unknown state\n\r");
	}
}
