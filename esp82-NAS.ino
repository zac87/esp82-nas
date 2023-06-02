/*
      esp82-NAS.ino

      NAS system running on ESP8266 boards, for storing small files in your
      local network. Can be extended with SD-card reader to make more storage
      available. Code for ST7735 TFTs is included in this file. Cointains
      complete setup routines to configure your board over your webbrowser.
      Currently secured with Basic HTTP Auth.

      Copyright 2022 globment.de

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is furnished
    to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


*/


#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <FS.h>         // Include the SPIFFS library

#include <base64.h>

#include "md5.h"
#include "site.h"

/*
  #include <SPI.h>
  #include <Wire.h>
  #include <Adafruit_GFX.h>    // Core graphics library
  #include <Adafruit_ST7735.h> // Hardware-specific library for ST7735


  #if defined(ARDUINO_FEATHER_ESP32) // Feather Huzzah32
  #define TFT_CS         14
  #define TFT_RST        15
  #define TFT_DC         32

  #elif defined(ESP8266)
  #define TFT_CS         15
  #define TFT_RST        0
  #define TFT_DC         2

  #else
  // For the breakout board, you can use any 2 or 3 pins.
  // These pins will also work for the 1.8" TFT shield.
  #define TFT_CS        15
  #define TFT_RST        0
  #define TFT_DC         2
  #
  #endif
*/

#define VERSION "0.0.2"

#define ROOT_DOC_DIR "/documents/"
#define FILE_STATUS "/status.txt"
#define FILE_DOCS "/documents.txt"

#define WIFI_DT "/sys/wk.key"
#define SYS_PW "/sys/ap.key"

const char* ssid = "FRITZ!Box 7560 SV";
const char* password = "Golf902906!";

const char* apssid = "esp8266nas-AP";
const char* appassword = "esp8266nas";

File fsUploadFile;              // a File object to temporarily store the received file

//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80


String getContentType(String filename); // convert the file extension to the MIME type
void initSoftAPMode();
void initWiFiMode();
bool isSoftAPConnected();
bool isUserAuthenticated();
void handleFileUpload();
bool handleSetupStart(String path);
bool handleFileRead(String path);       // send the right file to the client (if it exists)
bool handleDelete(String file_name);
bool handleDeleteAll();
bool handleSetup(bool newConfig);
void setAuthHandler();
void setupServerHandler();
void setupServerBlock();
bool isSetupComplete();
bool isSoftAPConnected();
//void printToTFT(String text, uint16_t color);
//void printlnToTFT(String text, uint16_t color);


void setAuthRedirectDelAll();
void setAuthRedirectDel();
void setAuthRedirectDocs();
void setAuthRedirectReset();

char setupSSID[64], setupPassword[128];
char md5Login[512];
bool softAPConnected;

//Standart IP 192.168.4.1

void setup() {
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  //tft.initR(INITR_BLACKTAB);
  //tft.fillScreen(ST77XX_BLACK);

  SPIFFS.begin();
  SPIFFS.info(fs_info);

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");
  //printlnToTFT("esp82-NAS started", ST77XX_GREEN);

  softAPConnected = false;
  setupServerHandler();
  setupServerBlock();

  if (!isSetupComplete()) {
    initSoftAPMode();
  } else {
    initWiFiMode();
  }

  Serial.println("reloading HTML files ...");
  //reloadIndex(); TODO program backup
  //reloadPub();
}

bool isSoftAPConnected() {
  return softAPConnected;
}


void setupServerBlock() {
  server.on(WIFI_DT, HTTP_GET,
  []() {
    server.send(403);
  });
  server.on(WIFI_DT, HTTP_POST,
  []() {
    server.send(403);
  });

  server.on(SYS_PW, HTTP_GET,
  []() {
    server.send(403);
  });
  server.on(SYS_PW, HTTP_POST,
  []() {
    server.send(403);
  });
}



void setupServerHandler() {
  server.on("/upload", HTTP_POST,                       // if the client posts to the upload page
  []() {
    if (strcmp(md5Login, "") != 0) {
      server.send(200);
    } else {
      server.send(403);
    }
  }, handleFileUpload);

  server.on("/deleteall", HTTP_POST,
  []() {
    setAuthRedirectDelAll();
  });

  server.on("/del", HTTP_GET,
  []() {
    setAuthRedirectDel();
  });

  server.on("/docs", HTTP_GET,
  []() {
    setAuthRedirectDocs();
  });

  server.on("/info", HTTP_GET,
  []() {
    setAuthRedirectInfo();
  });

  server.on("/setup", HTTP_POST,
  []() {
    if (handleSetup(false)) {
      server.sendHeader("Location", "/setup-confirm.html");     // Redirect the client to the success page
      server.send(303);
    }
  });

  server.on("/reset", HTTP_GET,
  []() {
    setAuthRedirectReset();
  });

}

void setAuthRedirectDelAll() {
  handleDeleteAll();
  if (strcmp(md5Login, "") != 0) {
    server.send(200);
  } else {
    server.send(403);
  }
}

void setAuthRedirectDel() {
  handleDelete(server.arg(0));
  if (strcmp(md5Login, "") != 0) {
    server.send(200);
  } else {
    server.send(403);
  }
}

void setAuthRedirectDocs() {
  if (strcmp(md5Login, "") != 0) {
    if (server.args() > 0) {
      for(int i = 0; i < server.args(); i++) {
           if (server.argName(i).equals("fn")) {
              if(server.arg(i).equals("false")) {
                server.send(200, "text/html", printDirToHTML(false));
              } else if(server.arg(i).equals("true")) {
                server.send(200, "text/html", printDirToHTML(true));
              }
           }
      }
    } else {
        server.send(200, "text/html", printDirToHTML(false));
    }
  } else {
    server.send(403);
  }
}

void setAuthRedirectInfo() {
  if (strcmp(md5Login, "") != 0) {
    server.send(200, "text/html", printSystemInfoToHtml());
  } else {
    server.send(403);
  }
}

void setAuthRedirectReset() {
  server.send(200);
  if (isSetupComplete()) {
    WiFi.softAPdisconnect(true);
    WiFi.disconnect();
    Serial.println("reseting device");
    delay(500);
    ESP.reset();
  }
}


void initSoftAPMode() {
  if (softAPConnected = (WiFi.softAP(apssid, appassword))) { //creates a access point
    /* printlnToTFT("WiFi AcccesPoint:", ST77XX_WHITE);
      printlnToTFT("SSID : " + String(apssid), ST77XX_WHITE);
      printlnToTFT("Pass.: " + String(appassword), ST77XX_WHITE);
    */
    server.onNotFound([]() {
      if (!handleSetupStart(server.uri()))                  // send it if it exists
        server.send(404, "text/plain", "404: Not Found");
    });
  }
}

void initWiFiMode() {
  Serial.println(setupSSID);
  Serial.println(setupPassword);
  wifiMulti.addAP(setupSSID, setupPassword);   // add Wi-Fi networks you want to connect to

  Serial.println("Connecting ...");
  // printlnToTFT("WiFi connecting...", ST77XX_WHITE);

  long timer = millis();

  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(250);
    if (millis() > timer + 80000) {
      handleSetup(true);
      break;
    }
    Serial.print('.');
  }
  Serial.println('\n');

  Serial.print("Connected to ");
  // printlnToTFT("Connected to ", ST77XX_GREEN);
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  // printlnToTFT(String(WiFi.SSID()), ST77XX_WHITE);
  Serial.print("IP address: ");
  // printlnToTFT("IP address :", ST77XX_WHITE);
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer
  // printlnToTFT(IpAddrToString(WiFi.localIP()), ST77XX_WHITE);

  if (MDNS.begin("esp82nas")) {              // Start the mDNS responder for esp8266.local
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.onNotFound([]() {                              // If the client requests any URI
    setAuthHandler();
  });
}

void loop(void) {
  server.handleClient();
}

String getContentType(String filename) {
  if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void setAuthHandler() {
  String authHeader = server.header("Authorization");
  Serial.println("Server Auth header " + authHeader);
  Serial.println("Encoded login creds " + String(md5Login));

  if (authHeader.equals("")) {
    server.sendHeader("WWW-Authenticate", "Basic realm=\"Restricted Area\"");
    server.send(401, "text/plain", "404: Unauthorized");
  }
  String base64FromHeader = authHeader.substring(authHeader.indexOf(' ') + 1);
  Serial.println("Base64 login : " + base64FromHeader);

  char *toMd5 = new char[base64FromHeader.length()];
  strcpy(toMd5, base64FromHeader.c_str());

  if (strcmp(md5Login, do_md5(toMd5).c_str()) == 0) {
    if (isSetupComplete()) {
      if (!handleFileRead(server.uri()))                  // send it if it exists
        server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
    }
  } else {
    server.sendHeader("WWW-Authenticate", "Basic realm=\"Restricted Area\"");
    server.send(401, "text/plain", "404: Unauthorized");
  }
}


void handleFileUpload() { // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String fileName = upload.filename;
    Serial.print("handleFileUpload Name: ");
    Serial.println(fileName);
    fileName = String(ROOT_DOC_DIR) + fileName;
    fsUploadFile = SPIFFS.open(fileName, "w+");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    fileName = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      //  printlnToTFT("File uploaded", ST77XX_WHITE);
      Serial.print("handleFileUpload Size: ");
      Serial.println(upload.totalSize);
      server.sendHeader("Location", "/documents.html");     // Redirect the client to the success page
      server.send(303);
      Serial.print("Sending redirect headers.");

      //reloadDoc();
      delay(100);
      //reloadIndex();
      delay(100);
      //reloadPub();
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }

}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";            // If a folder is requested, send the index file
  if (path.equals("/setup.html")) path = "index.html";
  //TODO /documents/index.html

  String contentType = getContentType(path);                // Get the MIME type
  String pathWithGz = path + ".gz";
  Serial.println("opening file " + path);
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {   // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz)) {                         // If there's a compressed version available
      path += ".gz";                                        // Use the compressed version
    }
    Serial.println("streaming file " + path);

    File file = SPIFFS.open(path, "r");                     // Open the file
    size_t sent = server.streamFile(file, contentType);     // And send it to the client
    file.close();                                           // Then close the file again
    // }
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;
}

bool handleSetupStart(String path) {
  Serial.println("handleFileRead (setup): " + path);
  if (path.endsWith("/")) path = "/setup.html";         // If a folder is requested, send the setup file

  String contentType = getContentType(path);            // Get the MIME type
  if (contentType.equals("text/html")) {
    if (!path.equals("/setup.html") && !path.equals("/setup-about.html") && !path.equals("/setup-confirm.html"))
      path = "/setup.html";
  }
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {   // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz)) {                         // If there's a compressed version available
      path += ".gz";                                        // Use the compressed version
    }

    File file = SPIFFS.open(path, "r");                     // Open the file
    size_t sent = server.streamFile(file, contentType);     // And send it to the client
    file.close();                                           // Then close the file again

    return true;
  }
  Serial.println("\tFile Not Found");
  return false;
}



bool handleDelete(String fileName) {

  if (SPIFFS.exists(fileName)) {
    if (fileName.startsWith(ROOT_DOC_DIR)) {
      //  printlnToTFT("Deleting " + fileName, ST77XX_RED);
      Serial.println("Deleting " + fileName);
      SPIFFS.remove(fileName);
      // reloadDoc();
      // reloadIndex();
      // reloadPub();
    }
  }
  server.sendHeader("Location", "/documents.html");     // Redirect the client to the success page
  server.send(303);

  return true;
}


bool handleDeleteAll() {
  Dir root = SPIFFS.openDir(ROOT_DOC_DIR);
  Serial.println("Deleting all files ...");
  // printlnToTFT("Deleting all files...", ST77XX_RED);

  while (root.next()) {
    Serial.println("Deleting " + root.fileName());
    SPIFFS.remove(root.fileName());
  }

  server.sendHeader("Location", "/documents.html");     // Redirect the client to the success page
  server.send(303);
  return true;
}

bool readSysPasswordFromFile() {
  if (strcmp(md5Login, "") == 0) {
    if (SPIFFS.exists(SYS_PW)) {                            // If the file exists
      Serial.println("Checking File ...");
      if (File file = SPIFFS.open(SYS_PW, "r")) {
        if (file != NULL) {
          Serial.println("Reading from /sys");
          String buffer = file.readStringUntil('\r');
          strcpy(md5Login, buffer.c_str());
          Serial.println(md5Login);
          file.close();
        } else {
          return false;
        }
      } else {
        Serial.println("File does not exist");
        return false;
      }
    } else {
      Serial.println("File does not exist");
      return false;
    }
  }
  return true;
}

bool handleSetup(bool newConfig) {
  if (!newConfig) {
    if (File file = SPIFFS.open(SYS_PW, "w+")) {
      if (file != NULL) {
        Serial.println("Writing to /sys");

        char userAndPw[] = "esp82nas:";
        strcat(userAndPw, server.arg(0).c_str());
        userAndPw[strlen(userAndPw)] = '\0';

        Serial.println(userAndPw);
        Serial.print("Encoded login ");
        Serial.println(base64::encode(userAndPw));

        String base64Login = base64::encode(userAndPw);
        char *toMd5 = new char[base64Login.length()];
        strcpy(toMd5, base64Login.c_str());
        file.println(do_md5(toMd5));

        file.close();
        Serial.println("Creds written");
      } else {
        return false;
      }
    } else {
      return false;
    }


    if (File file = SPIFFS.open(WIFI_DT, "w+")) {
      if (file != NULL) {
        Serial.println("Writing to /sys");
        char *trimedSSID;
        char *trimedPWD;
        trimedSSID = trimWhitespace((char *)server.arg(2).c_str());
        trimedPWD = trimWhitespace((char *)server.arg(3).c_str());
        strcpy(setupSSID, trimedSSID);
        strcpy(setupPassword, trimedPWD);
        setupSSID[strlen(setupSSID)] = '\0';
        setupPassword[strlen(setupPassword)] = '\0';

        file.println(setupSSID); //writing ssid
        file.println(setupPassword); //writing wifi pw
        file.close();

      } else {
        return false;
      }
    } else {
      return false;
    }
    delay(100);
  } else {
    if (SPIFFS.exists(SYS_PW)) {
      SPIFFS.remove(SYS_PW);
    }
    if (SPIFFS.exists(WIFI_DT)) {
      SPIFFS.remove(WIFI_DT);
    }
    delay(100);
    ESP.reset();
  }

  return true;
}


bool isSetupComplete() {
  readSysPasswordFromFile();
  SPIFFS.begin();
  if (strcmp(setupSSID, "") == 0 && strcmp(setupPassword, "") == 0) {
    if (SPIFFS.exists(WIFI_DT)) {                            // If the file exists
      Serial.println("Checking File ...");
      if (File file = SPIFFS.open(WIFI_DT, "r")) {
        if (file != NULL) {
          Serial.println("Reading from /sys");
          String buffer = file.readStringUntil('\r');
          strncpy(setupSSID, trimWhitespace((char *) buffer.c_str()), buffer.length());
          setupSSID[buffer.length()] = '\0';
          buffer = file.readStringUntil('\r');
          strncpy(setupPassword, trimWhitespace((char *) buffer.c_str()), buffer.length());
          setupPassword[buffer.length()] = '\0';
          file.close();
        } else {
          return false;
        }
      } else {
        Serial.println("File does not exist");
        return false;
      }
    } else {
      Serial.println("File does not exist");
    }
  }
  Serial.print("SSID : ");
  Serial.println(setupSSID);
  if (strcmp(setupSSID, "") != 0 && strcmp(setupPassword, "") != 0
      && SPIFFS.exists(WIFI_DT) && SPIFFS.exists(SYS_PW)) {
    Serial.println("Setup completed");
    return true;
  } else {
    Serial.println("Setup mode");
    //ESP.reset();
    return false;
  }
}

//void writeToSysFile(const char* fileName); ToDo
//server.argName()


char *trimWhitespace(char *str)
{
  char *end;

  // Trim leading space
  while (isspace((unsigned char)*str)) str++;

  if (*str == 0) // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}
/*
  void printlnToTFT(String text, uint16_t color) {
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.println(text);
  }

  void printToTFT(String text, uint16_t color) {
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
  }*/
