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
#define BLDG_SERVER_IP      "192.168.1.104"

////Prototypes////
void check_stream(char* buf);
void f_ethernet_init();
void set_doors(int a,int b,int c, int d,int e,int f,int g,int h);
void read_egress(void);
void egress_timer(void);
void button_check(void);
void watchdog_init(void);
void watchdog_start(void);
void watchdog_refresh(void);
void watchdog_status(void);

////Pin Declarations////
DigitalOut DOORS[8]={PC_6,PC_7,PC_8,PC_9,PA_8,PA_15,PC_10,PC_11};	//D0,D1,D2,D3,D4D5,D6,D7
DigitalIn EGRESS[8]={PC_12,PD_2,PB_3,PB_4,PB_5,PB_6,PB_7,PB_8};		//D8,D9,D10,D11,D12,D13,D14,D15

DigitalOut Debug_LED_0(PC_4);
DigitalOut Debug_LED_1(PC_5);

////Constructors///
SPI spi(PB_15,PB_14,PB_13); 			// timer interface -> mosi, miso, sclk
WIZnetInterface eth(&spi, PB_12, PB_0); // WIZnet interface -> spi, cs, reset
Serial pc(PA_9,PA_10);					// USART interface -> RX, TX
IWDG_HandleTypeDef hiwdg;				//Watchdog timer
Timer t[8];								//timer
Ticker flipper;							//Interrupt ticker
TCPSocketConnection bldg_client;		//TCP Socket Connection

////Variables////
bool door_status[8]={false,false,false,false,false,false,false,false};
int attempt=1;
int max_attempts = 10;
int ret;
int watchTimeMs = 10000;
char data[512];
char buffer[512];

 
int main()
{

	while (1){
		attempt = 1;

		watchdog_init();
		watchdog_start();
		watchdog_refresh();
		f_ethernet_init();
		bldg_client.set_blocking(true,50);

		while(attempt <=  max_attempts){
			watchdog_refresh();

			pc.printf("\nAttempting to Connect to Server...\n\r");
			ret=bldg_client.connect(BLDG_SERVER_IP,ECHO_SERVER_PORT);
			if(!ret){
				pc.printf("\nConnected to Building Server\n\r");
				attempt = 1; //reset attempts
				flipper.attach(&button_check,0.5 );
			}else{
				pc.printf("Connection Failed...This is attempt #%d of %d\n\r",attempt,max_attempts);
				attempt++;
			}
			read_egress();
			egress_timer();
			set_doors(door_status[0],door_status[1],door_status[2],door_status[3],door_status[4],door_status[5],door_status[6],door_status[7]);

			while(bldg_client.is_connected()){
				watchdog_refresh();
				int n;
				//pc.printf("Sending Data\n\r");
				//bldg_client.send("ID ",2);
				//sprintf(buffer,"ID-%s-%d%d%d%d%d%d%d%d\0",eth.getMACAddress(),status[0],status[1],status[2],status[3],status[4],status[5],status[6],status[7]);

				//bldg_client.send(eth.getMACAddress(),strlen(eth.getMACAddress()));
				pc.printf("Receiving Data\n\r");
				n = bldg_client.receive(data,512);
				if(n < 0) break;
				pc.printf("Data: %s\r\n",data);

				check_stream(data);
				//pc.printf("Bldg: %s, Room: %s, Doors: %s\n\r", str_bldg, str_room, str_doors);
				read_egress();
				egress_timer();
				//set_doors(door_status[0],door_status[1],door_status[2],door_status[3],door_status[4],door_status[5],door_status[6],door_status[7]);
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
    pc.printf("Connecting to building server @ %s on port %d...",BLDG_SERVER_IP, ECHO_SERVER_PORT );
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
    
    DOORS[0] = a;
    DOORS[1] = b;
    DOORS[2] = c;
    DOORS[3] = d;
    DOORS[4] = e;
    DOORS[5] = f;
    DOORS[6] = g;
    DOORS[7] = h;

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
    		sprintf(buffer,"STATUS:%s:%d%d%d%d%d%d%d%d",eth.getMACAddress(),door_status[0],door_status[1],door_status[2],door_status[3],door_status[4],door_status[5],door_status[6],door_status[7]);
    		pc.printf(buffer);
    		bldg_client.send(buffer,strlen(buffer));
    		break;
    	}
    }

}

void read_egress(void){

	for(int i = 0; i<8;i++){

		if(!door_status[i] && EGRESS[i].read()){
			door_status[i] = false;
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
			door_status[i] = true;
			pc.printf("Timer %d has reached 5 seconds\r\n",(i+1));
		}

	}

}

void button_check(void){
	read_egress();
	egress_timer();
	set_doors(door_status[0],door_status[1],door_status[2],door_status[3],door_status[4],door_status[5],door_status[6],door_status[7]);
}

void watchdog_init(void){

	hiwdg.Instance = IWDG;
	hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
	hiwdg.Init.Reload = 1350; //10 seconds (1350)
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
