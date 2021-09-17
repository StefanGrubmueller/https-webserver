#include <WiFi.h>
#include "SPIFFS.h"
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
 
using namespace httpsserver;

void listSPIFFS();
void listFilesInDir(File dir);
int createCert(SSLCert *cert);
void checkCertResult(int certresult);
 
const char* ssid = "POLYGONS-OFFICE";
const char* password =  "FEqHTLrx$#,G";

HTTPSServer * secureServer;
SSLCert * cert;

void setup() 
{
  Serial.begin(9600);
  if (!SPIFFS.begin()) {
        Serial.println("Failed to mount file system");
        return;
	}
  
  listSPIFFS();

  cert = new SSLCert();

  int createCertResult = createCert(cert);
  checkCertResult(createCertResult);
 
  Serial.println("Certificate created with success");
   
  secureServer = new HTTPSServer(cert);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());
 
  ResourceNode * nodeRoot = new ResourceNode("/", "GET", [](HTTPRequest * req, HTTPResponse * res){
    res->println("Secure Hello World!!!");
  });
 
  secureServer->registerNode(nodeRoot);
  secureServer->start();
  if (secureServer->isRunning()) {
    Serial.println("Server ready.");
  }
}
 
void loop() 
{
  secureServer->loop();
  delay(10);
}

void listSPIFFS() 
{
  File file = SPIFFS.open("/", "r");
  if(!file){
      Serial.println("Failed to open file");
  }
  listFilesInDir(file);
  file.close();
}

void listFilesInDir(File dir) {
  while (true) {
    File entry = dir.openNextFile();

    if (!entry) {
      break;
    }

    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      listFilesInDir(entry);
    } 
    else {
      Serial.print("\t\t\t");
      Serial.println(String(entry.size()) + " Byte");
    }
    entry.close();
  }
}

auto createCert(SSLCert *cert) -> int
{
  Serial.println("Creating certificate...");
  
  int createCertResult = createSelfSignedCert(*cert, KEYSIZE_1024, "CN=esp32.local,O=acme,C=US");
  return createCertResult;
}


void checkCertResult(int certresult)
{
  if (certresult != 0) {
    Serial.printf("Error generating certificate");
    return; 
  }
  else {
    Serial.println("Certificate created with success");
  }
} 