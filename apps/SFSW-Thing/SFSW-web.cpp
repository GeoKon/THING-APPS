#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>

#include  <cpuClass.h>
#include  <eepClass.h>
#include  <eepTable.h>
#include  <cliClass.h>

#include <webSupport.h>
//#include <edtCallbacks.h>

    ESP8266WebServer server(80);

// -------------------- forward references -------------------------
    void apCallbacks();
    String fileList();
// ----------------------- setupAP() and loopAP() ------------------
void setupAP()
{
    cpu.led( ON );
    bool ok = WiFi.softAP("GKE_AP");
    if( !ok )
        cpu.die( "Cannot start AP" );
    
    PN( "ACCESS POINT MODE. IP Address is: "); PR( WiFi.softAPIP());
    
    gtrace = 0xfff; // T_FILEIO | T_REQUEST | T_ACTIONS;
    
    // print file names
    PN( fileList() );
        
    apCallbacks();
    server.begin( 80 );
    PR("HTTP server started");
}
void loopAP()
{
    static int connected = -1;
    
    int numbconx = WiFi.softAPgetStationNum();
    if( connected != numbconx )
        PF("%d connections\r\n", connected = numbconx );
    server.handleClient();
}

extern struct MyEEP         // my parameters
    {
        char label[16];
        char devID[16];
        int  altcntr;     // 0=no RF, 1=Use basic v1, 2=use basic v2
        int  flags;       // bit0=IFTTT
        
    } myeep;

// --------------------- support functions -----------------------------
void apCallbacks()
{
    server.on("/restart", HTTP_GET, 
    [](){
      String resp("RESTART! (control is lost)");
      server.send(200, "text/plan", resp);
      delay( 1000 );
      ESP.restart();
    }); 
    server.on("/reset", HTTP_GET, 
    [](){
      String resp("RESET! (control is lost)");
      server.send(200, "text/plan", resp);
      delay( 1000 );
      ESP.reset();
    }); 
    
    
    server.on("/status", HTTP_GET, 
    [](){
      String json("",100);
      json.set( "{" );
      json.add( "'ssid': '%s',", eep.wifi.ssid );
      json.add( " 'pwd': '%s'", eep.wifi.pwd );
      json.add( "}" );
      showJson( json );  
      server.send(200, "text/json", json);
    }); 
    server.on("/parms", HTTP_GET, 
    [](){
      String json("",100);
      json.set( "{" );
      json.add( "'devid': '%s',", myeep.devID );
      json.add( "'userid': '%s',", myeep.label );
      json.add( "'altcntr': '%d'", myeep.altcntr );
      json.add( "}" );
      showJson( json );  
      server.send(200, "text/json", json);
    }); 
    server.on("/setap.htm", HTTP_GET, 
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
    
    server.on("/setparms.htm", HTTP_GET, 
    [](){
        String path = server.uri();
        if( server.args() > 2 )
        {
            String Sdevid   = server.arg(0);
            String Suserid  = server.arg(1);
            String Saltcntr = server.arg(2);
            PF("Entered %s, %s, %s\r\n", !Sdevid, !Suserid, !Saltcntr );
            strncpy( myeep.devID, !Sdevid, 16 );
            strncpy( myeep.label, !Suserid, 16 );
            myeep.altcntr = atoi( !Saltcntr );
        }
        if( !handleFileRead( server.uri()) )
            server.send(404, "text/plain", "File /setparms.htm");   
    }); 
    
    server.on("/dir", HTTP_GET,
    [](){
        showArgs();
        server.send(200, "text/plain", fileList() );     
    });
    
    server.on("/edit", HTTP_GET,  
    [](){
        showArgs();
        if(server.args() == 0) 
            return server.send(500, "text/plain", "BAD ARGS");
        String func = server.argName(0);  
        String path = "/"+server.arg(0);

        if( func=="del" )
        {
            PR("handleFileDelete: " + path);
            if(!SPIFFS.exists( path ) )
                return server.send(404, "text/plain", "FileNotFound");
            SPIFFS.remove(path);
        }
        else if( func=="new" )
        {
            PR("handleFileCreate: " + path);
            if(SPIFFS.exists(path))
                return server.send(500, "text/plain", "FILE EXISTS");
            
            File file = SPIFFS.open(path, "w");
            if(file)
                file.close();
            else
                return server.send(500, "text/plain", "CREATE FAILED");
        }
        server.send(200, "text/plain", fileList() );     
    });
    
    bool handleFileRead1(String path);
    server.on("/upload", HTTP_GET, 
    []() {                                                      // if the client requests the upload page
        showArgs();
        if (!handleFileRead1("/upload.htm"))                     // send it if it exists
            server.send(404, "text/plain", "404: Not Found");   // otherwise, respond with a 404 (Not Found) error
    });

    void handleFileUpload1();                                    // forward reference, see below
    server.on("/upload", HTTP_POST,                       // if the client posts to the upload page
    [](){ 
        PR("POST");
        server.send(200); 
        },                          // Send status 200 (OK) to tell the client we are ready to receive
        handleFileUpload1                                    // Receive and save the file
    );


    server.on("/",
    [](){
        server.send(200, "text/html",
        "<h1 align=\"center\">AP Services<br/><a href=\"index.htm\">Click to navigate</a></h1>"
        );
    });
    server.on("/favicon.ico", HTTP_GET, 
    [](){
        PR( "request: "+ server.uri() );
        server.send(200, "image/x-icon", "");
    });
    server.on("/currentsetting.htm", HTTP_GET, 
    [](){
        IPAddress ip = WiFi.localIP();
        char r[200];
        sprintf( r, "Model=GeorgeESP\r\nName=%d.%d.%d.%d\r\n", ip[0],ip[1],ip[2],ip[3] );
        server.send( 200, "text/html", r );
        PF("Responded to /currentsetting %s\r\n", r );
    });
    server.onNotFound( 
    [](){
        String path = server.uri() ;
        PR( "request: "+ path );
        if(SPIFFS.exists(path))
        {
            File file = SPIFFS.open(path, "r");
            server.streamFile(file, "text/html");
            file.close();
        }
        else
            server.send(404, "text/plain", "Serves only .htm files");   
    });
}
// -------------------------------- handlers -----------------------------------------
String fileList()
{
    Dir dir = SPIFFS.openDir("/");
    String output("",500);
    while (dir.next()) 
    {    
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        output.add( "%s (%s)\r\n", fileName.c_str(), formatBytes(fileSize).c_str() );
    }  
    return output;
}
// see: https://tttapa.github.io/ESP8266/Chap12%20-%20Uploading%20to%20Server.html

String getContentType1(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".txt")) return "text/plain";
  else if (filename.endsWith(".pdf")) return "image/x-icon";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead1(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType1(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

File fsUploadFile;
void handleFileUpload1()
{                                                       // upload a new file to the SPIFFS
    HTTPUpload& upload = server.upload();
    
    if(upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if(!filename.startsWith("/")) 
            filename = "/"+filename;
        
        Serial.print("handleFileUpload Name: "); Serial.println(filename);
        fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
        filename = String();
    } 
    else if(upload.status == UPLOAD_FILE_WRITE)
    {
        if(fsUploadFile)
        {
            fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
            PF("Current Size = %d\r\n", upload.currentSize );
        }
    } 
    else if(upload.status == UPLOAD_FILE_END)
    {
        if(fsUploadFile) 
        {                                    // If the file was successfully created
          fsUploadFile.close();                               // Close the file again
          Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
          server.sendHeader("Location","/success.html");      // Redirect the client to the success page
          server.send(303);
        } 
        else 
        {
          server.send(500, "text/plain", "500: couldn't create file");
        }
    }
}
