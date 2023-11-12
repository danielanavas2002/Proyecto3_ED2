 //********************************************************************************
// Librerias
//********************************************************************************
#include <SPI.h>
#include <SD.h>
//TFT
#include <stdint.h>
#include <stdbool.h>
#include <TM4C123GH6PM.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

#include "bitmaps.h"
#include "font.h"
#include "lcd_registers.h"
//****************************************************************
// Definición de etiquetas
//****************************************************************
// Botones
#define pinBtnR PF_4 //Boton Recolectar dato del Sensor
#define pinBtnG PF_0 //Boton Guardar 
// SD
#define CS_PIN PA_3
// TFT
#define LCD_RST PD_0
#define LCD_CS PD_1
#define LCD_RS PD_2
#define LCD_WR PD_3
#define LCD_RD PE_1 
int DPINS[] = {PB_0, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7}; 
//****************************************************************
// Variables Globales
//****************************************************************
// Funcionamiento Boton Recolectar dato del Sensor
int btnR_S = LOW; // Estado actual del Boton Recolectar
int btnR_LS = LOW; // Estado anterior del Boton Recolectar
int btnR_R; //Lectura Estado de Boton Recolectar
// Funcionamiento Guardar dato del Sensor
int btnG_S = LOW; // Estado actual del Boton Guardar
int btnG_LS = LOW; // Estado anterior del Boton Guardar
int btnG_R; //Lectura Estado de Boton Guardar
// Antirebote de los Botones
unsigned long lastDebounceTime = 0; // Último momento en que se cambió el estado del botón
unsigned long debounceDelay = 50; // Tiempo de rebote en milisegundos
// Varibles del Sensor
float temp = 0.00;
float tempA = 0.00;
// Datos de Tiempo para Data Log
int segundo;
int minuto;
int hora;
int dia;
// Funcionamiento de Registro de Tiempo
unsigned long previousMillis = 0;  // Variable para almacenar el tiempo anterior
const unsigned long interval = 1000; // Intervalo de 1 segundo en milisegundos
// Textos TFT
String diaString;
String horaString;
String minString;
String segString;
String tempString;
String dospuntos = ":";
String text1 = "MEDICION DE";
String text2 = "TEMPERATURA";
String text3 = " ULTIMO ";
String text4 = "REGISTRO";
String cmtxt = "C";
String errortxt = "ERROR";
String dospuntostxt = ":";
String diastxt = "DIAS:";
int tempDecimal;
int tempEntero;
// Barra de LLenado en Termometro 
int porLlenado;
int posBarra;
float tempMAX = 100.00;
//********************************************************************************
// Escritura SD
//********************************************************************************
File myFile;
//****************************************************************
// Prototipos de Funciones
//****************************************************************
void datalogger(void);
// Pantalla TFT
void LCD_Init(void);
void LCD_CMD(uint8_t cmd);
void LCD_DATA(uint8_t data);
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void LCD_Clear(unsigned int c);
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void LCD_Print(String text, int x, int y, int fontSize, int color, int background);
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]);
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[],int columns, int index, char flip, char offset);
void termometroM(void);
void circuloM(void);
extern uint8_t fondo[];
//*****************************************************************************
// Configuración
//*****************************************************************************
void setup() {
  // Definicion de Entradas
  pinMode(pinBtnR, INPUT_PULLUP);
  pinMode(pinBtnG, INPUT_PULLUP);
  // Monitor Serial 
  Serial.begin(115200); // Con Computadora
  Serial2.begin(115200); // Con ESP32
  // Tarjeta SD
  SPI.setModule(0); // Indicar que el módulo SPI se utilizará
  Serial.print("Inicializando Tarjeta... "); // Inicializar Tarjeta
  if (!SD.begin(CS_PIN)) {
    Serial.println("Inicializacion fallida!"); // Indicar si la inicialización es fallida
    return;
  }
  Serial.println("Inicializacion completa."); // Indicar si la inicialización se completo
  // Crea un nuevo archivo de datos (Si no Existe)
  myFile = SD.open("datalog.txt", FILE_WRITE);
  if (myFile) {
    myFile.println(" "); // Cada que se reinicie la Tiva colocar que es un Nuevo Registro
    myFile.println("************************************"); 
    myFile.println("           NUEVO REGISTRO           ");
    myFile.println("************************************");
    myFile.println("DIA | HORA | MIN | SEG | TEMPERATURA");
    myFile.close();
  } else {
    Serial.println("Error al abrir el archivo de datos."); // Indicar si no se puede abrir el archivo
  }
  // Pantalla TFT
  SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
  GPIOPadConfigSet(GPIO_PORTB_BASE, 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPU);
  LCD_Init();
  LCD_Clear(0x00);
  FillRect(0, 0, 320, 240, 0xFEEB);
  FillRect(0, 0, 320, 30, 0xD505);
  LCD_Print(diastxt, 195, 10, 2, 0x00, 0xD505);
  LCD_Print(text1, 20, 45, 2, 0x00, 0xFEEB);
  LCD_Print(text2, 20, 65, 2, 0x00, 0xFEEB);
  FillRect(15, 85, 185, 45, 0x03F9);
  LCD_Print(text3, 45, 140, 2, 0x00, 0xFEEB);
  LCD_Print(text4, 45, 160, 2, 0x00, 0xFEEB);
  FillRect(40, 180, 135, 55, 0x022B); 
  termometroM();
  circuloM();
}
//*****************************************************************************
// Loop Principal
//*****************************************************************************
void loop() {
  // Lectura de Estado de Botones
  btnR_R = digitalRead(pinBtnR); // Lee el estado actual del Boton Recolectar
  btnG_R = digitalRead(pinBtnG); // Lee el estado actual del Boton Guardar
  // Boton Recolectar
  if (btnR_R != btnR_LS) { 
    lastDebounceTime = millis(); // Si el estado del Boton Recolectar ha cambiado, actualiza el tiempo de rebote
  } if ((millis() - lastDebounceTime) > debounceDelay) { // Verifica si ha pasado suficiente tiempo desde el último cambio del Boton Recolectar para evitar el rebote
    if (btnR_R != btnR_S) { // Si es asi, actualiza el estado del Boton Recolectar
      btnR_S = btnR_R;
      if (btnR_S == LOW) {
        Serial2.println('1'); //Enviar un "1" al ESP32 por medio del Serial 2, para que sepa que debe enviar un dato de distancia
      }
    }
  }
  btnR_LS = btnR_R; // Guarda el estado actual del Boton Recolectar
  // Boton Guardar
  if (btnG_R != btnG_LS) { 
    lastDebounceTime = millis(); // Si el estado del Boton Guardar ha cambiado, actualiza el tiempo de rebote
  } if ((millis() - lastDebounceTime) > debounceDelay) { // Verifica si ha pasado suficiente tiempo desde el último cambio del Boton Guardar para evitar el rebote
    if (btnG_R != btnG_S) { // Si es asi, actualiza el estado del Boton Guardar
      btnG_S = btnG_R;
      if (btnG_S == LOW) {
        Serial2.println('2'); //Enviar un "2" al ESP32 por medio del Serial 2, para que sepa que sepa que se guardo un dato
        datalogger(); // Llamar función data logger para registrar los datos en un archivo de texto en la SD
        delay(200);
      }
    }
  }
  btnG_LS = btnG_R; // Guarda el estado actual del Boton Guardar
  // Lectura del Sensor enviada por el ESP32 enviada por Comunicacion Serial
  if (Serial2.available() > 0) {
    tempA = Serial2.parseFloat();  // Lee el nuevo valor de temperatura desde la comunicación serial
    if (tempA != 0.00 && tempA != temp) {
      temp = tempA; // Actualiza la temperatura anterior cuando esta cambia
      if (temp > tempMAX){ // Evitar que pase de Temperatura Max
        temp = tempMAX;
        }
       tempEntero = int(temp);  // Obtiene la parte entera
       tempDecimal = (temp - tempEntero)*100;  // Multiplica por 100 para obtener 2 decimales
       if(tempEntero < 10){
        tempString = "0" + String(tempEntero) + "." + String(tempDecimal); //Arreglar a String
       } else{
        tempString = String(tempEntero) + "." + String(tempDecimal); //Arreglar a String
       }
       FillRect(15, 85, 135, 45, 0x03F9); //Mostrar en TFT
       LCD_Print(tempString, 50, 100, 2, 0xffff, 0x03F9);
       LCD_Print(cmtxt, 140, 100, 2, 0xffff, 0x03F9);
       // Actualizar Barra del Termometro en TFT
       porLlenado = map(temp, 0.00, tempMAX, 0, 117);
       if (porLlenado > 117){
        porLlenado = 117;
       }
       posBarra = 117 - porLlenado + 64;
       FillRect(247, 64, 6, 117, 0xFEEB);
       FillRect(247, posBarra, 6, porLlenado, 0xE3AE);
    }
  }
  //Contador para Registro de Tiempo
  unsigned long currentMillis = millis(); // Obtiene el tiempo actual
  if (currentMillis - previousMillis >= interval) { // Comprueba si ha pasado el intervalo de 1 segundo
    previousMillis = currentMillis; // Guarda el tiempo actual como el tiempo anterior
    segundo++; //Aumentar el Tiempo 1 Segundo
  }
  if (segundo >= 60){ //Cuando Segundos llegue a 60 
    segundo = 0; // Reiniciar Segundos
    minuto++; //Aumentar un minuto
    if(minuto >= 60){ //Cuando Minutos llegue a 60
      minuto = 0; // Reiniciar Minutos
      hora++; //Aumentar una hora
      if(hora >= 24){ //Cuando Horas llegue a 24
        hora = 0; // Reiniciar Horas
        dia++; //Aumentar una dia 
      }
    } 
  }
  if(segundo < 10){
    segString = "0" + String(segundo); //Arreglar a String
  } else{
    segString = String(segundo);
  }
  if(minuto < 10){
    minString = "0" + String(minuto); //Arreglar a String
  } else{
    minString = String(minuto);
  }
  if(hora < 10){
    horaString = "0" + String(hora); //Arreglar a String
  } else{
    horaString = String(hora);
  }
  if(dia < 10){
    diaString = "0" + String(dia); //Arreglar a String
  } else{
    diaString = String(dia);
  } //Actualizar en TFT
  LCD_Print(horaString, 10, 10, 2, 0x00, 0xD505);
  LCD_Print(dospuntostxt, 40, 10, 2, 0x00, 0xD505);
  LCD_Print(minString, 55, 10, 2, 0x00, 0xD505);
  LCD_Print(dospuntostxt, 85, 10, 2, 0x00, 0xD505);
  LCD_Print(segString, 100, 10, 2, 0x00, 0xD505);
  LCD_Print(diaString, 275, 10, 2, 0x00, 0xD505);
  
}
//****************************************************************
// Data Logger
//****************************************************************
void datalogger(void){
  myFile = SD.open("datalog.txt", FILE_WRITE); // Abrir Archivo de datos como Escritura
  if (myFile) {
    // Escribe tiempo y temperatura en el archivo de datos
    if(dia < 10){
      myFile.print("0");
      myFile.print(dia);
      myFile.print("  | ");
     } else{
        myFile.print(dia);
        myFile.print("  | ");
       }
     if(hora < 10){
      myFile.print("0");
      myFile.print(hora);
      myFile.print("   | ");
     } else{
        myFile.print(hora);
        myFile.print("   | ");
       }
     if(minuto < 10){
      myFile.print("0");
      myFile.print(minuto);
      myFile.print("  | ");
     } else{
        myFile.print(minuto);
        myFile.print("  | ");
       }
     if(segundo < 10){
      myFile.print("0");
      myFile.print(segundo);
      myFile.print("  | ");
     } else{
        myFile.print(segundo);
        myFile.print("  | ");
       }
    myFile.print(temp);
    myFile.println(" °C");
    myFile.close();
    FillRect(40, 180, 135, 55, 0x022B); //Mostrar en TFT
    LCD_Print(horaString, 45, 190, 2, 0xffff, 0x022B); //45
    LCD_Print(dospuntostxt, 75, 190, 2, 0xffff, 0x022B);
    LCD_Print(minString, 90, 190, 2, 0xffff, 0x022B);
    LCD_Print(dospuntostxt, 120, 190, 2, 0xffff, 0x022B);
    LCD_Print(segString, 135, 190, 2, 0xffff, 0x022B);
    LCD_Print(diastxt, 55, 210, 2, 0xffff, 0x022B);
    LCD_Print(diaString, 135, 210, 2, 0xffff, 0x022B);
    Serial.println("Dato registrados en la tarjeta SD."); // Indicar que se guardaron los datos
  } else {
    Serial.println("Error al abrir el archivo de datos."); // Indicar si hay un error con el archivo
    FillRect(15, 180, 135, 55, 0xD7C444);
    LCD_Print(errortxt, 20, 180, 2, 0xffff, 0xD7C444);
  } 
 }
 //***************************************************************************************************************************************
// Función para inicializar LCD
//***************************************************************************************************************************************
void LCD_Init(void) {
  pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_WR, OUTPUT);
  pinMode(LCD_RD, OUTPUT);
  for (uint8_t i = 0; i < 8; i++){
    pinMode(DPINS[i], OUTPUT);
  }
  //****************************************
  // Secuencia de Inicialización
  //****************************************
  digitalWrite(LCD_CS, HIGH);
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_WR, HIGH);
  digitalWrite(LCD_RD, HIGH);
  digitalWrite(LCD_RST, HIGH);
  delay(5);
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);
  digitalWrite(LCD_CS, LOW);
  //****************************************
  LCD_CMD(0xE9);  // SETPANELRELATED
  LCD_DATA(0x20);
  //****************************************
  LCD_CMD(0x11); // Exit Sleep SLEEP OUT (SLPOUT)
  delay(100);
  //****************************************
  LCD_CMD(0xD1);    // (SETVCOM)
  LCD_DATA(0x00);
  LCD_DATA(0x71);
  LCD_DATA(0x19);
  //****************************************
  LCD_CMD(0xD0);   // (SETPOWER) 
  LCD_DATA(0x07);
  LCD_DATA(0x01);
  LCD_DATA(0x08);
  //****************************************
  LCD_CMD(0x36);  // (MEMORYACCESS)
  LCD_DATA(0x40|0x80|0x20|0x08); // LCD_DATA(0x19);
  //****************************************
  LCD_CMD(0x3A); // Set_pixel_format (PIXELFORMAT)
  LCD_DATA(0x05); // color setings, 05h - 16bit pixel, 11h - 3bit pixel
  //****************************************
  LCD_CMD(0xC1);    // (POWERCONTROL2)
  LCD_DATA(0x10);
  LCD_DATA(0x10);
  LCD_DATA(0x02);
  LCD_DATA(0x02);
  //****************************************
  LCD_CMD(0xC0); // Set Default Gamma (POWERCONTROL1)
  LCD_DATA(0x00);
  LCD_DATA(0x35);
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x02);
  //****************************************
  LCD_CMD(0xC5); // Set Frame Rate (VCOMCONTROL1)
  LCD_DATA(0x04); // 72Hz
  //****************************************
  LCD_CMD(0xD2); // Power Settings  (SETPWRNORMAL)
  LCD_DATA(0x01);
  LCD_DATA(0x44);
  //****************************************
  LCD_CMD(0xC8); //Set Gamma  (GAMMASET)
  LCD_DATA(0x04);
  LCD_DATA(0x67);
  LCD_DATA(0x35);
  LCD_DATA(0x04);
  LCD_DATA(0x08);
  LCD_DATA(0x06);
  LCD_DATA(0x24);
  LCD_DATA(0x01);
  LCD_DATA(0x37);
  LCD_DATA(0x40);
  LCD_DATA(0x03);
  LCD_DATA(0x10);
  LCD_DATA(0x08);
  LCD_DATA(0x80);
  LCD_DATA(0x00);
  //****************************************
  LCD_CMD(0x2A); // Set_column_address 320px (CASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0x3F);
  //****************************************
  LCD_CMD(0x2B); // Set_page_address 480px (PASET)
  LCD_DATA(0x00);
  LCD_DATA(0x00);
  LCD_DATA(0x01);
  LCD_DATA(0xE0);
//  LCD_DATA(0x8F);
  LCD_CMD(0x29); //display on 
  LCD_CMD(0x2C); //display on

  LCD_CMD(ILI9341_INVOFF); //Invert Off
  delay(120);
  LCD_CMD(ILI9341_SLPOUT);    //Exit Sleep
  delay(120);
  LCD_CMD(ILI9341_DISPON);    //Display on
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para enviar comandos a la LCD - parámetro (comando)
//***************************************************************************************************************************************
void LCD_CMD(uint8_t cmd) {
  digitalWrite(LCD_RS, LOW);
  digitalWrite(LCD_WR, LOW);
  GPIO_PORTB_DATA_R = cmd;
  digitalWrite(LCD_WR, HIGH);
}
//***************************************************************************************************************************************
// Función para enviar datos a la LCD - parámetro (dato)
//***************************************************************************************************************************************
void LCD_DATA(uint8_t data) {
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_WR, LOW);
  GPIO_PORTB_DATA_R = data;
  digitalWrite(LCD_WR, HIGH);
}
//***************************************************************************************************************************************
// Función para definir rango de direcciones de memoria con las cuales se trabajara (se define una ventana)
//***************************************************************************************************************************************
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) {
  LCD_CMD(0x2a); // Set_column_address 4 parameters
  LCD_DATA(x1 >> 8);
  LCD_DATA(x1);   
  LCD_DATA(x2 >> 8);
  LCD_DATA(x2);   
  LCD_CMD(0x2b); // Set_page_address 4 parameters
  LCD_DATA(y1 >> 8);
  LCD_DATA(y1);   
  LCD_DATA(y2 >> 8);
  LCD_DATA(y2);   
  LCD_CMD(0x2c); // Write_memory_start
}
//***************************************************************************************************************************************
// Función para borrar la pantalla - parámetros (color)
//***************************************************************************************************************************************
void LCD_Clear(unsigned int c){  
  unsigned int x, y;
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);   
  SetWindows(0, 0, 319, 239); // 479, 319);
  for (x = 0; x < 320; x++)
    for (y = 0; y < 240; y++) {
      LCD_DATA(c >> 8); 
      LCD_DATA(c); 
    }
  digitalWrite(LCD_CS, HIGH);
} 
//***************************************************************************************************************************************
// Función para dibujar una línea horizontal - parámetros ( coordenada x, cordenada y, longitud, color)
//*************************************************************************************************************************************** 
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {  
  unsigned int i, j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + x;
  SetWindows(x, y, l, y);
  j = l;// * 2;
  for (i = 0; i < l; i++) {
      LCD_DATA(c >> 8); 
      LCD_DATA(c); 
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar una línea vertical - parámetros ( coordenada x, cordenada y, longitud, color)
//*************************************************************************************************************************************** 
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {  
  unsigned int i,j;
  LCD_CMD(0x02c); //write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW);
  l = l + y;
  SetWindows(x, y, x, l);
  j = l; //* 2;
  for (i = 1; i <= j; i++) {
    LCD_DATA(c >> 8); 
    LCD_DATA(c);
  }
  digitalWrite(LCD_CS, HIGH);  
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  H_line(x  , y  , w, c);
  H_line(x  , y+h, w, c);
  V_line(x  , y  , h, c);
  V_line(x+w, y  , h, c);
}
//***************************************************************************************************************************************
// Función para dibujar el Termometro
//***************************************************************************************************************************************
void termometroM(void){
  FillRect(243, 46, 12, 1, 0x0000);
  FillRect(241, 47, 16, 1, 0x0000);
  FillRect(240, 48, 19, 1, 0x0000);
  FillRect(238, 49, 23, 1, 0x0000);
  FillRect(237, 50, 26, 1, 0x0000);
  FillRect(236, 51, 28, 1, 0x0000);
  FillRect(235, 52, 30, 1, 0x0000);
  FillRect(234, 53, 32, 1, 0x0000);
  FillRect(233, 54, 12, 1, 0x0000);
  FillRect(254, 54, 12, 1, 0x0000);
  FillRect(233, 55, 10, 1, 0x0000);
  FillRect(256, 55, 11, 1, 0x0000);
  FillRect(232, 56, 10, 1, 0x0000);
  FillRect(258, 56, 9, 1, 0x0000);
  FillRect(232, 57, 9, 1, 0x0000);
  FillRect(259, 57, 9, 1, 0x0000);
  FillRect(231, 58, 9, 2, 0x0000);
  FillRect(260, 58, 8, 2, 0x0000);
  FillRect(268, 59, 1, 1, 0x0000);
  FillRect(231, 60, 8, 2, 0x0000);
  FillRect(261, 60, 8, 3, 0x0000);
  FillRect(230, 61, 8, 115, 0x0000);
  FillRect(262, 63, 7, 114, 0x0000);
  FillRect(229, 172, 1, 4, 0x0000);
  FillRect(228, 173, 1, 3, 0x0000);
  FillRect(227, 174, 1, 2, 0x0000);
  FillRect(226, 175, 1, 1, 0x0000);
  FillRect(225, 176, 12, 1, 0x0000);
  FillRect(224, 177, 12, 1, 0x0000);
  FillRect(223, 178, 11, 1, 0x0000);
  FillRect(223, 179, 10, 1, 0x0000);
  FillRect(222, 180, 10, 1, 0x0000);
  FillRect(221, 181, 10, 1, 0x0000);
  FillRect(220, 182, 10, 1, 0x0000);
  FillRect(220, 183, 9, 1, 0x0000);
  FillRect(219, 184, 9, 2, 0x0000);
  FillRect(218, 186, 9, 2, 0x0000);
  FillRect(218, 188, 8, 2, 0x0000);
  FillRect(217, 189, 9, 2, 0x0000);
  FillRect(217, 191, 8, 3, 0x0000);
  FillRect(217, 194, 7, 1, 0x0000);
  FillRect(216, 195, 8, 11, 0x0000);
  FillRect(217, 206, 7, 1, 0x0000);
  FillRect(217, 207, 8, 3, 0x0000);
  FillRect(217, 210, 9, 1, 0x0000);
  FillRect(218, 211, 8, 1, 0x0000);
  FillRect(218, 212, 9, 2, 0x0000);
  FillRect(219, 214, 9, 2, 0x0000);
  FillRect(220, 216, 9, 1, 0x0000);
  FillRect(220, 217, 10, 1, 0x0000);
  FillRect(221, 218, 10, 1, 0x0000);
  FillRect(222, 219, 10, 1, 0x0000);
  FillRect(222, 220, 11, 1, 0x0000);
  FillRect(223, 221, 11, 1, 0x0000);
  FillRect(224, 222, 12, 1, 0x0000);
  FillRect(225, 223, 12, 1, 0x0000);
  FillRect(226, 224, 13, 1, 0x0000);
  FillRect(227, 225, 15, 1, 0x0000);
  FillRect(228, 226, 18, 1, 0x0000);
  FillRect(229, 227, 42, 1, 0x0000);
  FillRect(230, 228, 40, 1, 0x0000);
  FillRect(232, 229, 36, 1, 0x0000);
  FillRect(233, 230, 34, 1, 0x0000);
  FillRect(235, 231, 31, 1, 0x0000);
  FillRect(237, 232, 25, 1, 0x0000);
  FillRect(241, 233, 18, 1, 0x0000);
  FillRect(269, 173, 1, 4, 0x0000);
  FillRect(270, 174, 1, 3, 0x0000);
  FillRect(271, 175, 1, 2, 0x0000);
  FillRect(272, 176, 1, 1, 0x0000);
  FillRect(263, 177, 11, 1, 0x0000);
  FillRect(264, 178, 11, 1, 0x0000);
  FillRect(265, 179, 11, 1, 0x0000);
  FillRect(267, 180, 10, 1, 0x0000);
  FillRect(268, 181, 10, 1, 0x0000);
  FillRect(269, 182, 10, 1, 0x0000);
  FillRect(270, 183, 9, 1, 0x0000);
  FillRect(270, 184, 10, 1, 0x0000);
  FillRect(271, 185, 9, 1, 0x0000);
  FillRect(272, 186, 9, 1, 0x0000);
  FillRect(273, 187, 8, 1, 0x0000);
  FillRect(273, 188, 9, 1, 0x0000);
  FillRect(274, 189, 8, 2, 0x0000);
  FillRect(275, 191, 8, 3, 0x0000);
  FillRect(276, 194, 7, 2, 0x0000);
  FillRect(276, 196, 8, 9, 0x0000);
  FillRect(276, 205, 7, 2, 0x0000);
  FillRect(275, 207, 8, 3, 0x0000);
  FillRect(274, 210, 8, 2, 0x0000);
  FillRect(273, 212, 9, 1, 0x0000);
  FillRect(273, 213, 8, 1, 0x0000);
  FillRect(272, 214, 9, 1, 0x0000);
  FillRect(271, 215, 9, 2, 0x0000);
  FillRect(270, 217, 9, 1, 0x0000);
  FillRect(269, 218, 10, 1, 0x0000);
  FillRect(268, 219, 10, 1, 0x0000);
  FillRect(267, 220, 11, 1, 0x0000);
  FillRect(266, 221, 11, 1, 0x0000);
  FillRect(265, 222, 12, 1, 0x0000);
  FillRect(264, 223, 12, 1, 0x0000);
  FillRect(262, 224, 13, 1, 0x0000);
  FillRect(259, 225, 15, 1, 0x0000);
  FillRect(255, 226, 18, 1, 0x0000);
  //Lineas
  FillRect(277, 69, 23, 1, 0x0000);
  FillRect(276, 70, 25, 1, 0x0000);
  FillRect(275, 71, 27, 3, 0x0000);
  FillRect(276, 74, 25, 1, 0x0000);
  FillRect(277, 75, 23, 1, 0x0000);

  FillRect(278, 83, 13, 1, 0x0000);
  FillRect(277, 84, 15, 1, 0x0000);
  FillRect(276, 85, 17, 4, 0x0000);
  FillRect(277, 89, 15, 1, 0x0000);
  FillRect(278, 90, 13, 1, 0x0000);

  FillRect(277, 98, 23, 1, 0x0000);
  FillRect(276, 99, 25, 1, 0x0000);
  FillRect(275, 100, 27, 3, 0x0000);
  FillRect(276, 103, 25, 1, 0x0000);
  FillRect(277, 104, 23, 1, 0x0000);

  FillRect(278, 112, 13, 1, 0x0000);
  FillRect(277, 113, 15, 1, 0x0000);
  FillRect(276, 114, 17, 4, 0x0000);
  FillRect(277, 118, 15, 1, 0x0000);
  FillRect(278, 119, 13, 1, 0x0000);

  FillRect(277, 127, 23, 1, 0x0000);
  FillRect(276, 128, 25, 1, 0x0000);
  FillRect(275, 129, 27, 3, 0x0000);
  FillRect(276, 132, 25, 1, 0x0000);
  FillRect(277, 133, 23, 1, 0x0000);

  FillRect(278, 141, 13, 1, 0x0000);
  FillRect(277, 142, 15, 1, 0x0000);
  FillRect(276, 143, 17, 4, 0x0000);
  FillRect(277, 147, 15, 1, 0x0000);
  FillRect(278, 148, 13, 1, 0x0000);

  FillRect(277, 156, 23, 1, 0x0000);
  FillRect(276, 157, 25, 1, 0x0000);
  FillRect(275, 158, 27, 3, 0x0000);
  FillRect(276, 161, 25, 1, 0x0000);
  FillRect(277, 162, 23, 1, 0x0000);

  //Interno
  FillRect(248, 59, 4, 1, 0x0000);
  FillRect(247, 60, 6, 1, 0x0000);
  FillRect(246, 61, 8, 1, 0x0000);
  FillRect(245, 62, 4, 1, 0x0000);
  FillRect(251, 62, 4, 1, 0x0000);
  FillRect(245, 63, 3, 1, 0x0000);
  FillRect(252, 63, 3, 1, 0x0000);
  FillRect(245, 64, 2, 119, 0x0000);
  FillRect(253, 64, 2, 119, 0x0000);
  
  FillRect(242, 182, 3, 2, 0x0000);
  FillRect(240, 183, 2, 2, 0x0000);
  FillRect(238, 184, 2, 2, 0x0000);
  FillRect(237, 185, 1, 2, 0x0000);
  FillRect(236, 186, 1, 2, 0x0000);
  FillRect(235, 187, 1, 2, 0x0000);
  FillRect(234, 188, 1, 1, 0x0000);
  FillRect(233, 189, 2, 2, 0x0000);
  FillRect(232, 191, 2, 1, 0x0000);
  FillRect(231, 192, 2, 3, 0x0000);
  FillRect(230, 195, 2, 3, 0x0000);
  FillRect(229, 198, 2, 9, 0x0000);
  FillRect(230, 207, 2, 3, 0x0000);
  FillRect(231, 210, 2, 3, 0x0000);
  FillRect(232, 213, 2, 1, 0x0000);
  FillRect(233, 214, 2, 2, 0x0000);
  FillRect(234, 216, 2, 1, 0x0000);
  FillRect(235, 217, 2, 1, 0x0000);
  FillRect(236, 218, 2, 1, 0x0000);
  FillRect(237, 219, 3, 1, 0x0000);
  FillRect(238, 220, 4, 1, 0x0000);
  FillRect(240, 221, 5, 1, 0x0000);
  FillRect(242, 222, 16, 1, 0x0000);
  FillRect(245, 223, 10, 1, 0x0000);

  FillRect(255, 182, 3, 2, 0x0000);
  FillRect(258, 183, 2, 2, 0x0000);
  FillRect(260, 184, 2, 2, 0x0000);
  FillRect(262, 185, 1, 2, 0x0000);
  FillRect(263, 186, 1, 2, 0x0000);
  FillRect(264, 187, 1, 2, 0x0000);
  FillRect(265, 188, 1, 3, 0x0000);
  FillRect(266, 189, 1, 3, 0x0000);
  FillRect(267, 191, 1, 1, 0x0000);
  FillRect(267, 192, 2, 3, 0x0000);
  FillRect(268, 195, 2, 3, 0x0000);
  FillRect(269, 198, 2, 9, 0x0000);
  FillRect(268, 207, 2, 3, 0x0000);
  FillRect(267, 210, 2, 3, 0x0000);
  FillRect(266, 213, 2, 1, 0x0000);
  FillRect(265, 214, 2, 2, 0x0000);
  FillRect(264, 216, 2, 1, 0x0000);
  FillRect(263, 217, 2, 1, 0x0000);
  FillRect(262, 218, 2, 1, 0x0000);
  FillRect(260, 219, 3, 1, 0x0000);
  FillRect(258, 220, 4, 1, 0x0000);
  FillRect(255, 221, 5, 1, 0x0000);
  }

void circuloM(void){
  FillRect(247, 181, 6, 2, 0xE3AE);
  FillRect(245, 183, 10, 1, 0xE3AE);
  FillRect(242, 184, 16, 1, 0xE3AE);
  FillRect(240, 185, 20, 1, 0xE3AE);
  FillRect(238, 186, 24, 1, 0xE3AE);
  FillRect(237, 187, 26, 1, 0xE3AE);
  FillRect(236, 188, 28, 1, 0xE3AE);
  FillRect(235, 189, 30, 2, 0xE3AE);
  FillRect(234, 191, 32, 1, 0xE3AE);
  FillRect(233, 192, 34, 3, 0xE3AE);
  FillRect(232, 195, 36, 3, 0xE3AE);
  FillRect(231, 198, 38, 9, 0xE3AE);
  FillRect(232, 207, 36, 3, 0xE3AE);
  FillRect(233, 210, 34, 3, 0xE3AE);
  FillRect(234, 213, 32, 1, 0xE3AE);
  FillRect(235, 214, 30, 2, 0xE3AE);
  FillRect(236, 216, 28, 1, 0xE3AE);
  FillRect(237, 217, 26, 1, 0xE3AE);
  FillRect(238, 218, 24, 1, 0xE3AE);
  FillRect(240, 219, 20, 1, 0xE3AE);
  FillRect(242, 220, 16, 1, 0xE3AE);
  FillRect(245, 221, 10, 1, 0xE3AE);
  }
//***************************************************************************************************************************************
// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
/*void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  unsigned int i;
  for (i = 0; i < h; i++) {
    H_line(x  , y  , w, c);
    H_line(x  , y+i, w, c);
  }
}
*/ 
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c) {
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW); 
  
  unsigned int x2, y2;
  x2 = x+w;
  y2 = y+h;
  SetWindows(x, y, x2-1, y2-1);
  unsigned int k = w*h*2-1;
  unsigned int i, j;
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      LCD_DATA(c >> 8);
      LCD_DATA(c);
      
      //LCD_DATA(bitmap[k]);    
      k = k - 2;
     } 
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar texto - parámetros ( texto, coordenada x, cordenada y, color, background) 
//***************************************************************************************************************************************
void LCD_Print(String text, int x, int y, int fontSize, int color, int background) {
  int fontXSize ;
  int fontYSize ;
  
  if(fontSize == 1){
    fontXSize = fontXSizeSmal ;
    fontYSize = fontYSizeSmal ;
  }
  if(fontSize == 2){
    fontXSize = fontXSizeBig ;
    fontYSize = fontYSizeBig ;
  }
  
  char charInput ;
  int cLength = text.length();
  Serial.println(cLength,DEC);
  int charDec ;
  int c ;
  int charHex ;
  char char_array[cLength+1];
  text.toCharArray(char_array, cLength+1) ;
  for (int i = 0; i < cLength ; i++) {
    charInput = char_array[i];
    Serial.println(char_array[i]);
    charDec = int(charInput);
    digitalWrite(LCD_CS, LOW);
    SetWindows(x + (i * fontXSize), y, x + (i * fontXSize) + fontXSize - 1, y + fontYSize );
    long charHex1 ;
    for ( int n = 0 ; n < fontYSize ; n++ ) {
      if (fontSize == 1){
        charHex1 = pgm_read_word_near(smallFont + ((charDec - 32) * fontYSize) + n);
      }
      if (fontSize == 2){
        charHex1 = pgm_read_word_near(bigFont + ((charDec - 32) * fontYSize) + n);
      }
      for (int t = 1; t < fontXSize + 1 ; t++) {
        if (( charHex1 & (1 << (fontXSize - t))) > 0 ) {
          c = color ;
        } else {
          c = background ;
        }
        LCD_DATA(c >> 8);
        LCD_DATA(c);
      }
    }
    digitalWrite(LCD_CS, HIGH);
  }
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
//***************************************************************************************************************************************
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]){  
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW); 
  
  unsigned int x2, y2;
  x2 = x+width;
  y2 = y+height;
  SetWindows(x, y, x2-1, y2-1);
  unsigned int k = 0;
  unsigned int i, j;

  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k+1]);
      //LCD_DATA(bitmap[k]);    
      k = k + 2;
     } 
  }
  digitalWrite(LCD_CS, HIGH);
}
//***************************************************************************************************************************************
// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
//***************************************************************************************************************************************
void LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[],int columns, int index, char flip, char offset){
  LCD_CMD(0x02c); // write_memory_start
  digitalWrite(LCD_RS, HIGH);
  digitalWrite(LCD_CS, LOW); 

  unsigned int x2, y2;
  x2 =   x+width;
  y2=    y+height;
  SetWindows(x, y, x2-1, y2-1);
  int k = 0;
  int ancho = ((width*columns));
  if(flip){
  for (int j = 0; j < height; j++){
      k = (j*(ancho) + index*width -1 - offset)*2;
      k = k+width*2;
     for (int i = 0; i < width; i++){
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k+1]);
      k = k - 2;
     } 
  }
  }else{
     for (int j = 0; j < height; j++){
      k = (j*(ancho) + index*width + 1 + offset)*2;
     for (int i = 0; i < width; i++){
      LCD_DATA(bitmap[k]);
      LCD_DATA(bitmap[k+1]);
      k = k + 2;
     } 
  }
    
    
    }
  digitalWrite(LCD_CS, HIGH);
}
