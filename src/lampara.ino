#include <Adafruit_NeoPixel.h
#include <avr/wdt.h>

//SENSOR
int pinRest=A0; //Flexiforce sensor conectado al pin A0
int dataRest;
float lectura;
float umbralInicio =50.0;

//POMODORO
const int trabajoDuracion = 30;  // Duración del período de trabajo en minutos
const int descansoDuracion = 5;  // Duración del período de descanso en minutos
const int totalDuracion = 150;  // Duración total en minutos (2 horas y 30 minutos)

unsigned long tiempoInicio;
boolean enTrabajo = true;
boolean enPomodoro = false;

//LEDS
const int pinLed = 7;
const int totalLEDs = 24; // Recomendable pasarlo a 10 leds
int ledFadeTime = 5; //...........................................................................................................
uint32_t amarilloClaro =pixels.Color(10,10,0);
uint32_t blanco =pixels.Color(255,255,255);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(totalLEDs, pinLed, NEO_GRB + NEO_KHZ800);


void setup(){
    Serial.begin(9600);
    pinMode(pinRest,INPUT);
    
    tiempoInicio = millis();

    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
}

void loop(){
    dataRest=analogRead(pinRest);
    lectura=(dataRest*5.0)/1023.0;
    Serial.println(lectura);

    
    //rgbFadeInAndOut(0, 0, 255, ledFadeTime) //...................................................................................

    /*
    if (!enPomodoro && lectura > umbralInicio) { // Cuando el pomodoro este apagado y el celular este puesto
    enPomodoro = true;
    tiempoInicio = millis();  // Iniciar el temporizador Pomodoro
    }

    if (lectura < umbralInicio) { // Cuando el pomodoro este apagado y el celular este puesto
        enPomodoro = false;
        tiempoInicio = millis();  // Iniciar el temporizador Pomodoro
        Serial.println("Pomodoro interrumpido.");
    }

    if (enPomodoro) { // Cuando el pomodoro este encendido
        unsigned long tiempoActual = millis();
        unsigned long tiempoTranscurrido = (tiempoActual - tiempoInicio) / 1000;  // Convertir a segundos

        if (enTrabajo) {
        if (tiempoTranscurrido >= trabajoDuracion * 60) {
            enTrabajo = false;
            tiempoInicio = millis();  // Reiniciar el temporizador para el período de descanso
        }
        } 
        else {
            if (tiempoTranscurrido >= descansoDuracion * 60) {
                enTrabajo = true;
                tiempoInicio = millis();  // Reiniciar el temporizador para el próximo período de trabajo
        }
        }

        if (tiempoTranscurrido >= totalDuracion * 60) {
            // Se ha completado el ciclo Pomodoro, puedes realizar alguna acción aquí si es necesario
            Serial.println("Pomodoro completado.");
            enPomodoro = false;  // Terminar el Pomodoro
        }

        // Puedes realizar otras acciones en función de si estás en el período de trabajo o descanso
        if (enTrabajo) {
        Serial.println("¡Trabajo!");
        for(int i=1;i<numPixels;i++){
            pixels.setPixelColor(i, blanco);}

        // Realizar acciones relacionadas con el trabajo aquí
        } else {
        Serial.println("¡Descanso!");
        for(int i=1;i<numPixels;i++){
            pixels.setPixelColor(i, amarilloClaro);}

        // Realizar acciones relacionadas con el descanso aquí
        }
    }

    delay(1000);  // Esperar 1 segundo antes de verificar nuevamente

    */
}


void rgbFadeInAndOut(uint8_t wait) {

  uint8_t red [totalLEDs];
  uint8_t green [totalLEDs];
  uint8_t blue [totalLEDs];

  // randomly set each colour
  for (int i = 0; i < totalLEDs; i++)
    {
    red [i] = random(256);
    green [i] = random(256);
    blue [i] = random(256);
    }

  for(uint8_t b = 0; b <255; b++) {
     for(uint8_t i=0; i < strip.totalLEDs(); i++) {
        strip.setPixelColor(i, red [i] * b/255, green [i] * b/255, blue [i] * b/255);
     }

     strip.show();
     delay(wait);
  }

  for(uint8_t b=255; b > 0; b--) {
     for(uint8_t i = 0; i < strip.totalLEDs(); i++) {
        strip.setPixelColor(i, red [i] * b/255, green [i] * b/255, blue [i] * b/255);
     }
     strip.show();
     delay(wait);
  }
}