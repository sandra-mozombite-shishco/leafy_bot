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
const int pinWater85Level = 21;   //Pin digital
const int pinWater10Level = 19;   //Pin digital

const int pinMoistSoil = 36;      //Pin analogico
int MoistThre = 40;               //Es el valor% de humedad limite para regar la maceta

const int pinMosfetGate = 22;     //Pin digital

//Variables de estados
int waterLevel_state = 1;
int soilMoist_state = 0;
String tankLed = "Blanco";        // Ejemplos: "Led tanque ROJO"; "Led tanque NARANJA"; "Led tanque VERDE"
String tankNotif = "";
String tankScrMessage = "";

const int pinPushBut_pump=17;     //Pin digital en pull-down
volatile int pump_Buttontag = 0;  // Señal de inicio del tanque, cambiara cuando se presione el botón de llenado
                          //  pump_Buttontag = 0; no se manda la señal de activación del tanque
                          //  pump_Buttontag = 1; se manda la señal de activación del tanque*/
void IRAM_ATTR turnONpump();

int pump_Watertag = 0; // Señal de inicio del tanque, cambiara cuando se cumpla "case 0" con el suelo seco
                          //  pump_Watertag = 0; no se manda la señal de activación del tanque
                          //  pump_Watertag = 1; se manda la señal de activación del tanque*/
int pump_cycle = 0;
unsigned long betweenCycle_time = 0; // min*1000*60
static unsigned long initialTime=0;


/***************************** SENSORES DE CALIDAD DE AIRE ********************************************/ 
//Variables y pines del sensado de aire
//const int pinAir_MQ135=39;
//MQ135 mq135_sensor = MQ135(pinAir_MQ135);
String Board = "ESP-32";
const float Voltage_Resolution = 3.3;
const int ADC_Bit_Resolution = 12;

String Type_MQ135 = "MQ-135";
const int pinAir_MQ135 = 39; // Pin analógico GPIO39
const float RatioMQ135CleanAir = 3.6; //RS / R0 = 3.6
const float R0value_MQ135 = 0;

String Type_MQ9 = "MQ-9";
const int pinAir_MQ9 = 34; // Pin analógico GPIO34
const float RatioMQ9CleanAir = 9.6; //RS / R0 = 9.6
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
  
  //External setting interrupt
  pinMode(pinPushBut_pump, INPUT_PULLUP);  // Configurar el GPIO del pulsador en modo pull-down
  attachInterrupt(digitalPinToInterrupt(pinPushBut_pump), turnONpump, RISING);  // Configurar la interrupción en el flanco ascendente

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
  
  /*Uncomment below in case of calibration*/
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

  /*Uncomment below in case of calibration*/
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
    // waterLevel_state =
    //  2 : empty tank
    //  1 : half full tank
    //  0 : full tank

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
  if (perMoist > MoistThre) // when the soil is WET
  {
    soilMoist_state = 0;
    pump_Watertag = 0;
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
      pump_Watertag = 1;
      break;
    case 1:
      tankLed = "Led tanque nanranja parp."; 
      tankNotif = "Sí hay notif.";
      tankScrMessage = "Insuficiente agua";
      pump_Watertag = 0;
      break;
    case 2:
      tankLed = "Led tanque rojo parp."; 
      tankNotif = "Sí hay notif.";
      tankScrMessage = "¡¡¡Tanque vacío!!!";
      pump_Watertag = 0;
      break;
    default:
      break;
    }
  }

  
  //===============================================================================
  // CONTROL DE ENCENDIDO DE LA BOMBA DE AGUA
  //===============================================================================
  int pump_start =  pump_Buttontag +  pump_Watertag; 
  // 2 = 1 + 1: EMPIEZA. Se presiono el boton y el tanque esta lleno
  // 1 = 1 + 0: Se presionó el botón sin que el tanque esté lleno
  // 1 = 0 + 1: Tanque lleno sin presionar el botón
  // 0 = 0 + 0: Tanque vacio sin presionar boton
  //betweenCycle_time = 0; // min*1000*60
  unsigned long time = millis()- initialTime; 
  // mover de lugar para no operar todo el tiempo. Luego quitar del serial mas externo al loop

  pump_cycle = (pump_start==2) ? 1 : pump_cycle; 
  // pump_cycle = 1 when pump_start=2, else pump_start keep its value

  switch (pump_cycle)
  {
    case 1:                                             //CICLO 1 --------------
      if (waterLevel_state==0)                          //Tanque lleno
        {digitalWrite(pinMosfetGate, HIGH);}            //Enciende la bomba
      else if (waterLevel_state==2)                     //Tanque vacio
      {
        digitalWrite(pinMosfetGate, LOW);               //Apaga bomba
        initialTime = millis();                         //Reinicia la cuenta
        pump_cycle=2;
        betweenCycle_time = 15*1000; // min*1000*60
      }
      break;
    case 2:                                             //CICLO 2 --------------
      if (time>=betweenCycle_time)                      //Si se superan los 15 min
      {
        if (waterLevel_state<2)                         //Tanque lleno o con agua
          {digitalWrite(pinMosfetGate, HIGH);}          //Enciende la bomba
        else if (waterLevel_state==2)                   //Tanque vacio
        {  
          digitalWrite(pinMosfetGate, LOW);             //Apaga la bomba
          initialTime = millis();                       //Reinicia la cuenta
          pump_cycle=3;
          betweenCycle_time = 20*1000; // min*1000*60
        }
      }
      break;
    case 3:                                             //CICLO 3 --------------
      if (time>=betweenCycle_time)                      //Si se superan los 20 min
      {
        if (waterLevel_state<2)                         //Tanque lleno o con agua
          {digitalWrite(pinMosfetGate, HIGH);}          //Enciende la bomba
        else if (waterLevel_state==2)                   //Tanque vacio
        {  
          digitalWrite(pinMosfetGate, LOW);             //Apaga la bomba
          initialTime = millis();                       //Reinicia la cuenta
          pump_cycle=4;
          betweenCycle_time = 30*1000; // min*1000*60
        }
      }
      break;
    case 4:                                             //CICLO 4 --------------
      if (time>=betweenCycle_time)                      //Si se superan los 30 min
      {
        if (waterLevel_state<2)                         //Tanque lleno o con agua
          {digitalWrite(pinMosfetGate, HIGH);}          //Enciende la bomba
        else if (waterLevel_state==2)                   //Tanque vacio
        {  
          digitalWrite(pinMosfetGate, LOW);             //Apaga la bomba
          initialTime = 0;                              //Reinicia la cuenta
          pump_cycle=0;
          betweenCycle_time = 0; // min*1000*60
          time = 0;
          pump_Buttontag = 0;                           //Se resetea el estado del pulsador
        }
      }
      break;
    default:
      break;
  }
  

  //===============================================================================
  // CONTROL DE CALIDAD DE AIRE
  //===============================================================================



  //===============================================================================
  // PANTALLita :)
  //===============================================================================

  tft.setCursor(4, 10);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(2);
  tft.println("PROTIPO 2");


  
  tft.setTextSize(1);
  tft.println("");
  /*tft.println(tankLed);
  tft.println(tankNotif);
  tft.println(tankScrMessage);*/

  Serial.println(tankLed);
  Serial.println(tankNotif);
  Serial.println(tankScrMessage);

  if (soilMoist_state==0)
  {
    tft.print("SUELO WET");
    Serial.print("SUELO WET");
  }
  else
  {
    tft.print("SUELO DRY");
    Serial.print("SUELO DRY");
  }
  Serial.print("  %:");
  Serial.println(perMoist);
  tft.print(" %:");
  tft.println(perMoist);

  Serial.print("H2OLvl: ");
  Serial.println(waterLevel_state);
  Serial.print("PumpStart: ");
  Serial.println(pump_start);
  Serial.print("+H2Otag: ");
  Serial.print(pump_Watertag);
  Serial.print("  +PushBt: ");
  Serial.println(pump_Buttontag);

  tft.print("H2OLvl: ");
  tft.println(waterLevel_state);
  tft.print("PumpStart: ");
  tft.println(pump_start);
  tft.print("+H2Otag: ");
  tft.print(pump_Watertag);
  tft.print(" +PushBt: ");
  tft.println(pump_Buttontag);

  Serial.print("Cycle: ");
  Serial.println(pump_cycle);
  tft.print("Cycle: ");
  tft.println(pump_cycle);

  Serial.print("initialTime: ");
  Serial.println(initialTime);
  tft.print("initialTime: ");
  tft.println(initialTime);

  Serial.print("HalfTime: ");
  Serial.println(betweenCycle_time);
  tft.print("HalfTime: ");
  tft.println(betweenCycle_time);

  Serial.print("Time: ");
  Serial.println(time);
  tft.print("Time: ");
  tft.println(time);
  tft.println(" ");

  Serial.print("pinMOSFET: ");
  Serial.println(pinMosfetGate);
  tft.print("pinMOSFET: ");
  tft.println(pinMosfetGate);





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


void IRAM_ATTR turnONpump() {
  if (pump_Watertag == 1)  pump_Buttontag = 1;  
  // Cambiar el estado de la variable a 1 cuando se active la interrupción del botón
}