#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <RFID.h>
#include <SimpleDHT.h>

#define NOTA1 262
#define NOTA2 280
#define ALLARME_SPENTO 0
#define ALLARME_INSERITO 1
#define ALLARME_ATTIVO 2
#define INDIRIZZO_LCD 0x27

int pirPin = 7; // Input for HC-S501
int pinBuzzer = 2;
int pinSda = 53;  
int pinReset = 5;
int pinLuceBlu = 3;
int pinLuceVerde = 9;
int pinLuceRossa = 6;
int pinDHT11 = 4;
int pinVentola = 10;
int ledPin[] = {36,38,40,42,44};
int pulsantePin[] = {22,24,26,28,30};
int pinFiamma = 13;

int statoSensoreFiamma = 0;
int statoLed[] = {LOW,LOW,LOW,LOW,LOW};
int statoPulsante[] = {HIGH,HIGH,HIGH,HIGH,HIGH};
int ultimaLetturaPulsante[] = {HIGH,HIGH,HIGH,HIGH,HIGH};
int attesaDebouncePulsante= 50;
unsigned long ultimoTempoDebouncePulsante = 0;
int stato_allarme;
int stato_lcd;
int pirValue;
RFID rc522(pinSda, pinReset);
unsigned long ultimaLetturaTag = 0;
int antirimbalzoTag = 1000;
LiquidCrystal_I2C lcd(INDIRIZZO_LCD,16,2);
SimpleDHT11 dht11;
int tempo_aggiornamento_lcd = 2000;
unsigned long ultimaScritturalcd = 0;
int temp_attuale = 0;
int umi_attuale = 0;



/** Funzioni **/
void suona_allarme();
String leggi_tag(RFID& lettore);
void led_colore(int rosso, int verde, int blu);
void messaggio_lcd(String messaggio);
String converti_stato_allarme(int stato_allarme);
void accendiSpegniLed(int statoLed[],int ledPin[], int i);

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  SPI.begin();
  rc522.init();
  lcd.init();
  lcd.backlight();
  
  stato_allarme = ALLARME_SPENTO;
 
  pinMode(pirPin, INPUT);
  pinMode(pinBuzzer,OUTPUT);
  pinMode(pinLuceRossa, OUTPUT);
  pinMode(pinLuceVerde, OUTPUT);
  pinMode(pinLuceBlu, OUTPUT);
  pinMode(pinFiamma, INPUT);
  for(int i = 0; i < 5; i++){
    pinMode(ledPin[i],OUTPUT);
    digitalWrite(ledPin[i],LOW);
  }
  for(int j = 0; j < 5; j++){
    pinMode(pulsantePin[j], INPUT_PULLUP);
  }
    
  messaggio_lcd("caricamento...");
  // delay(60000);
  Serial.println("Pronto!\n");
}

void loop() {
  byte temperatura = 0;
  byte umidita = 0;
  byte data[40] = {0};
   
  if(Serial1.available() > 0) {
    int comando = Serial1.parseInt();
    Serial1.println(0);
    
    switch(comando) {
      case 1:
       accendiSpegniLed( statoLed ,ledPin , 0);
        break;
      case 2:
        accendiSpegniLed( statoLed ,ledPin , 1);
        break;
      case 3:
        accendiSpegniLed( statoLed ,ledPin , 2);
        break;
      case 4:
        accendiSpegniLed( statoLed ,ledPin , 3);
        break;
      case 5:
        accendiSpegniLed( statoLed ,ledPin , 4);
        break;   
      case 6:   //allarme inserito
        if(stato_allarme == ALLARME_SPENTO){
          stato_allarme = ALLARME_INSERITO;
        }else{
          stato_allarme = ALLARME_SPENTO; 
        }
        break; 
      case 10:
        Serial1.println(temp_attuale);
        Serial1.println(umi_attuale);
        Serial1.println(stato_allarme);
        Serial.flush();
        break;
    }
  }
  
  for(int i = 0; i < 5; i++){
    
    int lettura = digitalRead(pulsantePin[i]);
    if(lettura != ultimaLetturaPulsante[i]){            
      ultimoTempoDebouncePulsante = millis();        
    }
  
    if((millis() - ultimoTempoDebouncePulsante) > attesaDebouncePulsante){   
      if(lettura != statoPulsante[i] and lettura == LOW){     
        statoLed[i] = !statoLed[i];                            
        digitalWrite(ledPin[i], statoLed[i]);                   
      }
      statoPulsante[i] = lettura;                               
    }
  
    ultimaLetturaPulsante[i] = lettura;      
  }
  
  if (!dht11.read(pinDHT11, &temperatura, &umidita, data)) {
    temp_attuale =(int)temperatura;
    umi_attuale =(int)umidita;
  }
  
  if(temp_attuale > 20){
   analogWrite(pinVentola, 255);
  }else{
    analogWrite(pinVentola, 0);
  }

  if((millis()- ultimaScritturalcd) > tempo_aggiornamento_lcd){
    ultimaScritturalcd = millis();
    switch(stato_lcd){
      case 0:{
        String lcd_temp = (String)"temperatura  " + temp_attuale + "C";
        String lcd_umi = (String)"umidita  "+ umi_attuale + "%";
        Serial.println(lcd_temp);
        Serial.println(lcd_umi);
        lcd.clear();
        lcd.setCursor(0,0);
        messaggio_lcd (lcd_temp);
        lcd.setCursor(0,1); 
        messaggio_lcd (lcd_umi);
        stato_lcd = 1;
        break;
      }
      case 1:
        
        String lcd_allarme = converti_stato_allarme( stato_allarme);
        Serial.println(lcd_allarme);
        lcd.clear();
        lcd.setCursor(0,0);
        messaggio_lcd (lcd_allarme);
        stato_lcd = 0;
        break;
    }
  }
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
    statoSensoreFiamma = digitalRead(pinFiamma);
    if(statoSensoreFiamma == HIGH){
      stato_allarme = ALLARME_ATTIVO;
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

void messaggio_lcd(String messaggio){
  int lunghezza;
  lunghezza = messaggio.length();
  for(int i = 0; i < lunghezza ; i++){
    lcd.print(messaggio[i]);
  }
}

String converti_stato_allarme(int stato_allarme){
  switch(stato_allarme){
    case 0:
    return "allarme spento"; 
    break;
    case 1:
    return "allarme attivo";
    break;
    case 2:
    return "allarme in corso";
    break;
  }
}
void accendiSpegniLed(int statoLed[], int ledPin[], int i){
  if(statoLed[i] == HIGH){
    digitalWrite(ledPin[i], LOW);
    statoLed[i]= LOW;
  }else{
    digitalWrite(ledPin[i], HIGH);
    statoLed[i] = HIGH;
  }
}
