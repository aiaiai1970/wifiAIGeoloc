#include <WiFiS3.h>
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
ArduinoLEDMatrix matrix;

#include <NanoEdgeAI.h>
#include <knowledge.h>
float input_user_buffer[DATA_INPUT_USER * AXIS_NUMBER]; // Buffer of input values
float output_class_buffer[CLASS_NUMBER]; // Buffer of class probabilities
uint16_t id_class = 0;
const char *id2class[CLASS_NUMBER + 1] = { // Buffer for mapping class id to class name
	"unknown",
	"   Room A   ",
	"   Room B   "
};

/*
mode = 1 : list visible networks MAC
mode = 2 : datalogger
mode = 3 : run AI
*/
int mode = 3;

// list of wifi network to use (generated using mode 1)
String ssidArray[] {
"A0:A0:A0:A0:A0:A0",
"B0:B0:B0:B0:B0:B0",
"C0:C0:C0:C0:C0:C0",
"D0:D0:D0:D0:D0:D0",
"E0:E0:E0:E0:E0:E0",
"F0:F0:F0:F0:F0:F0"
};
int nbOfNetworks = 6;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  matrix.begin();

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don"t continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

	enum neai_state error_code = neai_classification_init(knowledge);
	if (error_code != NEAI_OK) {
		/* This happens if the knowledge does not correspond to the library or if the library works into a not supported board. */
    Serial.println("Failed to initialize NEAI lib!");
    Serial.println(error_code);
	}
}

void loop() {
  delay(500);
  if(mode == 1) visibleNetworks();
  if(mode == 2) datalogNetworks();
  if(mode == 3) aiFunction();
}

void visibleNetworks() {
  // scan for nearby networks:
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1)
  {
    Serial.println("Couldn't get a WiFi connection");
    while (true);
  }

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    byte bssid[6];
    String idWifi = stringMacAddress(WiFi.BSSID(thisNet, bssid));
    Serial.print("\"");
    Serial.print(idWifi);
    Serial.println("\",");
  }  
}

void datalogNetworks() {
  float outputBuffer[nbOfNetworks];
  for(int i = 0; i<nbOfNetworks; i++){
    outputBuffer[i] = -100;
  }
  // scan for nearby networks:
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1)
  {
    Serial.println("Couldn't get a WiFi connection");
    while (true);
  }

  // get the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    
    byte bssid[6];
    String idWifi = stringMacAddress(WiFi.BSSID(thisNet, bssid));
    // find index in the network table
    int indexId = findId(idWifi, ssidArray);
    // if index is found populate the value in the buffer array
    if(indexId<nbOfNetworks){
      outputBuffer[indexId] = WiFi.RSSI(thisNet);
    }
  }

  // Serial print the buffer content with space separator
  for(int i = 0; i<nbOfNetworks-1; i++){
    Serial.print(outputBuffer[i]);
    Serial.print(" ");
  }
  Serial.print(outputBuffer[nbOfNetworks-1]);
  Serial.println();
}

void aiFunction(){
  float outputBuffer[nbOfNetworks];
  for(int i = 0; i<nbOfNetworks; i++){
    outputBuffer[i] = -100;
  }
  // scan for nearby networks:
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1)
  {
    Serial.println("Couldn't get a WiFi connection");
    while (true);
  }

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    
    byte bssid[6];
    String idWifi = stringMacAddress(WiFi.BSSID(thisNet, bssid));
    // find index in the network table
    int indexId = findId(idWifi, ssidArray);
    // if index is found populate the value in the buffer array
    if(indexId<nbOfNetworks){
      outputBuffer[indexId] = WiFi.RSSI(thisNet);
    }
  }

  // use local AI model to un inference and get the corresponding place
  neai_classification(outputBuffer, output_class_buffer, &id_class);
  Serial.println(id_class);

  // display place name on LED matrix
  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(100);
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(id2class[id_class]);
  matrix.endText(SCROLL_LEFT);
  matrix.endDraw();

}


String stringMacAddress(byte mac[]) {
  String smac = "";
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      smac += "0";
    }
    String mac1 = String(mac[i],HEX);
    mac1.toUpperCase();
    smac += mac1;
    if (i > 0) {
      smac += ":";
    }
  }
  return(smac);
}

int findId(String idWifi, String ssidArray[]) {
  int returnId = nbOfNetworks;

  for(int i = 0; i<nbOfNetworks; i++){
    if(ssidArray[i] == idWifi){
      returnId = i;
    }
  }
  return(returnId);
}
