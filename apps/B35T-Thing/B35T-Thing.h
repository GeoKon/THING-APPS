// ---------------------------------------------------------------------------------
// Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
// You may use this code as you like, as long as you attribute credit to the author.
// ---------------------------------------------------------------------------------

#ifndef B35T-THING_H
#define B35T-THING_H

// Includes needed for all app modules

    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>   // used ONLY IN AP MODE
    
    #include <cliClass.h>                               // needed for the cli.XXX()
    #include <timeClass.h>                              // needed for the ModuloTic(). This program DOES NOT USER timer
    #include <eepClass.h>
    #include <eepTable.h>       // needed for the etable()
    #include <oledClass.h>
    #include <owonClass.h>      // needed for the owon.XXX()

    #include "SimpleSRV.h"          // baseline for any web-server application. Used ONLY in AP Mode

// ---------------- Exported by BT35-APMode.cpp to main() -----------------------------

    void setupAP();
    void loopAP();
    void apCallbacks();
    
// ---------------- Exported Class & Functions from BT35T-Interact --------------------

    enum oled_t
    {
        SCREEN_INIT = 0,
        SCREEN_WAITFOR_CLI,
        
		SCREEN_TIMEOUT,
		
		SCREEN_CLI_MODE,
		SCREEN_STA_MODE,
		SCREEN_AP_MODE,
       
        SCREEN_LOOP_READY,
		
        SCREEN_LOOP_STATUS,
        SCREEN_LOOP_VALUES,
        LAST
    };

    void show( oled_t code, int arg=0 );
    
    class ThingUO
    {
    private:                        // all are set by SCREEN_INIT
        bool large;                 // true for 128x64, false for 128x32    
        int screen_count;           // number of screens. 0=off, 1=default, 2...
        int screen_state;           // flips to number of screens
            
    public:
        void showBase( oled_t code, int arg=0 );
        void showLoopValues();            // what to display while looping (the 3rd screen)
    
        void showLoopScreen();
        void flipLoopScreen();          // advance to next screen. Called by push button. Uses ... for number of screens
    };
    extern ThingUO tuo;

#endif
