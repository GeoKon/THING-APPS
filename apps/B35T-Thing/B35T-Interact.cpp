// ---------------------------------------------------------------------------------
// Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
// You may use this code as you like, as long as you attribute credit to the author.
// ---------------------------------------------------------------------------------

// ------------------------ Include common to all app modules -----------------------
    
    #include "B35T-Thing.h"   

    extern CPU cpu;         // use main globals from main program

// ---------------------------------------------- OLED SHOW -------------------------------------------------
ThingUO tuo;
void show( oled_t code, int arg )
{
    tuo.showBase( code, arg );
}
static const char *echo( const char *s )
{
    if( isalnum( *s ) )
        PR( s );            // print with /r/n
    else
        PR( s+1 );
    return s;
}     
// ---------------------------------------------- OLED SHOW -------------------------------------------------
void ThingUO::showBase( oled_t code, int arg )
{
    IPAddress ip;
    static int ap_mode;	// 0=CLI, 1=STA, 2=AP
    
    switch( code )
    {
        case SCREEN_INIT:                 
            oled.init( BIG );
            large = true;                       // true for 128x64, false for 128x32    
            screen_count = 3;                   // number of screens. 0=off, 1=default, 2...
            screen_state = 1; 
            break;
        
        case SCREEN_WAITFOR_CLI:                // ------------------- requires delay in sec   
            oled.dsp( 0, "\vCLI: PRESS CR");
            oled.dsp( 2, "\v AP: BUTTON");
			oled.dsp( 5, "\vwithin %02d sec", arg);
            // 6th line displaying progress
            PF("For CLI press RETURN. For AP Mode press button. Do it within %d sec\r\n", arg );
            break;
                           
        case SCREEN_TIMEOUT:                   // ----------- requires 2nd argument 'count' 
            oled.dsp( 5, 7, "\v %02d", arg );    
            PF(".");
            // 6th line displaying progress
            break;
            
        case SCREEN_CLI_MODE:
		    oled.clearDisplay();
			oled.dsp( 3,0, "\bCLI MODE" );
			ap_mode = 0;
			PF("\r\nCLI MODE. Press h for help\r\n" );
			break;

        case SCREEN_STA_MODE:
		    oled.clearDisplay();
			oled.dsp( 3,0, "\bSTA MODE" );
			ap_mode = 1;
			PF("\r\nSTA MODE. User Thinker.io or Browser\r\n" );
			break;
			
		case SCREEN_AP_MODE:
		    oled.clearDisplay();
			oled.dsp( 3,0, "\bAP MODE" );
			ap_mode = 2;
			PF("\r\nSTA MODE. User Thinker.io or Browser\r\n" );
			break;
		
        case SCREEN_LOOP_VALUES:
            showLoopValues();
            break;

        case SCREEN_LOOP_READY:
            if( ap_mode == 0 )
				break;
			oled.clearDisplay();
			if( ap_mode == 1 )
				PR("Use Thinker.io or Browser");
			else
				PR("Select WiFi SSID: GKE_AP");
            break;

        case SCREEN_LOOP_STATUS:
            //oled.dsp( 0, 0, "\v%s\r", !tm.getDateString() );
			char *md;
			
            if( ap_mode == 1 )
			{
				ip = WiFi.localIP();
				md = "STA";
			}	
			else if( ap_mode == 2 )
			{
				ip = WiFi.softAPIP();
				md = "AP";
			}	
			else
			{
				ip[0]=ip[1]=ip[2]=ip[3]=0;
				md = "CLI";
			}	
            oled.dsp( 0, 0, "\a%d.%d.%d.%d", ip[0],ip[1],ip[2],ip[3] );
            oled.dsp( 1, 0, md );
            oled.dsp( 1, 3, " Port:%d", eep.wifi.port );
            
            //oled.dsp( 2, 0, "\a%s\r", !tm.getTimeString() );
            oled.dsp( 3, 0, "\aRSSI %ddBm", WiFi.RSSI() );
            if( large )
            {
                oled.dsp( 5, 0, "\aHeapNow %d\r", cpu.heapUsedNow() );
                oled.dsp( 6, 0, "\aHeapMax %d\r", cpu.heapUsedMax() );
            }
            break;
       default:
            PF("Unhandled case(%d) in __FILE__\r\n", code );
            break;
    }
}
void ThingUO::flipLoopScreen()
{
    screen_state = (++screen_state) % screen_count;  // 0, 1, or 2
    PF("Toggled to %d\r\n", screen_state );
    
    if( screen_state )
        oled.clearDisplay();    /// also turns ON
    else
        oled.displayOff();
}  
void ThingUO::showLoopScreen()           
{
    if( screen_state == 2 ) show( SCREEN_LOOP_VALUES );
    if( screen_state == 1 ) show( SCREEN_LOOP_STATUS );
}
// --------------------------- YOU MUST PROVIDE THE FOLLOWING ---------------------------
void ThingUO::showLoopValues()      
{
    static int flag;
    if( !owon.error() )
    {
        char *r = owon.getValueText() ;
        char *u = owon.getUnits( true );
        
        if( flag )
            oled.clearDisplay();
            
        if( (strlen( r ) + strlen( u ))<=8 )
        {
            oled.dsp( 1, "\b%s%s", r, u);
            oled.dsp( 3, "\b " );   // erase
        }
        else
        {
           oled.dsp( 1, "\b%s", r);
           oled.dsp( 3, "\b%s", u );
        }
        oled.dsp( 6, "\v%s %s\r", owon.getACDC(), owon.getType(true) ); 
        flag = 0;
    }
    else
    {
        PF( "Timeout\r\n" );
        if( flag == 0 )
            oled.clearDisplay();
        oled.dsp( 3, "\vTimeout(%d)", flag++ );
    }
}   
