#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WifiClientSecure.h>
#include <WebSocketsClient.h>
#include <ACAN2515.h>

#define cs_2515 5          // réglage broche CS
#define int_2515 36          // réglage broche Interuption 
#define f_2515 16000000     // réglage Fréquence du quartz de la carte MCP2515 
#define f_bus_can 250000      // réglage Vitesse du bus CAN
#define INTERVAL 200

CANMessage messageCANReception;
CANMessage message;

const char* ssid = "RPGL";
const char* password = "ViveMha1805";

class RegimeMoteur {
    public:
        int regime;
        CANMessage message;
        RegimeMoteur(CANMessage message) {
            message = message;
            regime = (message.data[0]*256 + message.data[1]) / 8;   
        }
};

class VitesseMoteur {
    public:
        int vitesse;
        CANMessage vite;
        VitesseMoteur(CANMessage vite) {
            vite = vite;
            vitesse = (vite.data[2]*256 + vite.data[3]) / 100;         
        }
};


ACAN2515 can2515(cs_2515, SPI, int_2515); // Déclaration de l'objet CAN utilisant le réglages des broches définis avant
const ACAN2515Mask masque = standard2515Mask(0b11111111111, 0, 0);
void message_1octet(const CANMessage & inMessage);

long previousMillis = 0;

//réglage identifiant trame voulue
const ACAN2515AcceptanceFilter filtres[] = {
   { standard2515Filter(0x3E1, 0, 0), message_1octet }, 
};

void message_1octet(const CANMessage & inMessage){}

///////////////////////////////////////////////////////////////////////////////////////////
/////////////////  Création Websocket et client wifi//////////////////////////////////////////
WiFiClient  client;
WebSocketsClient webSocket; // websocket client class instance

/////////////////////////////////////////////////////////////////////////////////////////

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
		case WStype_TEXT:
            message.id = 0x101;
            message.len = 1;
            String recvmess = String(payload, DEC);
            if(recvmess == "start"){
                message.data[0] = 1;
            }
            else if(recvmess == "stop"){
                message.data[0] = 3;
            }
            can2515.tryToSend(message);
            break;
    }
} // utile si réception de message de la part du serveur

void InitWifi() {

    WiFi.begin(ssid, password);
    
    // Wait for connection
    while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(100);
    }

    Serial.println("\nConnected to WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());

}

void setup() 
{

    Serial.begin(115200);

    InitWifi();    // fonction à créer ou remplacer par code de connexion au wifi

    webSocket.begin("10.129.26.97",8015); // Exemple avec adresse IP, port et chemin
    // WebSocket event handler
    webSocket.onEvent(webSocketEvent);
    // if connection failed retry every 5s
    webSocket.setReconnectInterval(5000);
    SPI.begin();
    delay(500);
    ACAN2515Settings can_vit(f_2515, f_bus_can);
    const uint16_t v_err = can2515.begin(can_vit, [] { can2515.isr(); },masque, filtres, 2 );   

    webSocket.onEvent(webSocketEvent);

    if (v_err == 0) { 
        Serial.println("Recepteur: Config ok");
    } 
	else 
    {
        Serial.println("Recepteur: Config HS"); 
        while (1); 
    }
	
	Serial.println("Attente de message !");
	delay(500);


}

void loop() 
{
    
    webSocket.loop();
 
  
    // Temporisation "non blocante" avant l'envoi d'un ou plusieurs message
    long currentMillis = millis();
    
    if (currentMillis - previousMillis > INTERVAL) // si temporisation > interval en ms
    {                                             // alors on publie le(s) message(s)
    // On mémorise la "date" à laquelle le ou les messages ont été publiés

        if(can2515.receive(messageCANReception)){

            can2515.receive(messageCANReception);

            RegimeMoteur MoteurInfo(messageCANReception);
            VitesseMoteur VitesseInfo(messageCANReception);

            String regime = String(MoteurInfo.regime);
            String kmh = String(VitesseInfo.vitesse);

            String message = "{ \"regime\" : \"" + regime  + "\", \"kmh\" : \"" + kmh + "\" }";

            webSocket.sendTXT(message);
            //Serial.print("message : ");
            //Serial.println(message);
        }
    
    }    
   
}  

//id = 101


//0x01 


