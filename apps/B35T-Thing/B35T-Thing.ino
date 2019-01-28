//#define _DEBUG_ 1
#define _DISABLE_TLS_

#define CLI_WAITSEC 10                                  // number of seconds to wait until CLI is bypassed
#define METER_READING_PERIOD 2                          // seconds. How often to read the meter
#define METER_DROPBOX_PERIOD 20                         // how often to save to dropbox
#define OLED_DISPLAY_PERIOD  2                          // seconds.
//#define ALERT_REPEAT_PERIOD (60*METER_READING_PERIOD)   // seconds. How long to wait for IFTTT repeats

// ------------------------ Include common to all app modules -----------------------

    #include "B35T-Thing.h"   
    #include <FS.h>                                     // includes only needed in this module
    #include <ThingerESP8266.h>
     
// --------- COMMAND TABLES & FORWARD REFERENCES OF THIS FILE -----------------------

    void setupCLI(); void loopCLI();
    void setupSTA(); void loopSTA();
    void sendtoDropbox();
    
    bool pb( char *s, int x );                          // print bool if debug is ON
    float pf( char *s, float f );                       // print float if debug is ON
    
    namespace myiTable                                  // see end of this file
    {
        extern void init( EXE &myexe );
        extern CMDTABLE table[]; 
    }

// --------------------------- THING SPECIFIC (STA MODE ONLY) -----------------------

    #define USERNAME "GeoKon"
    #define DEVICE_ID "OWONB35T"
    #define DEVICE_CREDENTIAL "success"
    #define SSID eep.wifi.ssid
    #define SSID_PASSWORD eep.wifi.pwd

// ------------------------ allocation of base classes ------------------------------

    CPU cpu;        // from GKESP-L1 library
    CLI cli;        // from GKESP-L1 library
    EXE exe;        // from GKESP-L1 library (cliClass.cpp)
    EEP eep;        // from GKESP-L1 library
    OLED oled;      // from GKESP-L1 library
    OWON owon;      // from GKESP-L2 library

    ThingerESP8266 thing(USERNAME,DEVICE_ID,DEVICE_CREDENTIAL); // used ONLY in STA MODE
    ESP8266WebServer server(80);                                // used ONLY in AP MODE
    
// --------------------------------- MY GLOBAL VARIABLES ----------------------------

    enum opmode_t                   // operating modes
    {
        CLI_MODE = 0,
        STA_MODE = 1,
        AP_MODE = 2
    };
    
    class GLB
    {
    public:
        bool debug;             // debug traces for Thinger
        opmode_t mode;          
        float simValue;     // simulated value 0...100
        bool  simulate;     // flag to activate or not simulation
        int dboxTm;
        bool dboxOn;
        GLB()
        {
            debug = simulate = dboxOn = false;
            mode  = CLI_MODE;
            simValue = 0.0;
            dboxTm = METER_DROPBOX_PERIOD;                      ;
        }
    } my;
// --------------------------------- SETUP() CODE ---------------------------------
void setup() 
{
	cpu.init();                                 // use default Baudrate, LED and BUTTONS
 	if( !SPIFFS.begin() )
        cpu.die("No SPIFFS");      				// initialize the SPI FS system
    
    if( int err = eep.fetchParms() )            // wifi and user fetch parms
    {
        eep.initWiFiParms();                    // initialize with default WiFis
        eep.saveParms();    
    }
    eep.updateRebootCount();
    eep.printParms("Current Parms");    // print current parms

    myiTable::init( exe );                      // associate myTable with the EXE class
    eepTable::init( exe, eep );                 // associate eepTable with the EXE class and EEP class
     
    exe.registerTable( myiTable::table );
    exe.registerTable( eepTable::table );
    
    owon.init( 1500 /* timeout for serial */, 115200/*pri baud*/, 2400/*sec baud*/);

 	show( SCREEN_INIT );
 	show( SCREEN_WAITFOR_CLI, CLI_WAITSEC );	// initialize screen; display CLI message
	cpu.led( OFF );

    for( int i=0; i<CLI_WAITSEC*100; i++ )              // loop every 10msec for CLI_WAITSEC
    {
        if( !(i%100) )                                  // update display every second
            show( SCREEN_TIMEOUT, CLI_WAITSEC-i/100 );

        if( Serial.read() == 0x0D ){ my.mode = CLI_MODE;  setupCLI(); return; }
        if( cpu.buttonPressed() )  { my.mode = AP_MODE;  setupAP(); return; }
        delay(10);
    }
    my.mode = STA_MODE;
    setupSTA();
}
void setupCLI()
{
    show( SCREEN_CLI_MODE );
    cpu.led( ON );
    cli.init( ECHO_ON, "cmd: " );
    cli.prompt();
}
void setupSTA()
{
    show( SCREEN_STA_MODE );
    cpu.led( OFF );
    
    thing.add_wifi(SSID, SSID_PASSWORD);
    
    // ----------------------- CONTROL: from Internet to meter --------------------------------

    thing["led"]      << invertedDigitalPin( 16 /* GPIO for LED */ ); 
    thing["debug"]    << inputValue( my.debug );
    thing["simulate"] << inputValue( my.simulate, { PF("Simulate=%d, Value=%.3f\r\n", my.simulate, my.simValue);});
    thing["simValue"] << inputValue( my.simValue );
    thing["dboxOn"]   << inputValue( my.dboxOn );
    thing["dboxTm"]   << inputValue( my.dboxTm );
    
    // ------------------------- READING from meter to Internet ------------------------------------

    thing["logging"] >> [](pson& ot ) // buckets
    {
        // owon.readMeter();
        // meter is read asynchronously by the loop() every N-seconds
        
        ot["meter"] = owon.getValue();
        ot["units"] = (const char *)owon.getUnits();
        PF("Logging %f\r\n", owon.getValue() );
    };    
    thing["reading"] >> [](pson& ot )
    {
        // owon.readMeter() is called by the main loop() every N-seconds
        // Readings are waiting in the OWON area to be sent out
        
        ot["value"   ] = owon.getValue();               // measurement exactly as displayed in meter
        ot["simValue"] = my.simValue;                   // simulated value

        if( my.simulate )
            ot["percent"] = my.simValue;
        else
        {
            if( owon.error() )
            {
                ot["string"] = "timeout";
                ot["acdcmax"] = "";
                ot["percent"] = 0.0;
            }
            else
            {
                char temp1[32], temp2[32];
                sprintf( temp1, "%s %s", owon.getValueText(), owon.getUnits());
                sprintf( temp2, "%s %s", owon.getACDC(), owon.getType());
                char *units = owon.getUnits( true /* in ASCII */);
                float scale = owon.getValue();
                
                ot["string"] = (const char*) temp1;
                ot["acdcmax"] = (const char*) temp2;
    
                if( strchr( units, 'C' ) )                  // degrees C -- max is 100
                    ;
                else if( strchr( units, 'F' ) )             // degrees F -- max is 212
                    scale *= 100.0/212.0;
                else if( strchr( units, 'z' ) )             // Hz -- max is 1000
                    scale /= 10.0;
                else if( strchr( units, 'h' ) )             // hFE -- max is 1000
                    scale /= 10.0;
                else if( strchr( units, '%' ) )             // duty cycle -- max is 100
                    ;
                else
                    scale = owon.getValue( true /* raw reading */)/60.0;
                    
                ot["percent"] = scale; 

                if( my.debug) 
                    PF( "Reading:%.0f, Units:%s, Scale:%f\r\n", owon.getValue( true ), units, scale );
            }
        }
    };    
    srvCallbacks();
    apCallbacks();
    server.begin( 80 );

    delay( 2000 );
    show( SCREEN_LOOP_READY );
}

// -------------------------------- main loop() ------------------------------------------

ModuloTic owonTic( METER_READING_PERIOD );        // how often to read the meter
ModuloTic oledTic( OLED_DISPLAY_PERIOD );         // how often to update the display

void loop() 
{
         if( my.mode == CLI_MODE ) loopCLI();
    else if( my.mode ==  AP_MODE ) loopAP();
    else if( my.mode == STA_MODE ) loopSTA();
    else
        ;
}
void loopCLI() 
{
    if( cli.ready() )
    {
        char *cmd = cli.gets();
        //PF("Command %s\r\n", cmd );
        exe.dispatch( cmd );
        cli.prompt();
    }
}
void loopSTA() 
{
    if( cpu.buttonPressed() )                       // Flip OLED screen if button is pressed
        tuo.flipLoopScreen();                       // (see BT35T-Interact.cpp)

    if( oledTic.ready() )                           // Update OLED screen with current readings
        tuo.showLoopScreen();                       // (This runs asynchronously from measurements)

    if( owonTic.ready() )                           // execute this every N-seconds
    {
        owon.readMeter();                           // request to read the meter. Block until read.
        if( my.debug )
            PF("\t\t\t\t\tMeter=%s %s\r\n", 
                owon.getValueText(), owon.getUnits());

        thing.stream( thing["reading"] );           // stream to Internet
        sendtoDropbox();                            // Check if logging is required, and do so
    }
    thing.handle();
    server.handleClient();    
}

// ---------------------- SUPPORT FUNCTIONS FOR CLI MODE ------------------------------
namespace myiTable
{
    static EXE *_exe;       //pointer to EXE class defined in main()
    
    void init( EXE &myexe ) {_exe = &myexe;}
    void help( int n, char *arg[] ) {_exe->help( n, arg );}

    #define RESPOND _exe->respond
    
    void trace( int n, char *arg[] ) 
    {
        if( n>1 )
            setTrace( atoi( arg[1] ) );
        RESPOND( "Trace = %d\r\n", getTrace() );
    }
    CMDTABLE table[]= 
    {
        {"h", "Help! List of all commands", help },
        {"trace", "1=FIO 2=REQ 4=RSP 8=ARGS", trace },
        {NULL, NULL, NULL}
    };
}

// ---------------------- SUPPORT FUNCTIONS FOR STA MODE ------------------------------
float pf( char *s, float f )
{
    if( my.debug )
        PF("%s (%.2f)\r\n", s, f );
    return f;
}
bool pb( char *s, int x )
{
    if( my.debug )
        PF("%s (%d)\r\n", s, x );
    return x;
}

void sendtoDropbox()
{
    static int count = 0; 
    char temp[60]; 
    pson data;
    if( !my.dboxOn ) 
    {
        count = 0;
        return;                             // do nothing if not enabled
    }
    if( count == 0 )
    {
        sprintf(temp, "%5.3f, %s, %s", owon.getValue(), owon.getUnits(), owon.getACDC() );
        data["value1"] = (const char *) temp;
        PF("Dropbox %s\r\n", temp );
        thing.call_endpoint( "IFTTT_B35T", data );
    }
    count++;    // increments by one, every time a measurement is taken, METER_READING_PERIOD
    if( count > (my.dboxTm/METER_READING_PERIOD ) )
        count = 0;
}
