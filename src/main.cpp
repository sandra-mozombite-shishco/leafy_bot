#include <Arduino.h>
//#include "MQ135.h"
#include <MQUnifiedsensor.h>
#include <TFT_eSPI.h>
#include <SPI.h>

/*********************************************  PANTALLA **********************************************/ 
// Pantalla
TFT_eSPI tft = TFT_eSPI();  // Invoke library

/***************************************** CONTROL DE AGUA ********************************************/ 
//Pines para el control de la bomba e indicadores de agua
const int pinMosfetGate = 22;     //Pin digital
const int pinWater85Level = 21;   //Pin digital
const int pinWater10Level = 19;   //Pin digital

const int pinMoistSoil = 36;      //Pin analogico
int MoistThre = 40;               //Es el valor% de humedad limite para regar la maceta

//Variables de estados
int waterLevel_state = 1;
int soilMoist_state = 0;
String tankLed = "Blanco";        // Ejemplos: "Led tanque ROJO"; "Led tanque NARANJA"; "Led tanque VERDE"
String tankNotif = "";
String tankScrMessage = "";


/***************************** SENSORES DE CALIDAD DE AIRE ********************************************/ 
//Variables y pines del sensado de aire
//const int pinAir_MQ135=39;
//MQ135 mq135_sensor = MQ135(pinAir_MQ135);
String Board = "ESP-32"; 
const float Voltage_Resolution = 3.3;
const int ADC_Bit_Resolution = 12;

String Type_MQ135 = "MQ-135";
const int pinAir_MQ135 = 39; // Pin analógico GPIO39
const float RatioMQ135CleanAir = 3.6; //RS / R0 = 3.6 ppm
const float R0value_MQ135 = 0;

String Type_MQ9 = "MQ-9";
const int pinAir_MQ9 = 34; // Pin analógico GPIO34
const float RatioMQ9CleanAir = 9.6; //RS / R0 = 60 ppm
const float R0value_MQ9 = 0;

MQUnifiedsensor MQ135(Board, Voltage_Resolution, ADC_Bit_Resolution, pinAir_MQ135, Type_MQ135); //Objeto de clase MQ
MQUnifiedsensor MQ9(Board, Voltage_Resolution, ADC_Bit_Resolution, pinAir_MQ9, Type_MQ9); //Objeto de clase MQ


void setup() {
  //start serial connection
  Serial.begin(9600);

  /*********************************************  PANTALLA **********************************************/ 
  //Screen
  tft.init(INITR_BLACKTAB);
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);

 /***************************************** CONTROL DE AGUA ********************************************/ 
  //configure water pins as an input and enable the internal pull-up resistor
  pinMode(pinWater85Level, INPUT_PULLUP);   //Pin entrada de nivel de agua
  pinMode(pinWater10Level, INPUT_PULLUP);   //Pin entrada de nivel de agua
  pinMode(pinMosfetGate, OUTPUT);           //Pin salida que activa la bomba

  /***************************** SENSORES DE CALIDAD DE AIRE ********************************************/ 
  //Configure Air quality objects
  //Set math model to calculate the PPM concentration and the value of constants
  MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ135.setA(110.47); MQ135.setB(-2.862); // Configure the equation to to calculate CO2 concentration
    /*
    Exponential regression:
    GAS      | a      | b
    CO       | 605.18 | -3.937  
    Alcohol  | 77.255 | -3.18 
    CO2      | 110.47 | -2.862
    Toluen  | 44.947 | -3.445
    NH4      | 102.2  | -2.473
    Aceton  | 34.668 | -3.369
    */

  MQ9.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ9.setA(599.65); MQ9.setB(-2.244); // Configure the equation to to calculate CO concentration
    /*
    Exponential regression:
    GAS     | a      | b
    LPG     | 1000.5 | -2.186
    CH4     | 4269.6 | -2.648
    CO      | 599.65 | -2.244
    */
  MQ135.init(); 
  MQ135.setRL(1); //Valor en kilo ohms

  MQ9.init(); 
  MQ9.setRL(1); //Valor en kilo ohms ---------------------------------------------------> FALTA CORROBORAR
  
  /*Uncomment above in case of calibration*/
  /*Serial.print("Calibrating MQ135 please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcR0/10);
  Serial.println("  done!.");
  
  if(isinf(calcR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  
  Serial.print("Valor de R0 para MQ135");
  Serial.println(MQ135.getR0());
  /*****************************  MQ CAlibration ********************************************/ 
  //MQ135.setR0(R0value_MQ135); Uncomment for a fix value

  /*Uncomment above in case of calibration*/
  /*Serial.print("Calibrating MQ9 please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ9.update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0 += MQ9.calibrate(RatioMQ9CleanAir);
    Serial.print(".");
  }
  MQ9.setR0(calcR0/10);
  Serial.println("  done!.");
  
  if(isinf(calcR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  
  Serial.print("Valor de R0 para MQ9");
  Serial.println(MQ9.getR0());
  /*****************************  MQ CAlibration ********************************************/ 
  //MQ135.setR0(R0value_MQ9); Uncomment for a fix value

  MQ135.serialDebug(true);
  MQ9.serialDebug(true);
}

// the loop function runs over and over again forever
void loop() {

  //===============================================================================
  // LECTURA DE LOS SENSORES
  //===============================================================================
  
  //Lectura de los sensores de nivel de agua
  int waterLevel_85 = digitalRead(pinWater85Level); //above level = 0, below level = 1
  int waterLevel_10 = digitalRead(pinWater10Level); //above level = 0, below level = 1
  waterLevel_state = waterLevel_85 + waterLevel_10;
    /* waterLevel_state =
      2 : empty tank
      1 : half full tank
      0 : full tank
    */

  //Lectura del sensor de humedad del suelo
  int moistSoil = analogRead(pinMoistSoil); // read the analog value from sensor
  float perMoist= (1-(moistSoil/4095.0))*100;

  //Lectura de los sensores de calidad de aire
  MQ135.update();
  float CO2_ppm = MQ135.readSensor();
  MQ9.update();
  float CO_ppm = MQ9.readSensor();


  //===============================================================================
  // CONTROL DE INDICADORES DEL ESTADO DE HUMEDAD DEL SUELO
  //===============================================================================
  if (perMoist < MoistThre) // when the soil is WET
  {
    soilMoist_state = 0;
    switch (waterLevel_state) // Cambios de los indicadores
    {
    case 0:
      tankLed = "Led tanque verde"; 
      tankNotif = "No hay notif.";
      tankScrMessage = "Tanque lleno";
      break;
    case 1:
      tankLed = "Led tanque AMARILLO"; 
      tankNotif = "No hay notif.";
      tankScrMessage = "Tanque con agua";
      break;
    case 2:
      tankLed = "Led tanque NARANJA"; 
      tankNotif = "No hay notif.";
      tankScrMessage = "Tanque vacío";
      break;
    default:
      break;
    }
  }
  else // when the soil is DRY
  {
    soilMoist_state = 1;
    switch (waterLevel_state)
    {
    case 0:
      tankLed = "Led tanque verde parp."; 
      tankNotif = "Sí hay notif.";
      tankScrMessage = "Tanque listo";
      break;
    case 1:
      tankLed = "Led tanque nanranja parp."; 
      tankNotif = "Sí hay notif.";
      tankScrMessage = "Insuficiente agua";
      break;
    case 2:
      tankLed = "Led tanque rojo parp."; 
      tankNotif = "Sí hay notif.";
      tankScrMessage = "¡¡¡Tanque vacío!!!";
      break;
    default:
      break;
    }
  }
  
  Serial.println(tankLed);
  Serial.println(tankNotif);
  Serial.println(tankScrMessage);

  //===============================================================================
  // PANTALLita :)
  //===============================================================================

  tft.setCursor(4, 10);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(2);
  tft.println("PROTIPO 2");
  tft.println("");

  
  tft.setTextSize(1);
  tft.println(tankLed);
  tft.println(tankNotif);
  tft.println(tankScrMessage);


  /***************************************** CONTROL DE AGUA ********************************************/ 
  // Control segun el sensor de nivel del deposito
/*
  if (waterLevel_10 == 1) { // Nivel de agua por debajo del 10%
    Serial.println("Despósito de agua vacío");
    tft.println("Tanque agua: VACIO");
    delay(4000);
    digitalWrite(pinMosfetGate, LOW); // 0 logico al gate -> apaga bomba
    
  }

  else if (waterLevel_85 == 0 && waterLevel_10 == 0) { //nivel de agua por encima del 85%
    Serial.println("Despósito de agua lleno");
    tft.println("Tanque agua: LLENO");
    delay(7000);
    digitalWrite(pinMosfetGate, HIGH); // 1 logico al gate -> enciende bomba
  }
  else{
    Serial.println("Hay agua en el depósito");
    tft.println("Hay agua en el tanque");
    delay(2000);
  }
  tft.println("");

  // Control segun la humedad del suelo
  

  if(perMoist < MoistThre){
    Serial.println("El suelo está seco. Heche agua en el depósito");
    tft.println("Suelo SECO");
  }else {
    Serial.println("El suelo está húmedo ");
    tft.println("Suelo HUMEDO ");
  }
  Serial.print("Humedad: ");  
  Serial.print(perMoist);
  Serial.println("%");
  delay(100);
  tft.print("Humedad: ");  
  tft.print(perMoist);
  tft.println("%");

  tft.println("");

  /***************************** SENSORES DE CALIDAD DE AIRE ********************************************/

  /*
  Serial.print("CO2: ");
  Serial.print(CO2_ppm);
  Serial.println(" PPM");

  
  Serial.print("CO: ");
  Serial.print(CO_ppm);
  Serial.println(" PPM");

  //Control según el sensor de gas
  /*delay(100);
  float ppm = mq135_sensor.getPPM();

  Serial.print("Sensor de aire: ");  
  Serial.print(ppm);
  Serial.println(" PPM"); 
  
  tft.print(ppm);
  tft.println(" ppm"); 

  if (ppm > 250 && ppm < 400){
    Serial.println("Calidad del aire: Normal");
    tft.println("Air Q.: Normal");
  } else if (ppm > 400 && ppm < 1000){
    Serial.println("Calidad del aire: Típico con buen intercambio de aire");
    tft.println("Air Q.: Típico con buen intercambio de aire");
  } else if (ppm > 1000){
    Serial.println("Calidad del aire: Baja calidad");
    tft.println("Air Q.: Baja");
  }*/

  Serial.println(" ");
  Serial.println(" ");

  delay(1000);
}

/*
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <TFT_eSPI.h>
#include <SPI.h>

#define REPORTING_PERIOD_MS     1000

// Create a PulseOximeter object
PulseOximeter pox;

// Pantalla
TFT_eSPI tft = TFT_eSPI();  

// Time at which the last beat occurred
uint32_t tsLastReport = 0;

// Callback routine is executed when a pulse is detected
void onBeatDetected() {
    tft.println("Beat!");
}

void setup() {
    Serial.begin(9600);
    ////PANTALLA
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(ST7735_BLACK);
    
    Serial.print("Initializing pulse oximeter..");

    // Initialize sensor
    if (!pox.begin()) {
        tft.println("FAILED");
        for(;;);
    } else {
        tft.println("SUCCESS");
    }

  // Configure sensor to use 7.6mA for LED drive
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

    // Register a callback routine
    pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
    // Read from the sensor
    pox.update();

    // Grab the updated heart rate and SpO2 levels
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        //tft.fillScreen(ST7735_BLACK);
        tft.print("Heart rate:");
        tft.print(pox.getHeartRate());
        tft.println("bpm");
        tft.println(" ");
        tft.print("SpO2: ");
        tft.print(pox.getSpO2());
        tft.println("%");


        Serial.print("Heart rate:");
        Serial.print(pox.getHeartRate());
        Serial.println("bpm");
        Serial.println(" ");
        Serial.print("SpO2: ");
        Serial.print(pox.getSpO2());
        Serial.println("%");

        tsLastReport = millis();
        //delay(1000);
        //tft.fillRect(18,30,100,100,TFT_BLACK);
    }
  tft.setCursor(4, 10);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(2);
  tft.println("PULSIOXIMETRO");
  tft.println("");
  tft.setTextSize(1);
}*/