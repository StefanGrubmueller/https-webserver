#include <WiFi.h>
#include <SPIFFS.h>
#include <FS.h>
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <string>
#include <vector>
 
using namespace httpsserver;

std::vector<std::string> listSPIFFSwww();
std::vector<std::string> listFilesInDir(File dir);
int createCert(SSLCert cert);
void checkCertResult(int certresult);

void checkIfFileNameExists(HTTPResponse * res, String name);
void loadFiles(const std::vector<std::string> &filenames, HTTPSServer *secureServer);
void respondWithFile(HTTPResponse * res, const std::string &name);
void setContentLength(HTTPResponse * res, size_t size);
void setContentType(HTTPResponse * res, std::string reqfile);
void setContent(HTTPResponse * res, File file);

 
const char* ssid = "POLYGONS-OFFICE";
const char* password =  "FEqHTLrx$#,G";

HTTPSServer * secureServer;
SSLCert * cert;

char contentTypes[][2][32] = {
  {".html", "text/html"},
  {".css",  "text/css"},
  {".js",   "application/javascript"},
  {".json", "application/json"},
  {".png",  "image/png"},
  {".jpg",  "image/jpg"},
  {".svg",  "image/svg+xml"},
  {"", ""}
};

void setup() 
{
  Serial.begin(9600);
  if (!SPIFFS.begin()) {
        Serial.println("Failed to mount file system");
        return;
	}
  
  const std::vector<std::string> filenames = listSPIFFSwww();

  cert = new SSLCert();

  int createCertResult = createSelfSignedCert(*cert, KEYSIZE_1024, "CN=192.168.10.79,O=acme,C=US");
  //int createCertResult = createCert(&cert);
  checkCertResult(createCertResult);
   
  secureServer = new HTTPSServer(cert);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());

  ResourceNode * nodeRoot = new ResourceNode("/", "GET", [] (HTTPRequest * req, HTTPResponse * res){
    const std::string name = "/www/wificonfig.html";
    respondWithFile(res, name);
  });
  secureServer->registerNode(nodeRoot);
  
  ResourceNode * nodepost = new ResourceNode("/wificonfig", "POST", [] (HTTPRequest * req, HTTPResponse * res){
    // byte buffer[256];
    // while(!(req->requestComplete())) {
    //    size_t s = req->readBytes(buffer, 256);
    //    //Serial.println(s);
    // }
    res->println("404 Not Found");
  });
  secureServer->registerNode(nodepost);
  
  //loadFiles(filenames, secureServer);

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


auto listSPIFFSwww() -> std::vector<std::string>
{
  File file = SPIFFS.open("/www", "r");
  if(!file){
      Serial.println("Failed to open file");
  }
  const std::vector<std::string> filenames = listFilesInDir(file);
  file.close();
  return filenames;
}

auto listFilesInDir(File dir) -> std::vector<std::string>
{ 
  std::vector<std::string> filenames;
  while (true) {
    File entry = dir.openNextFile();

    if (!entry) {
      break;
    }
    if (entry.isDirectory()) {
      Serial.println("/");
      listFilesInDir(entry);
    } 
    else {
      filenames.push_back(entry.name());
      Serial.println(entry.name());
      Serial.print("\t\t\t");
      Serial.println(String(entry.size()) + " Byte");
    }
    entry.close();
  }
  return filenames;
}

int createCert(SSLCert *cert)
{  
  Serial.println((long)&cert);
  int createCertResult = createSelfSignedCert(*cert, KEYSIZE_1024, "CN=192.168.10.79,O=acme,C=US");
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

void loadFiles(const std::vector<std::string> &filenames, HTTPSServer *secureServer)
{
  for(const auto & elem : filenames) {
    ResourceNode * Files = new ResourceNode(elem, "GET",  [] (HTTPRequest * req, HTTPResponse * res) {
      Serial.println(String(req->getRequestString().c_str()));
      respondWithFile(res, req->getRequestString());
    });
    secureServer->registerNode(Files);
  }
}

void respondWithFile(HTTPResponse * res, const std::string &name)
{
  File file = SPIFFS.open(String(name.c_str()), "r");
  checkIfFileNameExists(res, file.name());
  setContentLength(res,file.size());
  setContentType(res,file.name());
  setContent(res,file);

  file.close();
}

void checkIfFileNameExists(HTTPResponse * res, String name) 
{
  if (!SPIFFS.exists(name)) {
    res->setStatusCode(404);
    res->setStatusText("Not found");
    res->println("404 Not Found");
    return;
  }
}

void setContentLength(HTTPResponse * res, size_t size) 
{
  res->setHeader("Content-Length", intToString(size));
}

void setContentType(HTTPResponse * res, std::string reqfile) 
{
  int cTypeIdx = 0;
  do {
    if(reqfile.rfind(contentTypes[cTypeIdx][0]) != std::string::npos) {
      res->setHeader("Content-Type", contentTypes[cTypeIdx][1]);
      break;
    }
    ++cTypeIdx;
  } while(strlen(contentTypes[cTypeIdx][0])>0);
}

void setContent(HTTPResponse * res, File file) 
{
  uint8_t buffer[1024];
  size_t length = 0;

  do {
    length = file.read(buffer, 1024);
    res->write(buffer, length);
  } while (length > 0);
}