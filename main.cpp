#include "mbed.h"				//mbed standard library
#include "WIZnetInterface.h"	//Wiznet chipset interface
#include "stm32f4xx_hal_iwdg.h"	//Independant Watchdog Timer
#include "string.h"
#include "strings.h"


#define ECHO_SERVER_PORT	9999
#define	BLDG_SERVER_IP		"192.168.1.104"

WIZnetInterface eth(SPI_MOSI, SPI_MISO, SPI_SCK,SPI_CS,PB_4); // spi, cs, reset
Serial pc(SERIAL_TX,SERIAL_RX);

IWDG_HandleTypeDef hiwdg;	//Watchdog timer

DigitalOut SD_CS(PB_5);		// This is the chip select for the sd card which shares the SPI bus on the Arduino shield.
DigitalOut DOOR_1(PA_0,0);
DigitalOut DOOR_2(PA_1,0);
DigitalOut DOOR_3(PA_4,0);
DigitalIn EGRESS(PB_0);

//Prototypes
void f_ethernet_init(void);
void check_stream(char* buf);
void egress_button(void);
void led_check();
void watchdog_init(void);
void watchdog_start(void);
void watchdog_refresh(void);
void watchdog_status(void);


int ret;
bool status;
int max_attempts = 10;
char paq_en[64];
char * str0 = "<h1>Suck it Trebeck!</h1>";
char str_bldg[128];
char str_room[128];
char str_doors[128];
char * str_MAC = "MAC";
uint8_t mac[]={0x90,0xa2,0xDa,0x0d,0x42,0xe0};
char buffer[64];
char data_entry[64];
char data[512];
int attempt=1;
int entry=42;

int main()
{
	DOOR_1 = 1;
	DOOR_2 = 1;
	DOOR_3 = 1;
	wait(1);
	DOOR_1 = 0;
	DOOR_2 = 0;
	DOOR_3 = 0;

	watchdog_init();
	watchdog_status();
	watchdog_start();
// force the chip select for the SD card high to avoid collisions. We're not using the sd card for this program    

    SD_CS=1;
    //uint8_t mac[]={0x90,0xa2,0xDa,0x0d,0x42,0xe0};

    int length;
    f_ethernet_init();

    TCPSocketConnection bldg_client;
    bldg_client.set_blocking(true,1);


    while(attempt <=  max_attempts){
    	watchdog_refresh();
    	egress_button();
    	pc.printf("\nAttempting to Connect to Server...\n\r");
    	ret=bldg_client.connect(BLDG_SERVER_IP,ECHO_SERVER_PORT);
    	if(!ret){
    		pc.printf("\nConnected to Building Server\n\r");
    		attempt = 0; //reset attempts
    	}else{
    		pc.printf("Connection Failed...This is attempt #%d of %d\n\r",attempt,max_attempts);
    		attempt++;
    	}
    	while(bldg_client.is_connected()){
    		watchdog_refresh();		//Kick the dog
    		int n;
    		pc.printf("Sending Data\n\r");
    		bldg_client.send("ID",2);
    		bldg_client.send(eth.getMACAddress(),strlen(eth.getMACAddress()));
    		pc.printf("Receiving Data\n\r");
    		n = bldg_client.receive(data,512);
    		if(n < 0) break;
    		pc.printf("Data: %s\r\n",data);
    		egress_button();
    		check_stream(data);
    		//pc.printf("Bldg: %s, Room: %s, Doors: %s\n\r", str_bldg, str_room, str_doors);
    		wait(1);
    	}
    }
    
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
    wait(0.25);
    pc.printf(".");
    wait(0.25);
    pc.printf(".");
    wait(0.25);
    pc.printf(".\n\r");
    wait(0.25);
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

	for(int i = 0;i<strlen(buf);i++){
//		if(buf[i] == 'L' && buf[i+1] == 'E' && buf[i+2] == 'D'){
//			DOOR_1 = !DOOR_1;
//		}
//		if(buf[i] == 'D' && buf[i+1] == 'A' && buf[i+2] == 'T' && buf[i+3] == 'A'){
//			pc.printf("Data Received\n\r");
//		}
		pc.putc(buf[i]);
		if(buf[i] == 'D' && buf[i+1] == 'O' && buf[i+2] == 'O' && buf[i+3] == 'R' && buf[i+4] == '1' && buf[i+5] == '1'){
			pc.printf("Door 1 Engaged\r\n");
			DOOR_1 = 1;
		}
		if(buf[i] == 'D' && buf[i+1] == 'O' && buf[i+2] == 'O' && buf[i+3] == 'R' && buf[i+4] == '1' && buf[i+5] == '0'){
			pc.printf("Door 1 Disengaged\r\n");
			DOOR_1 = 0;
		}
	}
}

void egress_button(void){
	if(EGRESS){
		pc.printf("EGRESS DEPRESSED");
		DOOR_1=1;
		wait(5);
		DOOR_1=0;
	}
}

void watchdog_init(void){

	hiwdg.Instance = IWDG;
	hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
	hiwdg.Init.Reload = 1350; //10 seconds
	HAL_IWDG_Init(&hiwdg);

}

void watchdog_start(void){
	HAL_IWDG_Start(&hiwdg);
}

void watchdog_refresh(void){
	HAL_IWDG_Refresh(&hiwdg);
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
