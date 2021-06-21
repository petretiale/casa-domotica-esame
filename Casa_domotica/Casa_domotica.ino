#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <RFID.h>
#include <SimpleDHT.h>
#include <Servo.h>

#define NOTA1 262
#define NOTA2 280
#define ALLARME_SPENTO 0
#define ALLARME_INSERITO 1
#define ALLARME_ATTIVO 2
#define INDIRIZZO_LCD 0x27
#define SOGLIA 27 - 1
#define STOP_CANCELLO 90

int pirPin = 7; // Pin sensore PIR
int pinBuzzer = 2; // Pin buzzer
int pinSda = 53;  //Pin lettore tessera
int pinReset = 5; // Pin reset lettore tessera
int pinLuceBlu = 3;  // Pin led rgb blu
int pinLuceVerde = 9; // Pin led rgb verde
int pinLuceRossa = 6; // Pin led rgb rossa
int pinDHT11 = 4; // Pin sensore di umidità e temperatura
int pinVentola = 10;  // Pin motore dc
int ledPin[] = {36,38,40,42,44,46}; // Pin led
int pulsantePin[] = {22,24,26,28,30,32,23}; // Pin pulsanti
int pinFiamma = 13; // Pin sensore di fiamma
int pinServo = 11; // Pin servo cancello

// stato iniziale sensore di fiamma = 0
int statoSensoreFiamma = 0; 
// vettore per lo stato dei led
int statoLed[] = {LOW,LOW,LOW,LOW,LOW,LOW}; 
// vettore per lo stato dei pulsanti
int statoPulsante[] = {HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH}; 
// vettore per l'ultimo stato dei pulsanti
int ultimaLetturaPulsante[] = {HIGH,HIGH,HIGH,HIGH,HIGH,HIGH, HIGH};
int stato_allarme;
int stato_lcd;
int stato_ventilazione;
int pirValue;
int stato_cancello = 0; // stato iniziale chiuso
int temp_attuale = 0; // stato iniziale temperatura
int umi_attuale = 0; // stato iniziale umidità
int attesaDebouncePulsante= 50;
int antirimbalzoTag = 1000;
int tempo_aggiornamento_lcd = 2000;
unsigned long ultimoTempoDebouncePulsante = 0;
unsigned long ultimaScritturalcd = 0;
unsigned long ultimaLetturaTag = 0;
RFID rc522(pinSda, pinReset); // lettore RFID
LiquidCrystal_I2C lcd(INDIRIZZO_LCD,16,2); // LCD
SimpleDHT11 dht11; // Sensore umidità e temperatura
Servo cancello; // servo motore cancello



/** Funzioni **/
// funzione per il suono dell'allarme
void suona_allarme(); 
// funzione per la lettura Tag
String leggi_tag(RFID& lettore); 
// funzione per il controllo dei colori del led rgb
void led_colore(int rosso, int verde, int blu);
// funzione per stampare messaggi sul display LCD
void messaggio_lcd(String messaggio); 
// funzione per convertire gli stati dell'allarme in messaggi
String converti_stato_allarme(int stato_allarme);
// funzione per l'accensione e lo spegnimento dei led
void accendiSpegniLed(int statoLed[],int ledPin[], int i);
// funzione per aprire e chiudere il cancello
void apriChiudiCancello();

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  SPI.begin();
  rc522.init();
  lcd.init();
  lcd.backlight();
  
  stato_allarme = ALLARME_SPENTO; // stato iniziale 
  stato_ventilazione = LOW;
  
  pinMode(pirPin, INPUT);
  pinMode(pinBuzzer,OUTPUT);
  pinMode(pinLuceRossa, OUTPUT);
  pinMode(pinLuceVerde, OUTPUT);
  pinMode(pinLuceBlu, OUTPUT);
  pinMode(pinVentola, OUTPUT);
  pinMode(pinFiamma, INPUT);
  // inizializzazione dei led e dei pulsanti
  for(int i = 0; i < 6; i++){ 
    pinMode(ledPin[i],OUTPUT);
    digitalWrite(ledPin[i],LOW);
    pinMode(pulsantePin[i], INPUT_PULLUP);
  }
  // inizializzazione pulsante cancello
  pinMode(pulsantePin[6], INPUT_PULLUP);
  // inizializzazione ventola
  digitalWrite(pinVentola, LOW);
    
  messaggio_lcd("caricamento...");
  delay(60000); // attesa per inizializzare PIR
  Serial.println("Pronto!\n");
}

void loop() {
  byte temperatura = 0;
  byte umidita = 0;
  byte data[40] = {0};
  // gestione dei comandi provenienti da ESP32
  if(Serial1.available() > 0) {
    int comando = Serial1.parseInt();
    Serial1.println(1); // invio 1 (ok) ad esp32
    Serial1.flush();
    
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
      case 6:
        accendiSpegniLed(statoLed, ledPin, 5);
        break;   
      case 7:   //allarme inserito
        if(stato_allarme == ALLARME_SPENTO){
          stato_allarme = ALLARME_INSERITO;
        }else{
          stato_allarme = ALLARME_SPENTO; 
        }
        break;
      case 8:
        apriChiudiCancello();
        break; 
      case 10:
        Serial1.println(temp_attuale);
        Serial1.println(umi_attuale);
        Serial1.println(stato_allarme);
        Serial1.flush();
        break;
    }
  }
  //  lettura con antirimbalzo pulsanti
  for(int i = 0; i < 7; i++){
    
    int lettura = digitalRead(pulsantePin[i]);
    if(lettura != ultimaLetturaPulsante[i]){            
      ultimoTempoDebouncePulsante = millis();        
    }
  
    if((millis() - ultimoTempoDebouncePulsante) > attesaDebouncePulsante){   
      if(lettura != statoPulsante[i] and lettura == LOW){     
        if(i < 6) { // controllo i led solo se i è tra i primi 6 (0..5)
          statoLed[i] = !statoLed[i];                            
          digitalWrite(ledPin[i], statoLed[i]);          
        } else {
          Serial.println("Cnacellooo");
          apriChiudiCancello();
        }
      }
      statoPulsante[i] = lettura;                               
    }
    ultimaLetturaPulsante[i] = lettura;     
  }
  
  // lettura di temperatura e umidità dal sensore DHT11
  if (!dht11.read(pinDHT11, &temperatura, &umidita, data)) {
    temp_attuale =(int)temperatura;
    umi_attuale =(int)umidita;
  }
   // controllo impianto di aria

  if(temp_attuale > SOGLIA and stato_ventilazione == LOW){
    analogWrite(pinVentola, 255);
    stato_ventilazione = HIGH;
  } else if(temp_attuale <= SOGLIA and stato_ventilazione == HIGH) {
    analogWrite(pinVentola, 0);
    stato_ventilazione = LOW;
  }


  // lettura Rfid con antirimbalzo 
  if (rc522.isCard()){
    if((millis()- ultimaLetturaTag) > antirimbalzoTag){
      String codiceCarta = leggi_tag(rc522);
      Serial.println(codiceCarta);
      ultimaLetturaTag = millis();
      // se il codice della carta viene riconosciuto 
      if(codiceCarta == "B29A872788"){
        // cambio stato allarme
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

  if(stato_allarme == ALLARME_SPENTO){ // se lo stato allarme è spento
    led_colore(0,0,0); // spengo il led
  }
  
  if(stato_allarme == ALLARME_INSERITO){ // se lo stato allarme è inserito
    led_colore(255,0,0); // accendo il led
    pirValue = digitalRead(pirPin); // leggo lo stato del sensore Pir
    Serial.println(pirValue); // stampo su seriale lo stato del sensore Pir
    if(pirValue == HIGH){ // se il lo stato del sensore Pir è alto
      stato_allarme = ALLARME_ATTIVO; // lo stato del allarme passa ad attivo
      
    }
  }

  if(stato_allarme == ALLARME_ATTIVO){ // se lo stato allarme è attivo
    led_colore(255,0,0); //accendo il led 
    delay(200); // aspetta 2 ms
    led_colore(0,0,0); //spengo il led 
    suona_allarme(); // richiamo la funzione suona allarme
  }
  
  statoSensoreFiamma = digitalRead(pinFiamma); // leggo lo stato del sensore di fiamma 
  if(statoSensoreFiamma == HIGH){ // se lo stato del sensore di fiamma è alto 
    stato_allarme = ALLARME_ATTIVO; // lo stato del allarme passa ad attivo
  }
  
   // antirimbalzo per il display LCD
  if((millis()- ultimaScritturalcd) > tempo_aggiornamento_lcd){
    ultimaScritturalcd = millis();
    switch(stato_lcd){
      //scrittura di temperatura e umidità sul display LCD
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
      // scrittura dello stato di allarme sul display LCD
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

void apriChiudiCancello() {
  cancello.attach(pinServo);
  if(stato_cancello == 1) {
    cancello.write(71);
    delay(2500);
    stato_cancello = 0;
  } else {
    cancello.write(66);
    delay(3500);
    stato_cancello = 1;
  }
  cancello.detach();
}
