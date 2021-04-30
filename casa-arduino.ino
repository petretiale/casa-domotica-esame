#include <SPI.h>
#include <RFID.h>

#define NOTA1 262
#define NOTA2 280
#define ALLARME_SPENTO 0
#define ALLARME_INSERITO 1
#define ALLARME_ATTIVO 2

int pirPin = 7; // Input for HC-S501
int pinBuzzer = 2;
int pinSda = 10;  
int pinReset = 9;
int pinLuceBlu = 3;
int pinLuceVerde = 5;
int pinLuceRossa = 6;

int stato_allarme;
int pirValue;
RFID rc522(pinSda, pinReset);
unsigned long ultimaLetturaTag = 0;
int antirimbalzoTag = 1000;

/** Funzioni **/
void suona_allarme();
String leggi_tag(RFID& lettore);
void led_colore(int rosso, int verde, int blu);

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rc522.init();
  stato_allarme = ALLARME_SPENTO;
 
  pinMode(pirPin, INPUT);
  pinMode(pinBuzzer,OUTPUT);
  pinMode(pinLuceRossa, OUTPUT);
  pinMode(pinLuceVerde, OUTPUT);
  pinMode(pinLuceBlu, OUTPUT);
  
  
  delay(60000);
  Serial.println("Pronto!\n");
}

void loop() {
  if (rc522.isCard()){
    if((millis()- ultimaLetturaTag) > antirimbalzoTag){
      String codiceCarta = leggi_tag(rc522);
      Serial.println(codiceCarta);
      ultimaLetturaTag = millis();
      if(codiceCarta == "B29A872788"){
        Serial.println("riconosciuto");
        switch(stato_allarme){
          case ALLARME_SPENTO:
            stato_allarme = ALLARME_INSERITO;
            break;
          case ALLARME_INSERITO:
          case ALLARME_ATTIVO:
            stato_allarme = ALLARME_SPENTO;
            break;
        }
      }
    }
  }

  if(stato_allarme == ALLARME_SPENTO){
    led_colore(0,0,0);
  }
  
  if(stato_allarme == ALLARME_INSERITO){
    led_colore(255,0,0);
    pirValue = digitalRead(pirPin);
    Serial.println(pirValue);
    if(pirValue == HIGH){
      stato_allarme = ALLARME_ATTIVO;
      
    }
  }

  if(stato_allarme == ALLARME_ATTIVO){
    led_colore(255,0,0);
    delay(200);
    led_colore(0,0,0);
    suona_allarme();
  }
}


void suona_allarme(){
  tone(pinBuzzer,NOTA1,200);
  delay(200);
  tone(pinBuzzer,NOTA2,200);
  delay(200);
}

String leggi_tag(RFID& lettore) {
    lettore.readCardSerial();
    String codiceLetto ="";
     
    // Viene caricato il codice della tessera, all'interno di una Stringa
    for(int i = 0; i <= 4; i++)
    {
      codiceLetto+= String (lettore.serNum[i],HEX);
      codiceLetto.toUpperCase();
    }
    return codiceLetto;
}
void led_colore(int rosso, int verde, int blu){
 analogWrite(pinLuceRossa, rosso);
 analogWrite(pinLuceVerde, verde);
 analogWrite(pinLuceBlu, blu); 
}
