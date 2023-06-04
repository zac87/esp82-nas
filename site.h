/*
      site.c

    Header file for site.c
    Loading and generating dynamic HTML code.

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
#ifndef ESP82NAS_SITE_HEADER_H
#define ESP82NAS_SITE_HEADER_H


#include <FS.h>
#include <ESP8266WiFi.h>
#include <String.h>


#define FILE_INDEX "/index.html"
#define FILE_DOCUMENTS "/documents.html"
#define FILE_PUB "/documents/index.html"

#define DOCUMENT_DIR  "/documents/"

String sidebar_title = "";
String section_title = "";
String article_id = "";
String title = "";
String favicon = "";
String css_path = "";
String js_path = "";
String doc_get_path = "";

String header_html = "<DOCTYPE! html>\n " \
                     " <html lang=\"en\">\n " \
                     " <head>      \n" \
                     "<meta charset=\"utf-8\"> \n" \
                     " <title>esp82-NAS -  %s</title> \n" \
                     "<link rel=\"stylesheet\" type=\"text/css\" href=\"%s\" />\n " \
                     "<link rel=\"icon\" href=\"%s\" type=\"image/x-icon\" />\n" \
                     "<link rel=\"shortcut icon\" href=\"%s\" type=\"image/x-icon\" />\n";

String header_script_html = "<script src=\"%s\"></script>\n" \
                            "<script language=\"javascript\">\n" \
                            "httpGetDocs(\"%s\", function(result) {\n" \
                            "document.getElementById(\"dashboard-side\").innerHTML = \"Documents <br>\" + result;\n" \
                            "})  </script>\n";

String body_html = "</head><body> <header> \n" \
                   " <img src=\"img/logo.png\"> \n" \
                   " </header>\n" \
                   "  <nav> \n" \
                   "   <ul> \n" \
                   " <li><a href=\"index.html\">Dashboard</a></li> \n" \
                   " <li><a href=\"documents.html\">Documents</a></li> \n" \
                   " <li><a href=\"\">Settings</a></li>  \n" \
                   " <li><a href=\"about.html\">About</a></li> \n" \
                   " </ul> \n" \
                   "</nav> \n" \
                   "<section>%s\n<article id=\"%s\">\n";

String body_pub_html = "<body> <header> \n" \
                       " <img src=\"../img/logo.png\"> \n" \
                       " </header>\n" \
                       "<section> \n" \
                       "Public <article id=\"public-list\">\n";

String sidebar_html = "</section>\n" \
                      "<aside id=\"dashboard-side\">" \
                      "%s (<a href=\"/documents/\">public</a>)\n<br><br>\n";

String footer_pub_html = "</article></section>\n" \
                         " <footer>\n" \
                         "Copyright © <script>document.write(new Date().getFullYear()) </script> globment.de.  All rights reserved. &nbsp; \n" \
                         "</footer > \n" \
                         "</body> \n" \
                         "</html > ";

String footer_html = "</aside>\n" \
                     " <footer>\n" \
                     "Copyright © <script>document.write(new Date().getFullYear()) </script> globment.de.  All rights reserved. &nbsp; \n" \
                     "</footer > \n" \
                     "</body> \n" \
                     "</html > ";


String generateIndexHTML();
String generatePubHTML();
String IpAddrToString(const IPAddress & ipAddress);
String printDirToHTML(bool options);
bool setupSysFile(String fileName, String content);
void setupFiles();
void reloadIndex();
void reloadPub();
void startSiteInit();


FSInfo fs_info;



void setupFiles() {
  String content = generateIndexHTML();
  setupSysFile(FILE_INDEX, content);

  content = generatePubHTML();
  setupSysFile(FILE_PUB, content);
}

bool setupSysFile(String fileName, String content) {
  if (File file = SPIFFS.open(fileName, "w+")) {
    if (file != NULL) {
      Serial.println("Writing to " + fileName);
      file.println(content);
      file.close();
    } else {
      return false;
    }
  } else {
    return false;
  }

  return true;
}

void reloadIndex() {
  String content = generateIndexHTML();
  setupSysFile(FILE_INDEX, content);
}

void reloadPub() {
  Serial.println("starting to generate PUB file");
  String content = generatePubHTML();
  setupSysFile(FILE_PUB, content);
}
/*
  String generateDocHTML() {
  sidebar_title = "esp82-NAS";
  section_title = "Documents";
  article_id = "documents-list";
  title = "Documents";
  favicon = "img/favicon.ico";
  css_path = "style.css";
  js_path = "js/nn.js";

  Serial.println("Generating Doc HTML");

  char hh[1024];
  char bh[1024];
  char sh[1024];
  char ds[1024];

  snprintf(hh, 1024, header_html.c_str(), title.c_str(), css_path.c_str(), favicon.c_str(), favicon.c_str(), js_path.c_str());
  snprintf(bh, 1024, body_html.c_str(), section_title.c_str(), article_id.c_str());
  snprintf(sh, 1024, sidebar_html.c_str(), sidebar_title.c_str());
  snprintf(ds, 1024, doc_script_html.c_str(), article_id.c_str());


  char buffer[1024 * 5 + 512];
  memset(&buffer[0], 0, sizeof(buffer));
  strcat(buffer, hh);
  strcat(buffer, ds);
  strcat(buffer, bh);
  strcat(buffer, doc_form_html.c_str());
  strcat(buffer, sh);
  strcat(buffer, footer_html.c_str());
  buffer[strlen(buffer)] = '\0';

  String content = String(hh) + String(ds) + String(bh);
  content += printDirToHTML(true) + doc_form_html;
  content += sh + footer_html;

  return content;
  }
*/

String generatePubHTML() {
  title = "public";
  favicon = "img/favicon.ico";
  css_path = "../style.css";
  js_path = "../js/nn.js";
  doc_get_path = "../docs?";

  char hh[1024];
  char hs[512];

  snprintf(hh, 1024, header_html.c_str(), title.c_str(), css_path.c_str(), favicon.c_str(), favicon.c_str());
  snprintf(hs, 512, header_script_html.c_str(), js_path.c_str(), doc_get_path.c_str());

  String content = String(hh) + String(hs) + body_pub_html;
  content += printDirToHTML(false) + footer_pub_html;

  return content;
}


String generateIndexHTML() {
  sidebar_title = "Documents";
  section_title = "esp82-NAS Status";
  article_id = "dashboard-info";
  title = "Dashboard";
  favicon = "img/favicon.ico";
  css_path = "style.css";
  js_path = "js/nn.js";
  doc_get_path = "docs?";

  char hh[1024];
  char bh[1024];
  char sh[512];
  char hs[512];

  snprintf(hh, 1024, header_html.c_str(), title.c_str(), css_path.c_str(), favicon.c_str(), favicon.c_str());
  snprintf(hs, 512, header_script_html.c_str(), js_path.c_str(), doc_get_path.c_str());
  snprintf(bh, 1024, body_html.c_str(), section_title.c_str(), article_id.c_str());
  snprintf(sh, 512, sidebar_html.c_str(), sidebar_title.c_str());


  String content = hh + String(hs) + String(bh);
  content += "<p>SSID : " + String(WiFi.SSID()) + " </p> ";
  content += "<p>Local IP : " + IpAddrToString(WiFi.localIP()) + " </p> ";
  content += "<p>Channel : " + String(WiFi.channel()) + " </p> ";
  content += "<p>Total Space : " + String(fs_info.totalBytes / 1024) + "KiB </p> ";
  content += "<p>Used Space : " + String(fs_info.usedBytes / 1024) + "KiB </p> </article> ";
  content += sh + printDirToHTML(false) + footer_html;

  return content;
}

String printSystemInfoToHtml() {
  String content = "";

  content += "<p>SSID : " + String(WiFi.SSID()) + " </p> \n";
  content += "<p>Local IP : " + IpAddrToString(WiFi.localIP()) + " </p> \n";
  content += "<p>Channel : " + String(WiFi.channel()) + " </p> \n";
  content += "<p>Total Space : " + String(fs_info.totalBytes / 1024) + "KiB </p> \n";
  content += "<p>Used Space : " + String(fs_info.usedBytes / 1024) + "KiB </p>\n";

  return content;
}


String IpAddrToString(const IPAddress & ipAddress)
{
  return String(ipAddress[0]) + String(".") + \
         String(ipAddress[1]) + String(".") + \
         String(ipAddress[2]) + String(".") + \
         String(ipAddress[3])  ;
}

String printDirToHTML(bool options) {
  File f;
  String dirHtml = "";
  String path = DOCUMENT_DIR;
  Serial.println("Opening documents ...");
  if (!SPIFFS.begin()) {
    Serial.println("Could not mount fs");
  }
  Serial.println("Listing files ..." + path);
  Serial.println("Opening dir " + path);;

  Dir root = SPIFFS.openDir(path);
  //if (SPIFFS.exists(path)) {
  //root.next()
  Serial.println("File path : " + root.fileName());
  while (root.next()) {
    f = root.openFile("r");
    //path = f.name();
    String fileName = String(f.name());
    Serial.println("indexing " + fileName);

    if (f.isFile() && !fileName.equals(" / documents / index.html") != 0 && fileName.startsWith(DOCUMENT_DIR)) {
      if (!options) {
        dirHtml += "<a href = \"" + fileName + "\">" + fileName + "</a> [" + f.size() / 1024 + " Kb]<br>\n";
      } else {
        dirHtml += "<a href = \"" + fileName + "\">" + fileName + "</a> [" + f.size() / 1024 + " Kb]&nbsp;&nbsp;<a href=\"/del?file=" + fileName + "\">delete</a><br>\n";
      }
      Serial.print(fileName);
      Serial.println(" [" + String(f.size()) + " Bytes]");
    }
  }


  Serial.println("printing content of /documents done.");
  return dirHtml;

}

void startSiteInit() {

  setupFiles();
}
#endif
