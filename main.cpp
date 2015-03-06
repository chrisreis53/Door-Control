/*
*
*  Test program for W5500 mbed Library
*
*/
#include "mbed.h"
#include "WIZnetInterface.h"
#include "stm32f10x_iwdg.h"
 
#define ECHO_SERVER_PORT    9999
#define BLDG_SERVER_IP      "192.168.0.21"

void check_stream(char* buf);
void f_ethernet_init();
void set_doors(int a,int b,int c, int d,int e,int f,int g,int h);

DigitalOut DOOR_1(PC_6);
DigitalOut DOOR_2(PC_7);
DigitalOut DOOR_3(PC_8);
DigitalOut DOOR_4(PC_9);
DigitalOut DOOR_5(PA_8);
DigitalOut DOOR_6(PA_15);
DigitalOut DOOR_7(PC_10);
DigitalOut DOOR_8(PC_11);
 

SPI spi(PB_15,PB_14,PB_13); // mosi, miso, sclk
WIZnetInterface eth(&spi, PB_12, PB_0); // spi, cs, reset
Serial pc(PA_9,PA_10);
DigitalOut led(PC_4);

// for static IP setting
const char * IP_Addr    = "192.168.1.120";
const char * IP_Subnet  = "255.255.255.0";
const char * IP_Gateway = "192.168.1.111";

char DOOR[8];
int attempt=1;
int max_attempts = 10;
int ret;
char data[512];

 
int main()
{

    set_doors(1,1,1,1,1,1,1,1);
    wait(2);
    set_doors(0,0,0,0,0,0,0,0);
    
    f_ethernet_init();    
    TCPSocketConnection bldg_client;
    bldg_client.set_blocking(true,1);

    while(attempt <=  max_attempts){
        //TODO watchdog_refresh();
        //TODO egress_button();
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
            // TODO watchdog_refresh();     //Kick the dog
            int n;
            //pc.printf("Sending Data\n\r");
            bldg_client.send("ID",2);
            bldg_client.send(eth.getMACAddress(),strlen(eth.getMACAddress()));
            //pc.printf("Receiving Data\n\r");
            n = bldg_client.receive(data,512);
            if(n < 0) break;
            //pc.printf("Data: %s\r\n",data);
            //TODO egress_button();
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
    pc.printf("Connecting to building server @ %s on port %d",BLDG_SERVER_IP, ECHO_SERVER_PORT );
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

void check_stream(char* buf)
{

    for(int i = 0; i<strlen(buf); i++) {

        if(buf[i] == 'D' && buf[i+1] == 'O' && buf[i+2] == 'O' && buf[i+3] == 'R' && buf[i+4] == ':') {
            pc.printf("%d,%d,%d,%d,%d,%d,%d,%d",buf[i+5],buf[i+6],buf[i+7],buf[i+8],buf[i+9],buf[i+10],buf[i+11],buf[i+12]);
            set_doors(buf[i+5]-'0',buf[i+6]-'0',buf[i+7]-'0',buf[i+8]-'0',buf[i+9]-'0',buf[i+10]-'0',buf[i+11]-'0',buf[i+12]-'0');
            
        }

    }
}

void set_doors(int a,int b,int c, int d,int e,int f,int g,int h){
    
    DOOR_1 = a;
    DOOR_2 = b;    
    DOOR_3 = c;
    DOOR_4 = d;
    DOOR_5 = e;
    DOOR_6 = f;
    DOOR_7 = g;
    DOOR_8 = h;
}