/*
  BenderLAN
  Seriel to Mircophython
  4MB (FS:2MB OTA:1MB) DOUT

*/
#define ARDUINO_HOSTNAME  "bender" 
const char* progversion  = "BenderLan V0.6";//ota fs ntp serialIO (getpin)

#define pin_ledWLAN 2 //gpio 2 =onboard LED
#define pin_ledWLANinvert true

#include "wifisetup.h" //18:fe:34:d6:5:b7
#include "data.h" //index.htm

#define actionheader "HTTP/1.1 303 OK\r\nLocation:/index.htm\r\nCache-Control: no-cache\r\n\r\n"
//---------------------------------------------------------------------

#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <time.h>

#include <JeVe_EasyOTA.h>  // https://github.com/jeroenvermeulen/JeVe_EasyOTA/blob/master/JeVe_EasyOTA.h
#include "FS.h"   //http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html

#include "myNTP.h"
myNTP oNtp;

//---------------------------------------------------------------------

EasyOTA OTA;
ESP8266WebServer server(80);
File fsUploadFile;                      //Hält den aktuellen Upload

uint8_t MAC_array[6];
char MAC_char[18];
String macadresse="";



int counter=0;
#define cmax 10

bool isAPmode=false;
int anzahlVerbindungsversuche=0;

#define check_wlanasclient 30000      //alle 30 Sekunden*2 gucken ob noch verbunden, wenn nicht neuer Versuch
                                      //zwischen 30 und 60 Sekunden
unsigned long check_wlanasclient_previousMillis=0;
#define anzahlVersuche 10             //nach 10 Versuchen im AP-Modus bleiben
#define keinAPModus true              //true=immer wieder versuchen ins WLAN zu kommen

unsigned long tim_zeitchecker= 15*1000;//alle 15sec Timer checken
unsigned long tim_previousMillis=0;
byte last_minute;

static unsigned long update_strip_time = 0; //blink

char recChar;
const byte numChars = 255;          //max Anzahl der Zeichen die per Serial eingelesen werden
char receivedChars[numChars];       // an array to store the received data Seriel
char lastreceivedChars[numChars];   // Antwort für www
byte lastreceivedCharsLength=0;
boolean newDataVorhanden = false;
char endMarker1 = '\n';
char endMarker2 = '\r';
boolean chatfromwww=false;
static unsigned long chat_time = 0; 

String antworttimestamp="";
//------------------------------------------------------------------------

void connectWLAN(){
   setLEDWLAN(true);
  
   anzahlVerbindungsversuche++;
   OTA.setup(WIFI_SSID, WIFI_PASSWORD, ARDUINO_HOSTNAME);//connect to WLAN
   isAPmode=!(WiFi.status() == WL_CONNECTED);
   
/*   Serial.print("mode:");
   if(isAPmode)
      Serial.println("AP");
      else
      Serial.println("client");
*/
  macadresse="";
  WiFi.macAddress(MAC_array);
  for (int i = 0; i < sizeof(MAC_array); ++i) {
    if(i>0) macadresse+=":";
    macadresse+= String(MAC_array[i], HEX);
  }
  Serial.print("MAC:");
  Serial.println(macadresse);

  Serial.print("Connectedto:");
  Serial.println(WIFI_SSID);
  Serial.print("IPaddress:");
  Serial.println(WiFi.localIP()); 

  if(isAPmode){
      setLEDWLAN(true);
      delay(500);
      setLEDWLAN(false);
      delay(250);
      setLEDWLAN(true);
      delay(500);
      setLEDWLAN(false);
      delay(250);
      setLEDWLAN(true);
      delay(500);
      setLEDWLAN(false);
      delay(250);
    }
    else{
      anzahlVerbindungsversuche=0;//erfolgreich verbunden, Zaehler auf 0 setzen
      setLEDWLAN(false);
   }
}

void setLEDWLAN(bool an){
  if(pin_ledWLANinvert)an=!an;
  digitalWrite(pin_ledWLAN, an);
}
void toogleLEDWLAN(){
  digitalWrite(pin_ledWLAN, !digitalRead(pin_ledWLAN));
}
bool getLEDWLAN(){
  if(pin_ledWLANinvert){
     return digitalRead(pin_ledWLAN)==LOW;
    }
    else{
     return digitalRead(pin_ledWLAN)==HIGH;
    }
}

void setup() {
    delay(1000);

    receivedChars[0] = '\0';
    lastreceivedChars[0] = '\0';
  
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    Serial.print("version:");
    Serial.println(progversion);
   
 
  if(pin_ledWLAN!=-1){
    pinMode(pin_ledWLAN, OUTPUT);
    setLEDWLAN(true);
    }

  //SPIFFS
    SPIFFS.begin();
 
   //OTA
    OTA.onMessage([](char *message, int line) {
     Serial.print("OTA-");
     Serial.print(line);
     Serial.print(":");
     Serial.println(message);
     //toogleLEDWLAN();
    });

 
    connectWLAN();

    server.on("/", handleIndex);//daten&befehle
    server.on("/index.htm", handleIndex);
    server.on("/index.html", handleIndex);

    server.on("/action", handleAction);//daten&befehle
    server.on("/data.json", handleData);//aktueller Status+Dateiliste; "settimekorr"
  
    server.on("/upload", HTTP_POST, []() {
      server.send(200, "text/plain", "");
     }, handleFileUpload);             //Dateiupload
 
    server.onNotFound(handleNotFound);
    server.begin();
   // Serial.println("stat:HTTP server started");
    setLEDWLAN(false);
    
   //NTP start
   oNtp.begin();   
}


void loop() {
  OTA.loop();
  server.handleClient();
  oNtp.update();
  isAPmode=!(WiFi.status() == WL_CONNECTED);

  unsigned long currentMillis = millis();//milliseconds since the Arduino board began running

  if (currentMillis - update_strip_time > 500){//500ms
    counter++;
    update_strip_time = millis();
  } 
  
  if(counter>cmax){
    counter=0;
    //actins alle 500ms
  }

  static byte ndx = 0;
  static byte i = 0;
  //Serielle Eingaben von MyPy

// chatfromwww=false;
// chat_time = 0; //blink
 
 if(currentMillis - chat_time > 1000*2 && chatfromwww){  //2 sek nichts zu Server gesendet
      //Anfrage ignorieren, Serielle für interne Kommunikation freigeben
      newDataVorhanden=false;
      chatfromwww=false; 
      //Serial.println("stat:chattimeout");
 }
    
  //Kommunikation vom MyPy zum ESP
  if(newDataVorhanden==false || chatfromwww==false){ //wurde letzte Serielle Ausgabe in www übertragen, wenn eine Frage da war?
  while(Serial.available() > 0 ){//Abfragen wenn letzte Eingabe verarbeitet wurde (newDataVorhanden==false)
     recChar = Serial.read();               //char empfangen
     
     if (recChar != endMarker1
      &&
        recChar != endMarker2
     )
     {     //kein Endmakter gefunden:
            receivedChars[ndx] = recChar;   //char speichern
            ndx++;
            if (ndx >= numChars) {          //Überlauf, letztes Zeichen überschreiben
                ndx = numChars - 1;
            }
        }
        else 
        {   //Endmaker gefunden
            receivedChars[ndx] = '\0'; // terminate the input string
          
            if(ndx>0){//mehr als "0" Zeichen
             
              //auswerten, wenn keine interne Kommunikation
              if(checkSerial()==false){
                  
                  //umkopieren, für Ausgabe ins www (chat)
                  for(i=0;i<(ndx+1);i++){
                    lastreceivedChars[i]=receivedChars[i];
                  }
                  lastreceivedCharsLength=ndx+1;
                  //Zeit des Empfangs merken
                  byte h   =oNtp.getstunde();
                  byte m   =oNtp.getminute();
                  byte s  =oNtp.getsecunde();
                  antworttimestamp="";
                  if(h<10)antworttimestamp="0";
                  antworttimestamp+=String(oNtp.getstunde())+":";
                  if(m<10)antworttimestamp+="0";
                  antworttimestamp+=String(oNtp.getminute())+":";
                  if(s<10)antworttimestamp+="0";
                  antworttimestamp+=String(oNtp.getsecunde());
                  //Maker setzen, das Antwort von mypy im Puffer für www steht
                  newDataVorhanden = true;//Empfangspufer sperren bis Antwort 1x ans www gesendet wurde
                  
              }
              else{
                  newDataVorhanden=false;    //gibt Puffer für neue Daten frei
              }
            }
            ndx = 0;
         }
   }//while
  }




  if(oNtp.hatTime() && currentMillis - tim_previousMillis > tim_zeitchecker){//Timer checken
      tim_previousMillis = currentMillis;
      if(last_minute!=oNtp.getminute()){//nur 1x pro min
        //checktimer();
        last_minute=oNtp.getminute();
     }
   }

    //WLAN-ceck
  unsigned long cwl=random(check_wlanasclient, check_wlanasclient+check_wlanasclient);//x..x+15sec sonst zu viele Anfragen am AP
  if(currentMillis - check_wlanasclient_previousMillis > cwl){
      //zeit abgelaufen
      check_wlanasclient_previousMillis = currentMillis;
       if(isAPmode){//apmode
          Serial.print("anzahlVerbindungsversuche:");
          Serial.println(anzahlVerbindungsversuche);
         //neuer Verbindengsaufbauversuch
         if(anzahlVerbindungsversuche<anzahlVersuche || keinAPModus){//nur x-mal, dann im AP-Mode bleiben
               connectWLAN();
         }
      }
 }

}

//------------Data IO--------------------
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

void handleData(){// data.json
  String message = "{\r\n";
  String aktionen = "";

  //uebergabeparameter?
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "settimekorr") {
       oNtp.setTimeDiff(server.arg(i).toInt());
        aktionen +="set_timekorr ";
    }
  }
  
  message +="\"hostname\":\""+String(ARDUINO_HOSTNAME)+"\",\r\n";
  message +="\"aktionen\":\""+aktionen+"\",\r\n";
  message +="\"progversion\":\""+String(progversion)+"\",\r\n";
  message +="\"cpu_freq\":\""+String(ESP.getCpuFreqMHz())+"\",\r\n";
  message +="\"chip_id\":\""+String(ESP.getChipId())+"\",\r\n";
  message +="\"flashchiprealsize\":\""+String(ESP.getFlashChipRealSize())+"\",\r\n";
  
  message +="\"remoteIP\":\"";
  message +=server.client().remoteIP().toString().c_str();
  message +="\",\r\n";
 
  message +="\"isAPmode\":\"";
  if(isAPmode)
     message +="true";
    else
     message +="false";
  message +="\",\r\n";

  //letzte empfangene Daten von Serial zu www schicken
  if(lastreceivedCharsLength>0){
      String stemp=String(lastreceivedChars);
      message +="\"antwort\":{\"text\":\"";
      message +=stemp;
      message +="\",\"t\":\"";
      message +=antworttimestamp;
      message +="\"},\r\n";
     if(newDataVorhanden){ 
        newDataVorhanden = false;
      }//Puffer für neue Eingaben freigeben

      if(chatfromwww){
          if(stemp.indexOf("Bchat:")>-1){
             chatfromwww=false; //Antwort gesendet
          }
      }
  }

  //lokale Zeit
  byte ntp_stunde   =oNtp.getstunde();
  byte ntp_minute   =oNtp.getminute();
  byte ntp_secunde  =oNtp.getsecunde();
 
//  ntp_stunde
  message +="\"lokalzeit\":\"";
  if(ntp_stunde<10)message+="0";
  message+=String(ntp_stunde)+":";
  if(ntp_minute<10)message+="0";
  message+= String(ntp_minute)+":";
  if(ntp_secunde<10)message+="0";
  message+=String(ntp_secunde);
  message +="\",\r\n";
  
  message +="\"datum\":{\r\n";
  message +=" \"tag\":"+String(oNtp.getwochentag())+",\r\n";
  message +=" \"year\":"+String(oNtp.getyear())+",\r\n";
  message +=" \"month\":"+String(oNtp.getmonth())+",\r\n";
  message +=" \"day\":"+String(oNtp.getday())+",\r\n";
  message +=" \"timekorr\":"+String(oNtp.getUTCtimediff())+",\r\n";
  if(oNtp.isSummertime())
    message +=" \"summertime\":true\r\n";
  else
    message +=" \"summertime\":false\r\n";
  message +="},\r\n";

//led/relais-status
  
  message +="\"portstatus\":{\r\n";

  message +="\"ledWLAN\":"; 
  if(getLEDWLAN())
         message +="true";
         else
         message +="false";

  
  message +="\r\n";
  message +="},\r\n";//Portstatus
       
         

  message +="\"macadresse\":\""+macadresse+"\",\r\n";

  FSInfo fs_info;
  if (SPIFFS.info(fs_info)) {
      message +="\"fstotalBytes\":"+String(fs_info.totalBytes)+",\r\n";
      message +="\"fsusedBytes\":"+String(fs_info.usedBytes)+",\r\n";

      message +="\"fsused\":\"";
      message +=float(int(100.0/fs_info.totalBytes*fs_info.usedBytes*100.0)/100.0);
      message +="%\",\r\n";
   }
  //files
  message +="\"files\":[\r\n";
  String fileName;
  Dir dir = SPIFFS.openDir("/");
  uint8_t filecounter=0;
  while (dir.next()) {
      fileName = dir.fileName(); 
      if(filecounter>0)  message +=",\r\n";
      message +=" {";
      message +="\"fileName\":\""+fileName+"\", ";
      message +="\"fileSize\":"+String(dir.fileSize());
      message +="}";
      filecounter++;
  };
  message +="\r\n]\r\n";
//--
 
  message +="\r\n}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");//wenn vom HTTPS-Seiten aufgerufen wird!
  server.send(200, "text/plain", message );
  //Serial.println("send:data.json");
}


void handleIndex() {//Rueckgabe HTML
  //$h1gtag $info
  int pos1 = 0;
  int pos2 = 0;
  String s;
  String tmp;

  String message = "";

  while (indexHTM.indexOf("\r\n", pos2) > 0) {
    pos1 = pos2;
    pos2 = indexHTM.indexOf("\r\n", pos2) + 2;
    s = indexHTM.substring(pos1, pos2);

    //Tags gegen Daten ersetzen
    if (s.indexOf("$h1gtag") != -1) {
      s.replace("$h1gtag", progversion);//Ueberscherschrift=Prog-Version
    }

    //Liste der Dateien
    if(s.indexOf("$filelist") != -1){        
        tmp="<table class=\"files\">\n";
        String fileName;
        Dir dir = SPIFFS.openDir("/");
        while (dir.next()) {
            fileName = dir.fileName(); 
            Serial.print("getfilelist:");
            Serial.println(fileName);
            tmp+="<tr>\n";
            tmp+="\t<td><a target=\"_blank\" href =\"" + fileName + "\"" ;
            tmp+= " >" + fileName.substring(1) + "</a></td>\n\t<td class=\"size\">" + formatBytes(dir.fileSize())+"</td>\n\t<td class=\"action\">";
            tmp+="<a href =\"" + fileName + "?delete=" + fileName + "\" class=\"fl_del\"> l&ouml;schen </a>\n";
            tmp+="\t</td>\n</tr>\n";
        };

        FSInfo fs_info;
        tmp += "<tr><td colspan=\"3\">";
        if (SPIFFS.info(fs_info)) {
          tmp += formatBytes(fs_info.usedBytes).c_str(); //502
          tmp += " von ";
          tmp += formatBytes(fs_info.totalBytes).c_str(); //2949250 (2.8MB)   formatBytes(fileSize).c_str()
          tmp += " (";
          tmp += float(int(100.0/fs_info.totalBytes*fs_info.usedBytes*100.0)/100.0);
          tmp += "%)";
          /*tmp += "<br>\nblockSize:";
          tmp += fs_info.blockSize; //8192
          tmp += "<br>\npageSize:";
          tmp += fs_info.pageSize; //256
          tmp += "<br>\nmaxOpenFiles:";
          tmp += fs_info.maxOpenFiles; //5
          tmp += "<br>\nmaxPathLength:";
          tmp += fs_info.maxPathLength; //32*/
        }
        tmp+="</td></tr></table>\n";
        s.replace("$filelist", tmp);
    }

    
    message += s;
  }

  server.send(200, "text/html", message );
}


void handleAction() {//Rueckgabe JSON
  /*
      /action?bender=LEDOFF   LED ausschalten
      /action?benderser=huhu
      /action?getpin=0        aktuellen Status von Pin IO0
      http://192.168.0.45/action?bender=augeLR:0.5  -> Serial-> "act:augeLR:0.5"

  */
  String message = "{\n";
  message += "\"Arguments\":[\n";

  uint8_t AktionBefehl = 0;
  uint8_t keyOK = 0;
  uint8_t aktionresult = 0;
  float volt=0.0;
  float watt=0.0;

  for (uint8_t i = 0; i < server.args(); i++) {
    if (i > 0) message += ",\n";
    message += "  {\"" + server.argName(i) + "\" : \"" + server.arg(i) + "\"";

    if (server.argName(i) == "bender") {
       if(server.arg(i) == "LEDWLANON")  AktionBefehl = 1;
       if(server.arg(i) == "LEDWLANOFF") AktionBefehl = 2;

       Serial.print("act:");
       Serial.println(server.arg(i));
    }
    if (server.argName(i) == "benderser"){//senden an MyPhy von www
       chatfromwww=true;  //auf Antwort wartend
       chat_time= millis();//Zeitpunkt Chat empfangen
        
       Serial.print("chat(");
       Serial.print(server.client().remoteIP());
       Serial.print("):");
       Serial.println(server.arg(i));

      //antwort direkt holen?
       
       message += ",\n\"ser\":\"OK\"";
    }
   /*   
    if (server.argName(i) == "getpin"){
       message += " ,\"val\": \"";
       if(digitalRead( server.arg(i).toInt())==HIGH)
              message += "true";
              else
              message += "false";
       message += "\"";
    }
*/
    message += "}";
  }
  
  message += "\n]";

  if(AktionBefehl>0){
      aktionresult = handleAktion(AktionBefehl, 1);
      message += ",\n\"befehl\":\"";
      if (aktionresult > 0)
        message += "OK";
      else
        message += "ERR";
      message += "\"";
  }
  
  message += "\n}";
  server.send(200, "text/plain", message );

}


String getContentType(String filename) {              // ContentType fuer den Browser
  if (filename.endsWith(".htm")) return "text/html";
  //else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

void handleFileUpload() {          // Dateien ins SPIFFS schreiben
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    Serial.print("UploadDataName:");
    if (filename.length() > 30) {
      int x = filename.length() - 30;
      filename = filename.substring(x, 30 + x);
    }
    Serial.println(filename);
    filename = server.urlDecode(filename);
    filename = "/" + filename;
    
    fsUploadFile = SPIFFS.open(filename, "w");
    if(!fsUploadFile) Serial.println("UploadData:failed");
    
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile){
        Serial.print("UploadDataWrite:"); Serial.println(upload.currentSize);
        fsUploadFile.write(upload.buf, upload.currentSize);
      }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile){
      fsUploadFile.close();
      Serial.println("UploadData:close");
    }
    yield();
    //Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
    server.sendContent(actionheader);//Seite neu laden
  }
}

bool handleFileRead(String path) {//Datei loeschen oder uebertragen
  if(server.hasArg("delete")) {
    SPIFFS.remove(server.arg("delete"));  //hier wir geloescht
    server.sendContent(actionheader);//Seite neu laden
    Serial.print("deletefile:");
    Serial.print(server.arg("delete"));
    return true;
  }
  path = server.urlDecode(path);
  //Serial.print("GET ");
  //Serial.print(path);
  if (SPIFFS.exists(path)) {
     File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, getContentType(path));
    file.close();
    return true;
  }
  //Serial.println(" 404");
  return false;
}

void handleNotFound() {
 //--check Dateien im SPIFFS--
 if(!handleFileRead(server.uri())){ 
    //--404 als JSON--
    String message = "{\n \"error\":\"File Not Found\", \n\n";
    message += " \"URI\": \"";
    message += server.uri();
    message += "\",\n \"Method\":\"";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\",\n";
    message += " \"Arguments\":[\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      if (i > 0) message += ",\n";
      message += "  {\"" + server.argName(i) + "\" : \"" + server.arg(i) + "\"}";
    }
    message += "\n ]\n}";  
    server.send(404, "text/plain", message);  
  }
}

//----------------------IO-----------------------------
uint8_t handleAktion(uint8_t befehl, uint8_t key) {
 if(key != 1)return 0;  
    
  if (befehl == 1) {//LEDWLANON
    setLEDWLAN(true);
    return befehl;
  }
  if (befehl == 2) {//LEDWLANOFF
    setLEDWLAN(false);
    return befehl;
  }
 
  return 0;
}

//------------Serial IO--------------------

//get:time get:date get:day get:wochentag get:mac get:ip get:ssid get:netmode
//get:ledwlan 
//get:chipid get:flashsize get:chipspeed get:cyclecount

bool checkSerial(){
  //Daten von MyPy
  String s=receivedChars;


 /*if(s.indexOf("Bchat:")==-1){
    return false;
  }*/

  
  if(s.indexOf("get:")==-1)return false;
 
  if(s.indexOf("get:time")==0){
    Serial.print("ntp_time:");
    byte h  =oNtp.getstunde();
    byte m  =oNtp.getminute();
    byte s  =oNtp.getsecunde();
    Serial.print(h);
    Serial.print(":");
    Serial.print(m);
    Serial.print(":");
    Serial.println(s);
    return true;
  }
  
  if(s.indexOf("get:date")==0){
    Serial.print("ntp_date:");
    Serial.print(oNtp.getday());
    Serial.print(".");
    Serial.print(oNtp.getmonth());
    Serial.print(".");
    Serial.println(oNtp.getyear());
    return true;
  }
  
  if(s.indexOf("get:day")==0){
   Serial.print("ntp_day:");
   Serial.println(oNtp.getday());//1..31
   return true;
  }

  if(s.indexOf("get:wochentag")==0){
   Serial.print("ntp_wochentag:");
   Serial.println(oNtp.getwochentag());//5=Sa
   return true;
  }

  if(s.indexOf("get:mac")==0){
   Serial.print("esp_mac:");
   Serial.println(macadresse);
   return true;
  }
  
  if(s.indexOf("get:ip")==0){
   Serial.print("esp_ip:"); 
   Serial.println(WiFi.localIP());
   return true;
  }

  if(s.indexOf("get:ssid")==0){
   Serial.print("esp_ssid:"); 
   Serial.println(WIFI_SSID);
   return true;
  }

  if(s.indexOf("get:netmode")==0){
   Serial.print("esp_net:"); 
   if(isAPmode)
     Serial.println("AP");
     else
     Serial.println("client");
   return true;
  }

  if(s.indexOf("get:ledwlan")==0){
   Serial.print("esp_ledwlan:"); 
   if(getLEDWLAN())
     Serial.println("1");
     else
     Serial.println("0");
   return true;
  }

  if(s.indexOf("get:chipid")==0){
   Serial.print("esp_chipid:"); 
   Serial.println(ESP.getChipId());
   return true;
  }

  if(s.indexOf("get:flashsize")==0){
   Serial.print("esp_flashsize:"); 
   Serial.println(ESP.getFlashChipRealSize());//byte
   return true;
  }

  if(s.indexOf("get:chipspeed")==0){
   Serial.print("esp_chipspeed:"); 
   Serial.println(ESP.getFlashChipSpeed());//Hz
   return true;
  }

  if(s.indexOf("get:cyclecount")==0){
   Serial.print("esp_cyclecount:"); 
   Serial.println(ESP.getCycleCount());
   return true;
  }

  return false;
}


/*
 Serial.println("");

    Serial.print("FreeHeap:");
    Serial.println(ESP.getFreeHeap());
    
    //ESP.getFlashChipId() returns the flash chip ID as a 32-bit integer.
    Serial.print("FlashChipID:0x");
    Serial.println(ESP.getFlashChipId(),HEX);
     
    //ESP.getFlashChipSize() returns the flash chip size, in bytes, as seen by the SDK (may be less than actual size).
    //Serial.print("Flash Chip Size: ");
    //Serial.println(ESP.getFlashChipSize());//wie eingestellt...
    
    Serial.print("FlashChiprealSize:");
    Serial.println(ESP.getFlashChipRealSize());//!!
    
    //ESP.getFlashChipSpeed(void) returns the flash chip frequency, in Hz.
    Serial.print("FlashChipSpeed:");
    Serial.println(ESP.getFlashChipSpeed());
    
    //ESP.getCycleCount() returns the cpu instruction cycle count since start as an unsigned 32-bit. This is useful for accurate timing of very short actions like bit banging.
    Serial.print("CycleCount:");
    Serial.println(ESP.getCycleCount());
    
*/
