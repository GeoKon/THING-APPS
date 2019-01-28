// ---------------------------------------------------------------------------------
// Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
// You may use this code as you like, as long as you attribute credit to the author.
// ---------------------------------------------------------------------------------

#include <ESP8266WiFi.h>
#include <FS.h>
#include <ThingerESP8266.h>

#include  <cpuClass.h>
#include  <eepClass.h>
#include  <eepTable.h>
#include  <cliClass.h>

// ---------------------- DEFAULT INITIALIZATION PARMS ------------------
    #define USERNAME "GeoKon"
    #define DEFAULT_DEVICE_ID "SFGKE00"
    #define DEVICE_CREDENTIAL "success"
    
// ------------------------ HARDWARE SPECIFIC ---------------------------
/*
 * ESP8266 PIN FUNCTION  GPIO  CONNECTED TO
9 – MTMS  GPIO  GPIO14  J1 Pin 5
10 – MTDI GPIO  GPIO12  Relay (HIGH to turn on)
12 – MTCK GPIO  GPIO13  LED (LOW to turn on)
15 – GPIO0  Flash GPIO0 Tactile switch (LOW when switch is pressed)
25 – RDX  UART RXD  GPIO3 J1 Pin 2
26 – TXD  UART TXD  GPIO1 J1 Pin 3

MODIFCATIONS BY GK:
1.  Install MOSFET 2N7002 at Q3 position.
    Install 10k R2. RED LED is entirily driven by the RF module.
    Output of the RF Module is D1 (pin 9) of the RF Module
    RF Module is programmed independently from base board
    
2.  GPIO-04 is used as INPUT. Solder on the processor pin 16 
    Snake to the top of the board.
 */

//Inputs
    #define GPIO00  0
    #define GPIO02  2       // used in Rev 2 of the Basic instead of GPIO14
    #define GPIO04  4
    #define GPIO05  5
    #define GPIO14 14       // 5th pin on the header
    #define BUTTON GPIO00
    
// Outputs
    #define LED      13
    #define RELAY    12

// --------------------------------- COMMAND TABLES ---------------------------------
    #define CLI_WAITSEC 5
    extern COMMAND htable[], stable[];
    COMMAND *ToTables[] = { htable, &etable[1]/* skip 1st entry */, &stable[0], NULL };
    void printHelp( int argc, char *argv[] ){ printToT( ToTables ); }
    COMMAND htable[]=
    {
        {"h", "help", printHelp },
        {NULL, NULL, NULL}
    };
    char *help = 
    "\tUse 'eprint' to see settings first\r\n"
    "\tUse 'devID SFGKExx' to set the Thinger ID\r\n"
    "\tUse 'userID unitXX' to set the IFTTT label\r\n"
    "\tUse 'altCntr 0=none|1=GPIO14|2=GPIO4' to set the RF alternative control\r\n"
    "\tUse 'flag 1' to enable IFTTT\r\n"
    "\tThen reset\r\n";           
    
// ------------------------- THING SPCIFIC ----------------------------------

    ThingerESP8266 thing(USERNAME, DEFAULT_DEVICE_ID, DEVICE_CREDENTIAL);

//--------- Class with the global variables and initialization --------------
    
    class Globals
    {
    public:
        bool toggle;
        int wifiOK;     // 0 = trying to connect
                        // 1 = wifi found and connected
                        //-1 = wifi not found after N-attempts and operating in 'direct mode'
        Globals()
        {
            toggle  = false;
            wifiOK = false;
        }
    } my;

    struct MyEEP         // my parameters
    {
        char label[16];
        char devID[16];
        int  altcntr;     // 0=no RF, 1=Use basic v1, 2=use basic v2
        int  flags;       // bit0=IFTTT
        
    } myeep;
    #define FORMAT "UnitID=%s, DevID=%s, altCntr=%d, flags(IFTTT)=%d\r\n"
    void myDefaults()
    {
        strcpy( myeep.label, "Unit00"  );
        strcpy( myeep.devID, "SFGKE00" );
        myeep.altcntr = 0;
        myeep.flags   = 0;
    }
    char *DevLabel = "Unit01";      // device label. EEPROM DOES NOT WORK!
      
//------------------ Forward references ---------------------------------------

    int checkConnection();
    void turnOnOff( bool OnOff, bool notify );
    void turnDirectOnOff( bool OnOff );

//------------------ External references ---------------------------------------

    void setupAP();
    void loopAP();
 
// ----------------------------- Main Setup -----------------------------------
void setup() 
{
//  1. Initialize the hardware
//  --------------------------

    cpu.init( 115200, LED+NEGATIVE_LOGIC, BUTTON+NEGATIVE_LOGIC /* push button */ );
    
    pinMode( LED, OUTPUT);  digitalWrite( LED, HIGH );  // negative logic
    pinMode(RELAY, OUTPUT); digitalWrite( RELAY, LOW ); // positive logic
    
    pinMode(GPIO14, INPUT);
    pinMode(GPIO02, INPUT);
    pinMode(GPIO04, INPUT);
    pinMode(GPIO05, INPUT);
    
    if( !SPIFFS.begin() )
        cpu.die("Cannot initialize FS");

//  2. Retrieve EEPROM parameters and initialize as necessary
//  ---------------------------------------------------------
    eep.registerUserParms( &myeep, sizeof( myeep ), FORMAT );
    eep.registerInitCallBk( myDefaults );
    if( eep.fetchParms() )                      // if an error occurs fetching parameters...
    {
        cpu.blink( 3 );                         // three more blinks indicating re-initialization
        eep.initWiFiParms();                    // use the default WiFi
        eep.initUserParms();                    // initialize user parms
        eep.saveParms();
    }
    eep.updateRebootCount();

    PF( !eep.getParmString("----- Current EEPROM Parms") );
    PR( "----- Current INPUT bits" );
    PF( "GPIO02:%d GPIO04:%d GPIO014:%d\r\n", digitalRead( GPIO02), digitalRead( GPIO04 ), digitalRead( GPIO14 ));
   
    PN( help );                                             // print extensive help

    my.wifiOK = 0;      // define initial state of WiFi

//  3. Optionally enter in CLI mode or RELINTIALIZE
//  -----------------------------------------------
    //PF( "----- Within %dsec: press CR for CLI, or push BUTTON for factory defaults\r\n", CLI_WAITSEC );
    PF( "----- Within %dsec: press CR for CLI, or push BUTTON for AP Setup (192.168.4.1)\r\n", CLI_WAITSEC );
    cli.init( ECHO_ON, "cmd: ", true/*flush on*/); 
    for( int i=0; i<CLI_WAITSEC*100; (delay(10), i++) )     // wait for 2-seconds until RETURN is detected    
    {
        if( Serial.read() == 0x0D )
        {
            PF("----- CLI MODE!\r\n");
            interact( ToTables );
        }
        if( cpu.button() )
        {
            my.wifiOK=2;
            setupAP();
            return;
            
            cpu.blink( 3 );
            while( cpu.button() )                           // wait until released
                ;
            eep.initWiFiParms();                            // re-initialize
            eep.initUserParms();
            eep.saveParms();    
        }
    }
//  4. Define all controls, i.e. internet to change embedded states
//  ---------------------------------------------------------------

    if( strncmp( myeep.devID, "SFGKE", 5 )==0 ) // double check correct DeviceID
    {                                          
        thing.set_credentials( USERNAME, myeep.devID, DEVICE_CREDENTIAL );
        PF("Thing DEVICE_ID is %s\r\n", myeep.devID );
    }

    PF("----- Initializing THING\r\n");
    thing.add_wifi( eep.wifi.ssid, eep.wifi.pwd );  // initialize Thinker WiFi
 
    thing["led"]   << invertedDigitalPin( LED ); 

    thing["relay"] << [](pson& in )       // control switch from the Internet
    {   
        if( in.is_empty() )
            in=my.toggle;
        else
        {
            my.toggle=in;
            turnOnOff( my.toggle, myeep.flags&1 ); // this does IFTTT
        }
    };
    thing["external"] = []()       // control switch from the Internet
    {   
        PF("*** external called ****\r\n");
    };
    thing["extJson"] << [](pson &in)       // control switch from the Internet
    {   
        PF("Received from IFTTT Value1=[%s], Value2=[%s]\r\n", 
            (const char *)in["value1"], (const char *)in["value2"] );

        const char *p1;
        p1 = (const char *)in["value1"];
        if( strcmp( p1, "On" ) == 0 ||
            strcmp( p1, "1" )==0 )
                turnOnOff( true, false );
        if( strcmp( p1, "Off" ) == 0 ||
            strcmp( p1, "0" )==0 )
                turnOnOff( false, false );            
    };

//  4. Define all status reporting, i.e. from device to internet
//  ------------------------------------------------------------
        
    thing["button"] >> outputValue( cpu.button() );
    
    thing["status"] >> [](pson& ot )       // meter is read asynchronously by the loop() every N-seconds
    {   
        ot["toggle" ] = my.toggle ? 1 : 0;          // status ON/OFF
        ot["RFON"   ] = altControl();
    };

//  5. Handle interactive CLI (special handling)
//  --------------------------------------------
   
    thing["CLI"]  = [](pson& cli, pson& res)           // resets energy, including grand total
    {
        if( cli.is_empty() )
        {
            cli["cmd"] = "replace with cmd:";
            res = "";                                  // nothing to return
        }
        else
        {
            char cmd[40];
            strncpy( cmd, (const char *)cli["cmd"], 40 );
            
            rbuf.init();                                // clear response buffer
            execTables( ToTables, cmd );                // result is in buf;
            PF("%s", rbuf.pntr );
            res = (const char *)rbuf.pntr;
        }
    };
    PF("Hit RETURN to activate CLI during normal operation\r\n");
}

//  6. Main loop. Pool for Serial ready and controlled parameters
//  -------------------------------------------------------------
void loopWiFiOK();      
void loopNoWiFi();

void loop()
{
    int ok;
    
    if( my.wifiOK==1 )
        loopWiFiOK();                   // normal WiFi operation

    else if( my.wifiOK==2 )
        loopAP();                       // AP mode
        
    else if( my.wifiOK==-1 )
        loopNoWiFi();                   // direct control operation
    
    else                                // check if wifi connection is possible
    {
        my.wifiOK = checkConnection();  // if this is either 1 or -1, the 'else' is not executed again
        thing.handle();                 // yield time to Thinger
    }
}
void loopWiFiOK()
{
    if( cli.ready() )           // handle serial interactions
    {
        char cmd[40];
        strncpy( cmd, cli.gets(), 40 );
        rbuf.init();                                // clear response buffer
        execTables( ToTables, cmd );                // result is in buf;
        PF( rbuf.pntr );                            // print results
        cli.prompt();
    }
    if( cpu.button() || altControl() )       // if BUTTON on SONOFF is pressed ...
    {
        PF("You pressed BUTTON (or RF Remote)\r\n");
        my.toggle = !my.toggle;  
        turnOnOff( my.toggle, myeep.flags&1 ); // this does the device streaming & IFTTT

        while( cpu.button() || altControl() )
        {
            if( cli.ready() )                       // do not get stuck here!
                break;
            thing.handle();
            delay( 20 );
            yield();
        }
    }
    thing.handle();             // yield time to Thinger
}
void loopNoWiFi()
{
    if( cli.ready() )                               // handle serial interactions
        PR("CLI disabled in NoWiFi (aka Direct Control) mode");

    if( cpu.button() || altControl() )              // if BUTTON on SONOFF is pressed ...
    {
        PF("You pressed BUTTON (or RF Remote)\r\n");
        my.toggle = !my.toggle;  
        turnDirectOnOff( my.toggle );               // no IFTTT
        
        while( cpu.button() || altControl() )
            yield();
    }
    yield();
}

//  Support functions called by the main loop
//  -----------------------------------------

static uint16_t counter = 0;
int checkConnection()  // checks if WiFi connected. Prints messages
{
    IPAddress ip;
    PF( "Trying to connect to %s with %s [%d]...\r\n", 
            eep.wifi.ssid, eep.wifi.pwd, counter++ );
    cpu.blink(1);
    
    if( counter >= 4 )  
    {
        PF( "----- NO WIFI!\r\n" );
        cpu.led( ON );
        return -1;
    }        
    ip = WiFi.localIP();
    if( ip[0] )     // now connected!
    {
        PF( "----- CONNECTED!\r\n" );
        PF( "IP=%d.%d.%d.%d RSSI=%ddBm\r\n", ip[0],ip[1],ip[2],ip[3], WiFi.RSSI()  );
        PF( "Heap (now=%d, max=%d)\r\n", cpu.heapUsedNow(), cpu.heapUsedMax() );

        cpu.blink( 1 );         // one more blink (2-total)
        return 1;
    }
    return 0;                   // continue trying
}

bool altControl()                               // True if one of the alternative controls is asserted
{
    switch( myeep.altcntr )
    {
       default: 
        return false; 
        break;
        case 1: // basic RF rev 1
            return digitalRead( GPIO14 );   // positive logic
        case 2: // basic RF rev 2
            return digitalRead( GPIO04 );   // positive logic
    }
}

void turnOnOff( bool OnOff, bool notify )       // Main routine to turn relay ON or OFF
{                                               // Also handles IFTTT notifications
    turnDirectOnOff( OnOff );
    thing.stream( thing["status"] );            // stream data to internet for status change
    if( notify )
    {
        pson data;
        data["value1"] = (const char *)myeep.label;
        data["value2"] = (const char *) OnOff ? "ON" : "OFF";
        thing.call_endpoint( "IFTTT_Sonoff", data );  // add here device ID
    }
}
void turnDirectOnOff( bool OnOff )       // Main routine to turn relay ON or OFF
{                            
    char *st;
    
    if( OnOff )
    {
        st = "ON";
        digitalWrite( RELAY, HIGH ); 
        cpu.led( BLINK, 2 ); 
    }
    else
    {
        st = "OFF";
        digitalWrite( RELAY, LOW ); 
        cpu.led( BLINK, 1 ); 
    }
    PF("\t\t\tRelay is %s\r\n", st );
}


// ------------------------------- CLI COMMAND HANDLERS -------------------------------------
static void cliSF_UserID( int n, char **arg)     
{
    if( n>1 )
    {
        strncpy( myeep.label, arg[1], 16 );
        eep.saveParms( USERPARM_MASK );
    }
    rbuf.add("User ID=%s\r\n", myeep.label );
}
static void cliSF_DevID( int n, char **arg)     
{
    if( n>1 )
    {
        strncpy( myeep.devID, arg[1], 16 );
        eep.saveParms( USERPARM_MASK );
    }
    rbuf.add("Device ID=%s\r\n", myeep.devID );
}
static void cliSF_altcntr( int n, char **arg)     
{
    if( n>1 )
    {
        myeep.altcntr = atoi( arg[1] );
        eep.saveParms( USERPARM_MASK );
    }
    rbuf.add("altCntr=%d\r\n", myeep.altcntr );
}
static void cliSF_flags( int n, char **arg)     
{
    if( n>1 )
    {
        myeep.flags = atoi( arg[1] );
        eep.saveParms( USERPARM_MASK );
    }
    rbuf.add("flags=%d\r\n", myeep.flags );
}

static void cliSF_btest( int n, char **arg)     
{
    if( n>1 )
        n = atoi( arg[1] );
    else 
        n = 1000;
    for( int i=0;i<n; i++ )
    {
        PF("GPIO0 (Flash)=%d, GPIO02=%d, GPIO04=%d, GPIO14=%d\r\n", 
                        digitalRead( GPIO00 ),
                        digitalRead( GPIO02 ),
                        digitalRead( GPIO04 ),
                        digitalRead( GPIO14 ) );
        delay( 200 );
    }
    rbuf.add("OK\r\n");    
}

COMMAND stable[]= // must be external to be able to used by the cliSupport
{
    {"userid",  "[newid] Shows current UserID or saves a new one", cliSF_UserID},
    {"devid",   "[devid] Shows/saves DeviceID used by Thinger",    cliSF_DevID},
    {"altcntr", "[0=no RF, 1=GPIO14, 2=GPIO04] select alternative RF control",  cliSF_altcntr},
    {"flags",   "[flags] Shows/saves flags (bit0=IFTTT)",          cliSF_flags},
    
    {"on",      "Turn ON",              [](int, char**){
                                            turnOnOff( true, false );
                                            rbuf.add("Forced ON\r\n");
                                            }},
    {"off",     "Turn OFF",             [](int, char**){
                                            turnOnOff( false, false );
                                            rbuf.add("Forced ON\r\n");
                                            }},
    
    {"btest",   "Reads FLASH and RF bits", cliSF_btest},

    {NULL, NULL, NULL}
};
