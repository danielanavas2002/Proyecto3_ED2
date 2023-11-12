//****************************************************************
// Librerías
//****************************************************************
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <Temperature_LM75_Derived.h>
//****************************************************************
// Definición de etiquetas
//****************************************************************
#define PIN_NEO 14 //NeoPixel
Generic_LM75 temperature; //Temperatura
//****************************************************************
// Variables Globales
//****************************************************************
// Temperatura medida por Sensor
float temperatura;
// Comado recibido de la Tiva
int comando;
//****************************************************************
// Prototipos de Funciones
//****************************************************************
void colorWipe(uint32_t c, uint8_t wait);
void rainbow(uint8_t wait);
void rainbowCycle(uint8_t wait);
void theaterChase(uint32_t c, uint8_t wait);
void theaterChaseRainbow(uint8_t wait);
uint32_t Wheel(byte WheelPos);
//****************************************************************
// NeoPixel
//****************************************************************
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16 , PIN_NEO, NEO_GRB + NEO_KHZ800);
//****************************************************************
// Configuración
//****************************************************************
void setup() {
  // Monitor Serial
  Serial.begin(115200); // Con Computadora
  Serial1.begin(115200); // Con Tiva
  // NeoPixel
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  strip.begin();
  strip.setBrightness(50);
  strip.show(); // Iniciar 'off'
  Wire.begin();
}
//****************************************************************
// Loop Principal
//****************************************************************
void loop() {
  if(Serial1.available() > 0){ //Cuando se reciba un dato en el Serial 1
    comando = Serial1.read(); // Leer este valor y almacenar en variable comando
  }
  if(comando == '1'){ // Si comando es igual a 1
    temperatura = temperature.readTemperatureC(); // Se mide la temperatra utilizando la Libreria del Sensor
    if (temperatura <= 0){ //Si en algun momento se da una medicion erronea y se tiene un numero negativo
      temperatura = 0.01; //Enviar un dato mayor a 0 para evitar problemas en la Tiva
    }
    Serial1.println(temperatura); // Enviar este dato en el Serial 1 por UART hacia la Tiva
    Serial.print("Dato enviado: "); //Enviar dato en el Serial 0 por UART hacia la computadora
    Serial.print(temperatura);
    Serial.println(" °C");
    colorWipe(strip.Color(127, 127, 127), 25); // Indicador Visual - Lectura
    colorWipe(strip.Color(0, 0, 0), 25); 
    if (temperatura <= 25){
    colorWipe(strip.Color(50, 50, 255), 50); // Indicador Visual - Temperatura Baja
    } if (temperatura > 25 && temperatura < 40){
      colorWipe(strip.Color(255, 255, 50), 50); // Indicador Visual - Temperatura Normal
    } if (temperatura >= 40){
      colorWipe(strip.Color(255, 50, 50), 50); // Indicador Visual - Temperatura Alta
    }
    comando = 0; // Reiniciar comado
  }

  
  if(comando == '2'){ // Si comando es igual a 2 (Guardar dato en SD)
    colorWipe(strip.Color(50, 255, 50), 25); // Indicador Visual - Dato Guardado
    colorWipe(strip.Color(0, 0, 0), 25); 
    if (temperatura <= 25){
    colorWipe(strip.Color(50, 50, 255), 25); // Indicador Visual - Temperatura Baja
    } if (temperatura > 25 && temperatura < 40){
      colorWipe(strip.Color(255, 255, 50), 25); // Indicador Visual - Temperatura Normal
    } if (temperatura >= 40){
      colorWipe(strip.Color(255, 50, 50), 25); // Indicador Visual - Temperatura Alta
    }
    comando = 0; // Reiniciar comado
  }

}
//****************************************************************
// Funciones de Neopixel
//****************************************************************
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
