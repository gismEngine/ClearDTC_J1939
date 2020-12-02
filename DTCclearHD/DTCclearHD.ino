// for J1939 erease DTC

#include <mcp_can.h>
#include <SPI.h>

#define DEBUG Serial

// Default SPI pins on ESP32
// MOSI:  D23
// MISO:  D19
// SCK:   D18

#define CAN_INT 15
#define CAN_CS 4

#define LED_OK 2         // 100 ohm R  // On-board LED (GPIO 2)

const char* PROGMEM FIRMWARE_NAME = "DTC Clear HD";
const char* PROGMEM FIRMWARE_VER = "0.1";

MCP_CAN CAN(CAN_CS);

const uint32_t CLAIM_ADDRESS_TIME = 180000;          // in ms (3 min)
const uint32_t CLEAR_DM11_TIME = 1000;               // in ms (1 s)
const uint32_t CLEAR_DM3_TIME = 1000;                // in ms (1 s)

uint32_t claimAddressTime = 0;
uint32_t clearD3Time = 0;
uint32_t clearD11Time = 0;

bool addressClaimed = false;
bool d11Cleared = false;
bool d3Cleared = false;

bool led_status = false;

void setup() {
  initDebug();

  // Init CAN (MCP2515) interfaces:
  Serial.println(F("CAN-bus Setup:"));
  
  // Initialize MCP2515 running at 8MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ) == CAN_OK){
    Serial.println(F("MCP2515 Initialized Successfully!"));
  }else{
    Serial.println(F("Error Initializing MCP2515..."));
  }
  CAN.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
  
  pinMode(CAN_INT, INPUT);  

  // LED OK
  pinMode(LED_OK, OUTPUT);
  digitalWrite(LED_OK, LOW);
}

void loop() {

  if (millis() > claimAddressTime){
    if(claimAddress()){
      addressClaimed = true;
    }
    claimAddressTime = millis() + CLAIM_ADDRESS_TIME;
  }

  if (addressClaimed){
    
    if (millis() > clearD11Time){
      if(deleteDM11()){             // Active DTC
        
        clearD11Time = millis() + (2 * CLEAR_DM11_TIME);
        clearD3Time = millis() + CLEAR_DM3_TIME;
        d11Cleared = true;
      }
    }

    if (d11Cleared){
      if (millis() > clearD3Time){
        if(deleteDM3()){             // Passive DTC
          clearD3Time = millis() + CLEAR_DM3_TIME;
          d3Cleared = true;
        }
      }
    }

  }

  if(d3Cleared && d11Cleared){
    led_status = !led_status;
    digitalWrite(LED_OK, led_status);
    
    d3Cleared = false;
    d11Cleared = false;
  }

}


byte requestClaimAddressFrame[3] = {0x00, 0xEE, 0x00};
byte claimAddressFrame[8] = {0x01, 0x00, 0xE0, 0x59, 0x00, 0x81, 0x00, 0x00};

bool claimAddress(void){
  
  byte sndStat = CAN.sendMsgBuf(0x18EAFFF9, 1, 3, requestClaimAddressFrame);
  if(sndStat == CAN_OK){
    delay(100);
    byte sndStat2 = CAN.sendMsgBuf(0x18EEFFF9, 1, 8, claimAddressFrame);
  }


  if ((sndStat == CAN_OK) && (sndStat == CAN_OK)){
    Serial.println("Address claim: OK");
    return true;
  }
  Serial.println("Address claim: FAIL");
  return false;
  
}


byte requestClearDM11Frame[3] = {0xD3, 0xFE, 0x00};

// ACTIVE DTC
bool deleteDM11(void){

  byte sndStat = CAN.sendMsgBuf(0x18EAFFF9, 1, 3, requestClearDM11Frame);
  if(sndStat == CAN_OK){
    Serial.println("Clear DM11: OK");
    return true;
  }

  Serial.println("Clear DM11: FAIL");
  return false;
  
}


byte requestClearDM3Frame[3] = {0xCC, 0xFE, 0x00};

// PASIVE DTC
bool deleteDM3(void){

  byte sndStat = CAN.sendMsgBuf(0x18EAFFF9, 1, 3, requestClearDM3Frame);
  if(sndStat == CAN_OK){
    Serial.println("Clear DM3: OK");
    return true;
  }

  Serial.println("Clear DM3: FAIL");
  return false;
  
}


void initDebug(void){
  DEBUG.begin(115200);
  
  DEBUG.print(F("\n\n\n"));                         //  Print firmware name and version const string
  DEBUG.print(FIRMWARE_NAME);
  DEBUG.print(F(" - "));
  DEBUG.println(FIRMWARE_VER);

  DEBUG.print(F("CPU@ "));      
  DEBUG.print(getCpuFrequencyMhz());    // In MHz
  DEBUG.println(F(" MHz"));

  DEBUG.print(F("Xtal@ "));      
  DEBUG.print(getXtalFrequencyMhz());    // In MHz
  DEBUG.println(F(" MHz"));

  DEBUG.print(F("APB@ "));      
  DEBUG.print(getApbFrequency());        // In Hz
  DEBUG.println(F(" Hz"));

  EspClass e;
  
  DEBUG.print(F("SDK v: "));
  DEBUG.println(e.getSdkVersion());

  DEBUG.print(F("Flash size: "));
  DEBUG.println(e.getFlashChipSize());
 
  DEBUG.print(F("Flash Speed: "));
  DEBUG.println(e.getFlashChipSpeed());
 
  DEBUG.print(F("Chip Mode: "));
  DEBUG.println(e.getFlashChipMode());

  DEBUG.print(F("Sketch size: "));
  DEBUG.println(e.getSketchSize());

  DEBUG.print(F("Sketch MD5: "));
  DEBUG.println(e.getSketchMD5());
}
