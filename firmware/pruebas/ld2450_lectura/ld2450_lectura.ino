/*
  PRUEBA 1: leer el radar HLK-LD2450 con el ESP32
  ------------------------------------------------
  Conexiones:
    Radar 5V  -> 5V del ESP32 (pin VIN / 5V)
    Radar GND -> GND del ESP32
    Radar TX  -> GPIO16 del ESP32 (RX2)
    Radar RX  -> GPIO17 del ESP32 (TX2)

  Abre el Monitor Serie a 115200 baudios.
  El radar habla a 256000 baudios, 8N1 (fijo, lo define el fabricante).

  Formato de cada trama (30 bytes) según el protocolo del fabricante:
    4 bytes  cabecera: AA FF 03 00
    24 bytes 3 objetivos x 8 bytes (X, Y, velocidad, resolución)
    2 bytes  cierre:   55 CC
  X y Y vienen en milímetros, velocidad en cm/s.
  Cada valor de 16 bits usa el bit 15 como signo: 1 = positivo, 0 = negativo.
*/

#define RADAR_RX_PIN 16   // el ESP32 RECIBE por aquí (va al TX del radar)
#define RADAR_TX_PIN 17   // el ESP32 ENVÍA por aquí (va al RX del radar)

HardwareSerial Radar(2);  // usamos el puerto serie número 2 del ESP32

const uint8_t CABECERA[4] = {0xAA, 0xFF, 0x03, 0x00};
const int LARGO_TRAMA = 30;

uint8_t trama[LARGO_TRAMA];
int posicion = 0;
unsigned long ultimaImpresion = 0;

// Convierte el valor crudo del radar a un número con signo
int16_t decodificar(uint16_t crudo) {
  int16_t valor = crudo & 0x7FFF;
  return (crudo & 0x8000) ? valor : -valor;
}

void procesarTrama() {
  // Imprimimos máximo 5 veces por segundo para poder leer
  if (millis() - ultimaImpresion < 200) return;
  ultimaImpresion = millis();

  bool hayAlguien = false;
  for (int t = 0; t < 3; t++) {
    int base = 4 + t * 8;
    int16_t x = decodificar(trama[base]     | (trama[base + 1] << 8));
    int16_t y = decodificar(trama[base + 2] | (trama[base + 3] << 8));
    int16_t v = decodificar(trama[base + 4] | (trama[base + 5] << 8));

    // Si X y Y valen 0, ese "slot" está vacío (no hay objetivo)
    if (x == 0 && y == 0) continue;

    hayAlguien = true;
    Serial.printf("Objetivo %d -> X: %5d mm | Y: %5d mm | Velocidad: %4d cm/s\n",
                  t + 1, x, y, v);
  }
  if (!hayAlguien) Serial.println("Sin objetivos detectados");
}

void setup() {
  Serial.begin(115200);
  Radar.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  Serial.println("Radar LD2450 iniciado, esperando datos...");
}

void loop() {
  while (Radar.available()) {
    uint8_t b = Radar.read();

    if (posicion < 4) {
      // Buscando la cabecera AA FF 03 00
      if (b == CABECERA[posicion]) {
        trama[posicion++] = b;
      } else if (b == CABECERA[0]) {
        trama[0] = b;
        posicion = 1;
      } else {
        posicion = 0;
      }
    } else {
      trama[posicion++] = b;
      if (posicion == LARGO_TRAMA) {
        // Verificamos el cierre 55 CC
        if (trama[28] == 0x55 && trama[29] == 0xCC) procesarTrama();
        posicion = 0;
      }
    }
  }
}
