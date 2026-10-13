/*
  DIAGNÓSTICO DE HARDWARE (sin WiFi, sin servidor)
  ------------------------------------------------
  Prueba el radar y el lector RFID al mismo tiempo, y dice con números qué está
  funcionando y qué no. Sirve para separar un problema de cables/energía de un
  problema del programa.

  Conexiones (las mismas del proyecto):
    Radar TX -> GPIO16   Radar RX -> GPIO17   Radar 5V -> 5V   GND -> GND
    RC522: SDA(SS)->GPIO5  SCK->GPIO18  MOSI->GPIO23  MISO->GPIO19  RST->GPIO22  3.3V->3V3  GND->GND

  Monitor Serie a 115200. Comandos:
    m  cambia el método de lectura de tarjetas (hay 3, se muestra cuál está activo)
    x  reinicia el lector RFID
    b  busca la velocidad (baudios) a la que habla el radar, y prueba con los cables cruzados
    h  muestra los bytes crudos que llegan del radar
    l  oculta o muestra las lineas del radar (util para la prueba de mover cables)
    g  cambia la sensibilidad de la antena del lector (minima / media / maxima)
*/

#include <SPI.h>
#include <MFRC522.h>
#include <esp_system.h>

constexpr int RADAR_RX_PIN = 16;
constexpr int RADAR_TX_PIN = 17;
constexpr int RFID_SS_PIN = 5, RFID_RST_PIN = 27, RFID_SCK_PIN = 18, RFID_MISO_PIN = 19, RFID_MOSI_PIN = 23;

HardwareSerial Radar(2);
MFRC522 lector(RFID_SS_PIN, RFID_RST_PIN);

// ---------------- Radar ----------------
const uint8_t CABECERA[4] = {0xAA, 0xFF, 0x03, 0x00};
uint8_t trama[30];
int posicion = 0;
unsigned long bytesRadar = 0, tramasRadar = 0;
int16_t ultimoX = 0, ultimoY = 0;
unsigned long ultimoObjetivoMs = 0;

int16_t decodificar(uint16_t crudo) {
  int16_t valor = crudo & 0x7FFF;
  return (crudo & 0x8000) ? valor : -valor;
}

// Recibe un byte; devuelve true cuando se completa una trama válida
bool alimentar(uint8_t b) {
  if (posicion < 4) {
    if (b == CABECERA[posicion]) trama[posicion++] = b;
    else if (b == CABECERA[0]) { trama[0] = b; posicion = 1; }
    else posicion = 0;
    return false;
  }
  trama[posicion++] = b;
  if (posicion < 30) return false;
  posicion = 0;
  return trama[28] == 0x55 && trama[29] == 0xCC;
}

void registrarTrama() {
  tramasRadar++;
  for (int t = 0; t < 3; t++) {
    int base = 4 + t * 8;
    int16_t x = decodificar(trama[base] | (trama[base + 1] << 8));
    int16_t y = decodificar(trama[base + 2] | (trama[base + 3] << 8));
    if (!(x == 0 && y == 0)) { ultimoX = x; ultimoY = y; ultimoObjetivoMs = millis(); break; }
  }
}

void leerRadar() {
  while (Radar.available()) {
    uint8_t b = Radar.read();
    bytesRadar++;
    if (alimentar(b)) registrarTrama();
  }
}

bool radarAbierto = false;
int pinRx = RADAR_RX_PIN;   // pines que se están usando ahora (se pueden intercambiar en la prueba)
int pinTx = RADAR_TX_PIN;

void abrirRadar(unsigned long baudios) {
  if (radarAbierto) Radar.end();
  radarAbierto = true;
  Radar.setRxBufferSize(2048);
  Radar.begin(baudios, SERIAL_8N1, pinRx, pinTx);
  posicion = 0;
}

// Prueba las velocidades que admite el LD2450 con los pines actuales; devuelve la que da tramas válidas (0 si ninguna)
unsigned long probarVelocidades() {
  const unsigned long candidatas[] = {256000, 115200, 460800, 230400, 57600, 38400, 19200, 9600};
  unsigned long mejor = 0;
  int mejorTramas = 0;
  for (unsigned long baudios : candidatas) {
    abrirRadar(baudios);
    unsigned long bytes = 0;
    int tramas = 0;
    unsigned long fin = millis() + 1200;
    while (millis() < fin) {
      while (Radar.available()) {
        uint8_t b = Radar.read();
        bytes++;
        if (alimentar(b)) tramas++;
      }
      delay(1);
    }
    Serial.printf("  %6lu baudios: %5lu bytes, %3d tramas validas\n", baudios, bytes, tramas);
    if (tramas > mejorTramas) { mejorTramas = tramas; mejor = baudios; }
  }
  return mejor;
}

// Muestra los primeros bytes que llegan, tal cual (un radar sano empieza cada trama con AA FF 03 00)
void muestraCruda(unsigned long baudios) {
  abrirRadar(baudios);
  Serial.printf("Bytes crudos a %lu baudios (un radar sano repite AA FF 03 00 ... 55 CC):\n", baudios);
  unsigned long fin = millis() + 2500;
  int n = 0;
  while (millis() < fin && n < 60) {
    while (Radar.available() && n < 60) {
      Serial.printf("%02X ", Radar.read());
      n++;
      if (n % 20 == 0) Serial.println();
    }
    delay(1);
  }
  Serial.println();
  if (n == 0) Serial.println("(no llego ningun byte)");
}

void escanearVelocidades() {
  Serial.printf("Buscando la velocidad del radar con los cables como estan (RX=GPIO%d, TX=GPIO%d)...\n",
                RADAR_RX_PIN, RADAR_TX_PIN);
  pinRx = RADAR_RX_PIN;
  pinTx = RADAR_TX_PIN;
  unsigned long baudios = probarVelocidades();
  if (baudios != 0) {
    Serial.printf(">>> El radar habla a %lu baudios. Se usara esa velocidad en esta prueba.\n", baudios);
    abrirRadar(baudios);
    return;
  }

  Serial.printf("Sin resultado. Probando con RX y TX intercambiados (RX=GPIO%d, TX=GPIO%d)...\n",
                RADAR_TX_PIN, RADAR_RX_PIN);
  pinRx = RADAR_TX_PIN;
  pinTx = RADAR_RX_PIN;
  baudios = probarVelocidades();
  if (baudios != 0) {
    Serial.printf(">>> El radar SI responde a %lu baudios, pero con los cables CRUZADOS.\n", baudios);
    Serial.printf(">>> Su cable TX esta en GPIO%d: muevelo a GPIO%d, y el cable RX del radar a GPIO%d.\n",
                  RADAR_TX_PIN, RADAR_RX_PIN, RADAR_TX_PIN);
    Serial.println(">>> (O deja los cables y cambia RADAR_RX_PIN y RADAR_TX_PIN en config.h.)");
    abrirRadar(baudios);
    return;
  }

  pinRx = RADAR_RX_PIN;
  pinTx = RADAR_TX_PIN;
  Serial.println(">>> Ninguna combinacion produjo tramas validas.");
  muestraCruda(256000);
  abrirRadar(256000);
}

// ---------------- RFID ----------------
// Método 0: estándar.  1: estándar + apagar/encender la antena tras leer.  2: "despertar" (WUPA).
int metodo = 0;
const char* NOMBRES[3] = {"0 estandar", "1 estandar + reinicio de antena", "2 despertar (WUPA)"};
unsigned long lecturas = 0;
char ultimoUid[24] = "";
unsigned long ultimaVistaMs = 0;
unsigned long ultimaRevisionMs = 0;

bool lectorResponde() {
  byte v = lector.PCD_ReadRegister(MFRC522::VersionReg);
  return !(v == 0x00 || v == 0xFF);
}

// Sensibilidad de la antena que se puede probar con el comando 'g'
const byte GANANCIAS[3] = {0x00, 0x40, 0x70};
const char* NOMBRES_GANANCIA[3] = {"minima (18 dB)", "media (33 dB)", "maxima (48 dB)"};
int gananciaIdx = 1;

bool mostrarRadar = true;              // comando 'l' para ocultar las líneas del radar
unsigned long reinicios = 0;          // veces que el lector se apagó solo y hubo que reiniciarlo
unsigned long fallosUid = 0;          // la tarjeta contestó pero no se pudo leer su UID
MFRC522::StatusCode ultimoFalloUid = MFRC522::STATUS_OK;

byte ultimoTx = 0x03;                 // último valor de TxControlReg que se vio con la antena apagada

void configurarLector() {
  lector.PCD_Init();
  lector.PCD_SetAntennaGain(GANANCIAS[gananciaIdx]);
}

// Si la antena se apagó (el chip se reinició), se vuelve a configurar
void vigilarLector() {
  byte tx = lector.PCD_ReadRegister(MFRC522::TxControlReg);
  if ((tx & 0x03) != 0x03) {
    reinicios++;
    ultimoTx = tx;
    configurarLector();
  }
}

// Cuenta qué responde el lector cada vez que busca una tarjeta
unsigned long busquedas = 0, respuestasOk = 0, colisiones = 0, sinRespuesta = 0, errores = 0;

bool hayTarjeta() {
  byte atqa[2];
  byte tam = sizeof(atqa);
  lector.PCD_WriteRegister(MFRC522::TxModeReg, 0x00);
  lector.PCD_WriteRegister(MFRC522::RxModeReg, 0x00);
  lector.PCD_WriteRegister(MFRC522::ModWidthReg, 0x26);
  MFRC522::StatusCode r = (metodo == 2) ? lector.PICC_WakeupA(atqa, &tam)
                                        : lector.PICC_RequestA(atqa, &tam);
  busquedas++;
  if (r == MFRC522::STATUS_OK) respuestasOk++;
  else if (r == MFRC522::STATUS_COLLISION) colisiones++;
  else if (r == MFRC522::STATUS_TIMEOUT) sinRespuesta++;
  else errores++;
  return r == MFRC522::STATUS_OK || r == MFRC522::STATUS_COLLISION;
}

void leerRfid() {
  if (millis() - ultimaRevisionMs < 100) return;
  ultimaRevisionMs = millis();
  vigilarLector();
  if (!hayTarjeta()) return;
  MFRC522::StatusCode resultado = lector.PICC_Select(&lector.uid, 0);
  if (resultado != MFRC522::STATUS_OK) {          // la tarjeta contestó, pero falló la lectura del UID
    fallosUid++;
    ultimoFalloUid = resultado;
    return;
  }

  char uid[24] = {0};
  for (byte i = 0; i < lector.uid.size && i < 10; i++) sprintf(uid + i * 2, "%02X", lector.uid.uidByte[i]);
  lector.PICC_HaltA();
  lector.PCD_StopCrypto1();
  if (metodo == 1) {
    lector.PCD_AntennaOff();
    delay(5);
    lector.PCD_AntennaOn();
  }

  bool nueva = strcmp(uid, ultimoUid) != 0 || (millis() - ultimaVistaMs > 800);
  strcpy(ultimoUid, uid);
  ultimaVistaMs = millis();
  if (nueva) {
    lecturas++;
    Serial.printf("*** TARJETA LEIDA #%lu  UID=%s  (metodo %s)\n", lecturas, uid, NOMBRES[metodo]);
  }
}

// ---------------- Informe ----------------
const char* motivoReinicio() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  return "encendido normal";
    case ESP_RST_SW:       return "reinicio por software";
    case ESP_RST_PANIC:    return "ERROR DEL PROGRAMA (panic)";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:      return "PROGRAMA TRABADO (watchdog)";
    case ESP_RST_BROWNOUT: return "FALTA DE ENERGIA (brownout)";
    case ESP_RST_EXT:      return "boton EN / reset externo";
    default:               return "otro";
  }
}

// Avisa en cuanto el lector se reinicia, para la "prueba de mover cables"
void avisarReinicios() {
  static unsigned long ultimoAvisoMs = 0, vistos = 0;
  if (millis() - ultimoAvisoMs < 400) return;
  ultimoAvisoMs = millis();
  if (reinicios != vistos) {
    Serial.printf("   !!! el lector se reinicio (%lu veces en total)\n", reinicios);
    vistos = reinicios;
  }
}

void informar() {
  static unsigned long t0 = 0, b0 = 0, f0 = 0;
  static int segundos = 0;
  if (millis() - t0 < 1000) return;
  t0 = millis();
  segundos++;

  if (mostrarRadar) {
    Serial.printf("[%3lus] radar: %lu bytes/s, %lu tramas/s", millis() / 1000, bytesRadar - b0, tramasRadar - f0);
    if (millis() - ultimoObjetivoMs < 1000 && ultimoObjetivoMs != 0)
      Serial.printf(" | PERSONA x=%d y=%d mm", ultimoX, ultimoY);
    else
      Serial.print(" | sin persona");
    Serial.printf(" | tarjetas: %lu\n", lecturas);
  }
  b0 = bytesRadar;
  f0 = tramasRadar;

  if (segundos % 5 == 0) {
    byte antena = lector.PCD_ReadRegister(MFRC522::TxControlReg) & 0x03;
    Serial.printf("      lector RFID: %s | antena: %s | ganancia: 0x%02X | metodo: %s\n",
                  lectorResponde() ? "responde" : "SIN RESPUESTA",
                  antena == 0x03 ? "encendida" : "APAGADA",
                  lector.PCD_GetAntennaGain(), NOMBRES[metodo]);
    Serial.printf("      busquedas de tarjeta: %lu | la tarjeta respondio: %lu | colision: %lu | sin respuesta: %lu | errores: %lu\n",
                  busquedas, respuestasOk, colisiones, sinRespuesta, errores);
    busquedas = respuestasOk = colisiones = sinRespuesta = errores = 0;
    Serial.printf("      lecturas de UID fallidas: %lu (ultimo motivo: %s) | reinicios del lector: %lu (TxControlReg al fallar: 0x%02X) | ganancia elegida: %s\n",
                  fallosUid, reinterpret_cast<const char*>(MFRC522::GetStatusCodeName(ultimoFalloUid)),
                  reinicios, ultimoTx, NOMBRES_GANANCIA[gananciaIdx]);
    fallosUid = 0;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.printf("Motivo del ultimo reinicio: %s\n", motivoReinicio());

  abrirRadar(256000);
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  configurarLector();
  Serial.printf("Lector RFID: %s\n", lectorResponde() ? "responde" : "SIN RESPUESTA (revisa cables y 3.3V)");
  Serial.println("Comandos: 'm' cambia el metodo de lectura de tarjetas, 'x' reinicia el lector, 'b' busca la velocidad del radar, 'h' muestra los bytes crudos del radar, 'g' cambia la sensibilidad del lector, 'l' oculta o muestra las lineas del radar.");
  escanearVelocidades();
  Serial.println("Esperado: el radar debe mostrar unas 10 tramas/s.");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'm' || c == 'M') {
      metodo = (metodo + 1) % 3;
      Serial.printf("Metodo de lectura de tarjetas: %s\n", NOMBRES[metodo]);
    } else if (c == 'b' || c == 'B') {
      escanearVelocidades();
    } else if (c == 'h' || c == 'H') {
      muestraCruda(256000);
    } else if (c == 'x' || c == 'X') {
      configurarLector();
      Serial.println("Lector RFID reiniciado.");
    } else if (c == 'l' || c == 'L') {
      mostrarRadar = !mostrarRadar;
      Serial.println(mostrarRadar ? "Lineas del radar: visibles." : "Lineas del radar: ocultas.");
    } else if (c == 'g' || c == 'G') {
      gananciaIdx = (gananciaIdx + 1) % 3;
      configurarLector();
      Serial.printf("Sensibilidad de la antena: %s\n", NOMBRES_GANANCIA[gananciaIdx]);
    }
  }
  leerRadar();
  leerRfid();
  informar();
  avisarReinicios();
}
