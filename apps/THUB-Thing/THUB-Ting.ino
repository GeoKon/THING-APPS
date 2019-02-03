// ---------------------------------------------------------------------------------
// Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
// You may use this code as you like, as long as you attribute credit to the author.
// ---------------------------------------------------------------------------------

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ThingerESP8266.h>
#include <FS.h>

#include <cpuClass.h>           // baseline CPU class
#include <timeClass.h>          // Used for ModuloTic()
#include <prsClass.h>           // parser class to decode JSON
#include <eepClass.h>           // eeprom parameters for WiFi
#include <eepTable.h>

#define USERNAME "GeoKon"
#define DEVICE_ID "THERMOSTATS"
#define DEVICE_CREDENTIAL "success"

// ------------------ CONSTANTS & DECLARATIONS ------------------------------

    #define T_REQUEST   1       // trace bit masks   
    #define T_RECEIVE   2

    #define SCAN_INTERVAL 10    // how often (in seconds) to take a measurement
    #define SCAN_COUNT 4        // number of URLs to get data from
    #define DROPBOX_PERIOD 30   // how often (in minutes) to save in DROPBOX
    
// --------------------- Forward references ---------------------------------

    bool parseEng( char *text );
    bool parseRdt( char *text );
    bool get_client();    
    void read_client();
    void sendtoDropbox();
        
// ---------------------------- Globals -------------------------------------

    ThingerESP8266 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
    HTTPClient http;

    CPU cpu;
    EEP eep;
    PRS pr( 100 );                  // working buffer for parser
    ModuloTic tic(SCAN_INTERVAL);   // how often to poll URLs

    char *homeurl = "http://192.168.0.";    // to be completed wuth IP3

    static struct state_t       // global parameters
    {
        int trace;
        int totalscans;         // total scans

        int dboxTm;
        bool dboxOn;
        
        int scanindex;          // 0, 1, ..., modulo-1
        
    } my = { 7, 0, DROPBOX_PERIOD, false, 0 };

    static void nextScan() { my.scanindex++; if( my.scanindex >= SCAN_COUNT ) my.scanindex=0; }
    
    static struct rdt_t         // per URL measurements
    {
        const char *label;      // label for this sensor
        const char *labIP3;     // label for IP3 of this sensor
        int  ip3;               // subnet ip[3];
        char *urlend;           // what to append after ip3. OVER-WRITTEN BY EEP VALUES
        float previous;         // previous measurement
        float measurement;      // GET measurement
        int   errorcount;       // number of not succcesful readings
        void (*handler)();      // function to call to handle parsing
        
    } rd[ SCAN_COUNT ] = {
                            { "OfficeTemp",     "OfficeIP3",     75, "/tstat",           0.0, 0.0, 0, read_temp },
                            { "Humidity",       "HumidityIP3",   75, "/tstat/humidity",  0.0, 0.0, 0, read_humidity },
                            { "BackYardTemp",   "BackYardIP3",   61, "/tstat",           0.0, 0.0, 0, read_temp },
                            { "PatioTemp",      "PatioIP3",      63, "/tstat",           0.0, 0.0, 0, read_temp }
                         };

    struct myparm_t
    {
        int ip3[ SCAN_COUNT ];  

    } myeep, defaults = { 75, 75, 61, 63 };
    #define FORMAT "OfficeIP3:%d HumidityIP3:%d BackyardIP3:%d ParioIP3:%d\r\n"

// Expected response {"state":"DISCH", "time":"06:26:43", "volts":13.500, "amps":-165.1, "watts":-2229, "totalWs":-987815808}
// see client here: https://github.com/esp8266/Arduino/blob/dd81336b79ddf15925876b983af13816d9d5807e/libraries/ESP8266HTTPClient/src/ESP8266HTTPClient.cpp
// tutorial: https://techtutorialsx.com/2016/07/21/esp8266-post-requests/

// ------------------------ elapsed time class ------------------------------
    class ELAPSE
    {
    private: 
        unsigned long T0;
    public:
        void start() { T0=millis(); }
        void printURL( char *url, int err )
        {
            if( my.trace & T_REQUEST )
                PF("[%12s] ΔT=%5ldms httpErr=%3d URL=%s\r\n", rd[my.scanindex].label, millis()-T0, err, url );
        }
        void printResult( char *rcv, int err )
        {
            if( my.trace & T_RECEIVE )
                PF("[%12s] ΔT=%5ldms ParsErr=%3d RCV=%s\r\n", rd[my.scanindex].label, millis()-T0, err, rcv );
        }
    } elapse;
// -------------------------------- setup() ---------------------------------
void setup() 
{
    cpu.init();
    SPIFFS.begin();

    eep.registerUserParms( &myeep, sizeof( myparm_t ), FORMAT );
    eep.registerInitCallBk( [](){ myeep=defaults; } );
    
    if( eep.fetchParms() )             // fetch EEPROM parameters
    {
        eep.initWiFiParms();            // initialize with default WiFis
        eep.initUserParms();
        eep.saveParms();         
    }
    eep.updateRebootCount();
    eep.printParms("Current Parms");    // print current parms
    
    PR("Press BUTTON within 5sec to initialize with default IPs");
    for( int i=0; i<500; i++ ) 
    {
        if( cpu.buttonPressed() )
        {
            for(int i=0; i<SCAN_COUNT; i++ )    // overwrite the rd[].ip3 parms
                myeep.ip3[i]=rd[i].ip3;
            PR("Initialized with default IP Addresses");
            break;
        }
        delay( 10 );
    }
    for(int i=0; i<SCAN_COUNT; i++ )    // overwrite the rd[].ip3 parms with EEPROM
        rd[i].ip3=myeep.ip3[i];
    
    thing.add_wifi( eep.wifi.ssid, eep.wifi.pwd );
    
    // ------------ FROM THE Thinker.io TO THE DEVICE -------------------------
    
    thing["trace"]    << inputValue( my.trace  );
    thing["dboxOn"]   << inputValue( my.dboxOn );
    thing["dboxTm"]   << inputValue( my.dboxTm );
    
    thing[ (const char *)rd[0].labIP3 ] << inputValue( rd[0].ip3, {myeep.ip3[0] = rd[0].ip3; eep.saveParms( USER_PARMS ); });
    thing[ (const char *)rd[1].labIP3 ] << inputValue( rd[1].ip3, {myeep.ip3[1] = rd[1].ip3; eep.saveParms( USER_PARMS ); });
    thing[ (const char *)rd[2].labIP3 ] << inputValue( rd[2].ip3, {myeep.ip3[2] = rd[2].ip3; eep.saveParms( USER_PARMS ); });
    thing[ (const char *)rd[3].labIP3 ] << inputValue( rd[3].ip3, {myeep.ip3[3] = rd[3].ip3; eep.saveParms( USER_PARMS ); });

    // ------------ FROM THE DEVICE TO Thinker.io -----------------------------
   
    thing["reading"] >> [](pson& ot )
    {
        ot[ (const char *)rd[0].label ] = rd[0].measurement;
        ot[ (const char *)rd[1].label ] = rd[1].measurement;
        ot[ (const char *)rd[2].label ] = rd[2].measurement;
        ot[ (const char *)rd[3].label ] = rd[3].measurement;

        ot[ "scan"   ] = rd[my.scanindex].label;

        BUF bf(60);
        float scale = 100.0/(float) my.totalscans;
        for( int i=0; i<SCAN_COUNT; i++ )
            bf.add( "%c:%.0f%%  ", rd[i].label[0], (float) rd[i].errorcount*scale );

        ot[ "errors" ] = (const char *)bf.c_str();
    };
    PR("Entering Loop");
}
// -------------------------------- loop() ---------------------------------

void loop() 
{
    static bool ok;
        
    if (WiFi.status() != WL_CONNECTED) 
    {
        PR( "Waiting for WiFi Connection...\r\n");
        delay(5000);
        return;  
    }
    if( tic.ready() && (!ok) )              // trigger a request
    {
        nextScan();                         // prep for next scan 0 ... SCAN_COUNT-1
        my.totalscans++;                    // if error, does not proceed with parsing 
        
        ok = get_client();                  // GETs URL; increments [my.scanindex].errorcount
    }
    if( ok )
    {
        (*rd[ my.scanindex ].handler)();    // parses JSON; increments [my.scanindex].errorcount
        thing.stream( thing["reading"] );   // report to Thinker.io every SCAN_INTERVAL
        sendtoDropbox();
        ok = false;
    }
    thing.handle();
}
bool get_client()
{
    char url[80];     // URL construction
    sprintf( url, 
        "%s%d%s", homeurl, rd[my.scanindex].ip3, rd[my.scanindex].urlend );

    elapse.start();
    http.begin( url );              // ---------- TCP SESSION STARTS --------------
    //http.setTimeout( 2000 );
    int httpCode = http.GET();
    elapse.printURL( url, httpCode );

    if( httpCode == HTTP_CODE_OK )
        return true;
    rd[ my.scanindex ].errorcount++;
    return false;
}
void read_temp()
{
    elapse.start();
    String payload = http.getString();
    http.end();                         // ---------- TCP SESSION ENDS --------------

    INIT( !payload );
    LCURL;
    FIND("temp");
    COLON;
    rd[ my.scanindex ].measurement = ERROR ? rd[ my.scanindex ].previous : (rd[ my.scanindex ].previous = atof( COPYD ));
    
    elapse.printResult( (char *)payload.c_str(), ERROR );

    if( ERROR )
        rd[ my.scanindex ].errorcount++;
}
void read_humidity()
{
    elapse.start();
    String payload = http.getString();
    http.end();                         // ---------- TCP SESSION ENDS --------------

    INIT( !payload );
    LCURL;
    FIND("humidity");
    COLON;
    rd[ my.scanindex ].measurement = ERROR ? rd[ my.scanindex ].previous : (rd[ my.scanindex ].previous = atof( COPYD ));

    elapse.printResult( (char *)payload.c_str(), ERROR );

    if( ERROR )
        rd[ my.scanindex ].errorcount++;
}
void sendtoDropbox()
{
    static int count = 0; 
    pson data;
    if( !my.dboxOn ) 
    {
        count = 0;
        return;                             // do nothing if not enabled
    }
    if( count == 0 )
    {
        BUF buf(100);                       // temp buffer
        for( int i=0; i<SCAN_COUNT; i++ )
            buf.add( "\t%c:\t%5.0f", rd[i].label[0], rd[i].measurement );
        data["value1"] = (const char *) buf.pntr;
        PF("Dropbox %s\r\n", buf.pntr );
        thing.call_endpoint( "IFTTT_TEMPS", data );
    }
    count++;    // increments by one, every time a measurement is taken, METER_READING_PERIOD
    if( count > ((my.dboxTm*60)/SCAN_INTERVAL) )
        count = 0;
}
