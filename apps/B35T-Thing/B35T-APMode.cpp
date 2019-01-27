
#include "B35T-Thing.h" 

// -------------- Exported by main program ONLY WHEN USING AP MODE --------------------

    extern CPU cpu;
    extern OLED oled;
    extern EXE exe;
    extern ModuloTic owonTic;
    extern ModuloTic oledTic;
    extern ESP8266WebServer server;
    
// --------------------------------- AP SETUP() CODE ---------------------------------
void setupAP()
{
    show( SCREEN_AP_MODE );
    cpu.led( ON );

    if( !WiFi.softAP("GKE_AP") ) 
        cpu.die( "Cannot start AP Mode" );
    
    setTrace( T_REQUEST | T_JSON );    
    srvCallbacks();
    apCallbacks();
    
    server.begin( 80 );
    delay( 2000 );
    show( SCREEN_LOOP_READY );
}

// -------------------------------- AP main loop() ------------------------------------------
void loopAP() 
{
    static int connected = -1;
    
    int numbconx = WiFi.softAPgetStationNum();
    if( connected != numbconx )
    {
        PF("%d connections\r\n", connected = numbconx );
        cpu.led( OFF );
        delay( 300 );
        cpu.led( ON );
    }
    if( cpu.buttonPressed() )                       // Flip OLED screen if button is pressed
        tuo.flipLoopScreen();                       // (see BT35T-Interact.cpp)

    if( oledTic.ready() )                           // Update OLED screen with current readings
        tuo.showLoopScreen();                       // (This runs asynchronously from measurements)

    if( owonTic.ready() )                           // execute this every N-seconds
    {
          owon.readMeter();                           // request to read the meter. Block until read.
//        if( my.debug )
            PF("\t\t\t\t\tMeter=%s %s\r\n", 
                owon.getValueText(), owon.getUnits());
    }
    server.handleClient();
}
// ---------------------- SUPPORT FUNCTIONS FOR AP MODE -------------------------------

BUF cliresp(2000);               // allocate buffer for CLI response

char *landing = "<head><title>AP Mode</title><style>body {text-align: center;}</style></head>"
                "<body><h1>Root AP Services</h1>"
                "<h2><a href=\"owmeter.htm\">Display meter. (Use /owon to fetch reading in JSON)</a></h2>"
                "<h2><a href=\"SimpleSRV.htm\">Help (SimpleSRV Endpoints)</a></h2>"
                "<h2><a href=\"setap.htm\">Set WiFi Credentials</a></h2>"
                "<h2><a href=\"webcli.htm\">WEB CLI Mode</a></h2>"
                "<h2><a href=\"dir\">List of files (/dir)</a></h2>"
                "<h2><a href=\"upload\">Upload .htm file (/upload)</a></h2>"
                "<h2><a href=\"reset\">Reboot (/reset)</h2>"
                "</body>";

void apCallbacks()
{
    server.on("/", [](){ server.send(200, "text/html", landing ); });

    server.on("/setap.htm", HTTP_GET,       // Interactive End Point: /setap?ssid=xx&pwd=yy
    [](){
        String path = server.uri();
        if( server.args() > 1 )
        {
            String Sssid = server.arg(0);
            String Spwd  = server.arg(1);
            PF("Entered %s and %s\r\n", !Sssid, !Spwd );
            strcpy( eep.wifi.ssid, !Sssid );
            strcpy(  eep.wifi.pwd, !Spwd ); 
        }
        if( !handleFileRead( server.uri()) )
            server.send(404, "text/plain", "File /setap.htm");   
    }); 

    server.on("/owon", HTTP_GET,     // get OWON reading
    [](){
      showArgs();
      //String json("",200);
      BUF json(200);
      
      json.set( "{" );
      json.add( "'reading': %6.3f,",owon.getValue() );
      json.add(   " 'text': '%s',", owon.getValueText() );
      json.add(  " 'units': '%s',", owon.getUnits(true) );
      json.add(   " 'acdc': '%s',", owon.getACDC() );
      json.add(   " 'type': '%s',", owon.getType() );
      json.add(  " 'error': %d",    owon.error() );
      json.add( "}" );
      
      showJson( json.quotes() );  
      server.send(200, "text/json", json.quotes() );
    });

    server.on("/webcli.htm", HTTP_GET,              // when command is submitted, webcli.htm is called  
    [](){
        showArgs();
        if( server.args() )                             // if command given
        {
            cliresp.init();                             // initialize the response buffer
            //cliresp.set("(Max:%d bytes)\r\n", cliresp.maxsiz );
            exe.dispatch( !server.arg(0), &cliresp );   // command is executed here and response is saved in 'cliresp' buffer
            cliresp.add("(Used:%d bytes)\r\n", cliresp.length());
        }        
        showJson( !cliresp );                           // show the same on the console
        handleFileRead("/webcli.htm" );                 // reprint same html
    });
    
    server.on("/clirsp", HTTP_GET,      
    [](){
        showArgs();
        showJson( !cliresp );
        server.send(200, "text/html", !cliresp );
    });
}
