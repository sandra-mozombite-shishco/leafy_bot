#include <Arduino.h>
#include <MQUnifiedsensor.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

/***************************************** CONTROL DE AGUA ********************************************/ 
//Pines para el control de la bomba e indicadores de agua
const int pinWater85Level = 21;   //Pin digital
const int pinWater10Level = 19;   //Pin digital

const int pinMoistSoil = 36;      //Pin analogico
int MoistThre = 40;               //Es el valor% de humedad limite para regar la maceta

const int pinMosfetGate = 22;     //Pin digital

//Variables de estados
int waterLevel_state = 1;
volatile int soilMoist_state = 0;

String tankLed = "Blanco";        // Ejemplos: "Led tanque ROJO"; "Led tanque NARANJA"; "Led tanque VERDE"
String tankNotif = "";
String tankScrMessage = "";

const int pinPushBut_pump=15;     //Pin digital en pull-down
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
String Board = "ESP-32";
const float Voltage_Resolution = 3.3;
const int ADC_Bit_Resolution = 12;

String Type_MQ135 = "MQ-135";
const int pinAir_MQ135 = 39; // Pin analógico GPIO39
const float RatioMQ135CleanAir = 3.6; //RS / R0 = 3.6
const float R0value_MQ135 = 7.62;

String Type_MQ9 = "MQ-9";
const int pinAir_MQ9 = 34; // Pin analógico GPIO34
const float RatioMQ9CleanAir = 9.6; //RS / R0 = 9.6
const float R0value_MQ9 = 4.1;

MQUnifiedsensor MQ135(Board, Voltage_Resolution, ADC_Bit_Resolution, pinAir_MQ135, Type_MQ135); //Objeto de clase MQ
MQUnifiedsensor MQ9(Board, Voltage_Resolution, ADC_Bit_Resolution, pinAir_MQ9, Type_MQ9); //Objeto de clase MQ

/*********************************************  PANTALLA **********************************************/ 
// Pantalla
TFT_eSPI tft = TFT_eSPI();  // Invoke library

/*********************************************  LEDS **************************************************/ 
const int pinLed = 32;
const int numPixels = 10;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numPixels, pinLed, NEO_GRB + NEO_KHZ800);
//Estados del tanque:
/*uint32_t naranjaClaro =pixels.Color(255,164,104);
uint32_t amarilloClaro =pixels.Color(255,237,91);
uint32_t verdeClaro =pixels.Color(94,196,89);*/
uint32_t naranjaClaro =pixels.Color(40,10,2);
//uint32_t amarilloClaro =pixels.Color(117,113,0); // Buen amarillo
uint32_t amarilloClaro =pixels.Color(10,10,0);
uint32_t verdeClaro =pixels.Color(5,30,15);

uint32_t rojo =pixels.Color(255,0,0);
uint32_t naranja =pixels.Color(150,50,10);
uint32_t verde =pixels.Color(0,80,20);

//Estados del suelo
uint32_t azul =pixels.Color(0, 0, 40);
//Comparte naranja

//Concentración de gases
uint32_t cyan =pixels.Color(0,40,40);
uint32_t morado =pixels.Color(101, 23,135);
uint32_t magenta =pixels.Color(255,0,255);

//Fotosintesis
uint32_t negro =pixels.Color(0,0,0);
uint32_t gris =pixels.Color(80,80,80);
uint32_t blanco =pixels.Color(255,255,255);
uint32_t colorLamp = negro;

int count = 0;

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

  /*****************************  MQ CAlibration ********************************************/ 
  /*Uncomment below in case of calibration*/
  /*Serial.print("Calibrating MQ135 please wait.");
  float calcR0_135 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0_135 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcR0_135/10);
  Serial.println("  done!.");
  
  if(isinf(calcR0_135)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0_135 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  
  Serial.print("Valor de R0 para MQ135");
  Serial.println(MQ135.getR0());*/

  /*Uncomment below in case of calibration*/
  /*Serial.print("Calibrating MQ9 please wait.");
  float calcR0_9 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ9.update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0_9 += MQ9.calibrate(RatioMQ9CleanAir);
    Serial.print(".");
  }
  MQ9.setR0(calcR0_9/10);
  Serial.println("  done!.");
  
  if(isinf(calcR0_9)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0_9 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  
  Serial.print("Valor de R0 para MQ9");
  Serial.println(MQ9.getR0());*/

  /*****************************  MQ CAlibration ********************************************/
  MQ135.setR0(R0value_MQ135); //Uncomment for a fix value
  MQ9.setR0(R0value_MQ9); //Uncomment for a fix value

  //MQ135.serialDebug(true);
  //MQ9.serialDebug(true);

  /*********************************************  LEDS **************************************************/
  pixels.begin();
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
    Serial.print("page0.tMoist.txt=\"HÚMEDO\"");                    //Texto 
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    
    Serial.print("page0.iconMoist.pic=2");                          //Ícono
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    
    pixels.setPixelColor(0, azul);                                  //Led

    soilMoist_state = 0;
    pump_Watertag = 0;
    switch (waterLevel_state) // Cambios de los indicadores
    {
    case 0:
      Serial.print("page0.tTank.txt=\"Lleno\"");                    //Texto
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
      Serial.print("page0.tBtPost.txt=\"\"");       
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

      Serial.print("page0.iconTank.pic=4");                         //Ícono
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
      
      pixels.setPixelColor(3, verdeClaro);                          //Led
      
      break;
    case 1:
      Serial.print("page0.tTank.txt=\"Con agua\"");                 //Texto
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
      Serial.print("page0.tBtPost.txt=\"\"");       
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

      Serial.print("page0.iconTank.pic=5");                        //Ícono
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
      
      pixels.setPixelColor(3, amarilloClaro);                      //Led

      break;
    case 2:
      Serial.print("page0.tTank.txt=\"Vacío\"");                  //Texto
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
      Serial.print("page0.tBtPost.txt=\"\"");       
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
      Serial.print("page0.iconTank.pic=6");                        //Ícono
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
      
      pixels.setPixelColor(3, naranjaClaro);                       //Led

      break;
    default:
      break;
    }
  }
  else // when the soil is DRY
  {
    Serial.print("page0.tMoist.txt=\"SECO\"");                    //Texto
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

    Serial.print("page0.iconMoist.pic=3");                        //Ícono
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

    pixels.setPixelColor(0, naranja);                             //Led

    soilMoist_state = 1;
    switch (waterLevel_state)
    {
    case 0:
      Serial.print("page0.tTank.txt=\"LISTO\"");                    //Texto
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
      Serial.print("page0.tBtPost.txt=\"Presione botón\"");       
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

      Serial.print("page0.iconTank.pic=4");                         //Ícono
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

      pixels.setPixelColor(3, verde);                               //Led

      pump_Watertag = 1;
      break;
    case 1:
      Serial.print("page0.tTank.txt=\"FALTA AGUA\"");               //Texto
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff); 
      Serial.print("page0.tBtPost.txt=\"\"");       
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

      Serial.print("page0.iconTank.pic=5");                         //Ícono
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

      pixels.setPixelColor(3, naranja);                             //Led

      pump_Watertag = 0;
      break;
    case 2:
      Serial.print("page0.tTank.txt=\"¡VACÍO!\"");                  //Texto
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
      Serial.print("page0.tBtPost.txt=\"\"");       
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

      Serial.print("page0.iconTank.pic=6");                         //Ícono
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
      
      pixels.setPixelColor(3, rojo);                                //Led

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

  if (pump_cycle>0) {
      Serial.print("page1.pump_State.txt=\"Riego activado\""); 
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);}
  else {
    Serial.print("page1.pump_State.txt=\"Riego desactivado\"");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);}
  

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
  
  Serial.print("page1.nCiclo.txt=\""+String(pump_cycle)+"\"");
  Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

  //===============================================================================
  // CONTROL DE CALIDAD DE AIRE
  //===============================================================================

  //Contraciones de CO2
  int airQ_state=0;
  //if (CO2_ppm > 250 && CO2_ppm < 400){
  if (CO2_ppm < 400){
    airQ_state=0;
    
    Serial.print("page1.CO2_ppm.txt=\""+String(CO2_ppm)+"\"");     //Texto
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO2_state.txt=\"IDEAL\"");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO2_state.pco=0");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

    pixels.setPixelColor(2, cyan);                                 //Led

  } else if (CO2_ppm > 400 && CO2_ppm < 1000){
    airQ_state=0;
    
    Serial.print("page1.CO2_ppm.txt=\""+String(CO2_ppm)+"\"");     //Texto
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO2_state.txt=\"NORMAL\"");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO2_state.pco=0");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

    pixels.setPixelColor(2, cyan);                                 //Led

  } else if (CO2_ppm > 1000 && CO2_ppm < 2000){
    airQ_state=1;

    Serial.print("page1.CO2_ppm.txt=\""+String(CO2_ppm)+"\"");     //Texto
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO2_state.txt=\"RIESGO MODERADO\"");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO2_state.pco=32799");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

    pixels.setPixelColor(2, morado);                               //Led

  } else if (CO2_ppm > 2000 && CO2_ppm < 5000){
    airQ_state=2;

    Serial.print("page1.CO2_ppm.txt=\""+String(CO2_ppm)+"\"");     //Texto
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO2_state.txt=\"PELIGRO\"");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO2_state.pco=63519");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

    pixels.setPixelColor(2, magenta);                              //Led

  } else if (CO2_ppm > 5000){
    airQ_state=2;

    Serial.print("page1.CO2_ppm.txt=\""+String(CO2_ppm)+"\"");      //Texto
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO2_state.txt=\"ALTO PELIGRO! ⚠️\"");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO2_state.pco=63488");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

    pixels.setPixelColor(2, magenta);                               //Led
  }

   //Contraciones de CO
  if (CO_ppm < 23.8){
    airQ_state=airQ_state+0;

    Serial.print("page1.CO_ppm.txt=\""+String(CO_ppm)+"\"");             //Texto
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO_state.txt=\"NORMAL\"");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO_state.pco=0");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

    pixels.setPixelColor(1, cyan);                                //Led

  } else if (CO_ppm > 23.8 && CO_ppm < 69.02){
    airQ_state=airQ_state+1;

    Serial.print("page1.CO_ppm.txt=\""+String(CO_ppm)+"\"");      //Texto
    Serial.write(0xff); 
    Serial.write(0xff); 
    Serial.write(0xff);
    Serial.print("page1.CO_state.txt=\"PELIGRO\"");
    Serial.write(0xff); 
    Serial.write(0xff); 
    Serial.write(0xff);
    Serial.print("page1.CO_state.pco=63519");
    Serial.write(0xff); 
    Serial.write(0xff); 
    Serial.write(0xff);

    pixels.setPixelColor(1, morado);                               //Led
    
  } else if (CO_ppm > 69.02){
    airQ_state=airQ_state+2;
    Serial.println("Concentración CO2: PELIGRO ALTO");

    Serial.print("page1.CO_ppm.txt=\""+String(CO_ppm)+"\"");       //Texto
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO_state.txt=\"ALTO PELIGRO! ⚠️\"");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page1.CO_state.pco=63488");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);

    pixels.setPixelColor(1,magenta);                                //Led
  }
  
  //Calidad de aire

  if (airQ_state == 0){
    Serial.print("page0.tAirQ.txt=\"BUEN AIRE\"");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page0.tAirQ.pco=0");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page0.iconAirQ.pic=7");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    colorLamp = negro;

  } else if (airQ_state == 1){
    Serial.print("page0.tAirQ.txt=\"CUIDADO\\rConcentración riesgosa\"");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page0.tAirQ.pco=32799");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page0.iconAirQ.pic=8");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    colorLamp = gris;
    
  } else if (airQ_state == 2){
    Serial.print("page0.tAirQ.txt=\"PELIGRO\\rMejora la ventilación\"");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page0.tAirQ.pco=63519");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page0.iconAirQ.pic=9");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    colorLamp = blanco;
  }
  else if (airQ_state >2){
    Serial.print("page0.tAirQ.txt=\"¡ALTO PELIGRO! ⚠️\\rAbre ventanas y sal del lugar\"");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page0.tAirQ.pco=63488");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    Serial.print("page0.iconAirQ.pic=9");
    Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    colorLamp = blanco;
  }

  //===============================================================================
  // LEDS
  //===============================================================================
  
  //Lampara
  for(int i=4;i<numPixels;i++){
    pixels.setPixelColor(i, colorLamp);}

  pixels.show(); 


  //===============================================================================
  // CAMBIO DE CARITAS
  //===============================================================================


  if (airQ_state == 0){
    if (soilMoist_state == 0){ //Wet condition
      Serial.print("page0.icFace.pic=12");
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    }
    else {
      Serial.print("page0.icFace.pic=13"); //Dry
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    }

  } else if (airQ_state == 1){
    if (soilMoist_state == 0){ //Wet condition
      Serial.print("page0.icFace.pic=14");
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    }
    else {
      Serial.print("page0.icFace.pic=16"); //Dry
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    }
    
  } else if (airQ_state > 1){
    if (soilMoist_state == 0){ //Wet condition
      Serial.print("page0.icFace.pic=15");
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    }
    else {
      Serial.print("page0.icFace.pic=16"); //Dry
      Serial.write(0xff); Serial.write(0xff); Serial.write(0xff);
    }
  }
  
  delay(200);

  //===============================================================================
  // PANTALLita :)
  //===============================================================================

  tft.setCursor(4, 10);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(2);
  tft.println("PROTIPO 2");
  
  tft.setTextSize(1);
  tft.println("");
  tft.print("Porcentaje de humedad: ");
  tft.println(perMoist);
  tft.println("");

  tft.print("H2OLvl: ");
  tft.println(waterLevel_state);
  tft.print("PumpStart: ");
  tft.println(pump_start);
  tft.print("+H2Otag: ");
  tft.print(pump_Watertag);
  tft.print(" +PushBt: ");
  tft.println(pump_Buttontag);

  tft.print("Cycle: ");
  tft.println(pump_cycle);

  //tft.print("initialTime: ");
  //tft.println(initialTime);


  //tft.print("HalfTime: ");
  //tft.println(betweenCycle_time);


  //tft.print("Time: ");
  //tft.println(time);
  //tft.println(" ");

  tft.print("pinMOSFET: ");
  tft.println(pinMosfetGate);
  tft.println("");
  tft.print("Air Q: ");
  //tft.println(airQ_state);
  tft.println("");
}


void IRAM_ATTR turnONpump() {
  if (pump_Watertag == 1)  pump_Buttontag = 1;  
  // Cambiar el estado de la variable a 1 cuando se active la interrupción del botón
}