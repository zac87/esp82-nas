/*
      site.c

    Loads HTML content dynamically and adds data to it.

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

#include <FS.h>
#include <ESP8266WiFi.h>
#include <String.h>

#include "site.h"

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

  isIndex = false;

  char hh[1024];

  snprintf(hh, 1024, header_html.c_str(), title.c_str(), css_path.c_str(), favicon.c_str(), favicon.c_str(), js_path.c_str());

  String content = String(hh) + body_pub_html;
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

  char hh[1024];
  char bh[1024];
  char sh[512];

  snprintf(hh, 1024, header_html.c_str(), title.c_str(), css_path.c_str(), favicon.c_str(), favicon.c_str(), js_path.c_str());
  snprintf(bh, 1024, body_html.c_str(), section_title.c_str(), article_id.c_str());
  snprintf(sh, 512, sidebar_html.c_str(), sidebar_title.c_str());

  String content = hh + String(bh);
  content += "<p>SSID : " + String(WiFi.SSID()) + " </p> ";
  content += "<p>Local IP : " + IpAddrToString(WiFi.localIP()) + " </p> ";
  content += "<p>Channel : " + String(WiFi.channel()) + " </p> ";
  content += "<p>Total Space : " + String(fs_info.totalBytes / 1024) + "KiB </p> ";
  content += "<p>Used Space : " + String(fs_info.usedBytes / 1024) + "KiB </p> </article> ";
  content += sh + printDirToHTML(false) + footer_html;

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

    if (f.isFile() && !fileName.equals("/documents/index.html") != 0 && fileName.startsWith(DOCUMENT_DIR)) {
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
