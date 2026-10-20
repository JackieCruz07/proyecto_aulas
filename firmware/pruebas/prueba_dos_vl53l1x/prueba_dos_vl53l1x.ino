/*
  PRUEBA DE LOS DOS VL53L1X (solos, sin radar ni RFID)
  -----------------------------------------------------
  Comprueba que los dos sensores se encienden uno a uno, reciben cada uno su propia
  dirección I2C y miden por separado.

  Conexiones (los dos sensores comparten SDA, SCL, alimentación y GND):
    VL53L1X A y B:  VIN/VCC -> 3V3     GND -> GND
                    SDA -> GPIO21      SCL -> GPIO22
    Sensor A:       XSHUT -> GPIO25 MORADO
    Sensor B:       XSHUT -> GPIO26 NARANJA
    GPIO1 de ambos: sin conectar

  Librería necesaria: "VL53L1X" de Pololu (Administrar bibliotecas).
  Monitor Serie a 115200. Tapa un sensor con la mano y fíjate que cambie solo su lectura.
*/

#include <Wire.h>
#include <VL53L1X.h>

constexpr int SDA_PIN = 21, SCL_PIN = 22;
constexpr int XSHUT_PIN[2] = {25, 26};
constexpr uint8_t DIRECCION[2] = {0x30, 0x31};

VL53L1X sensor[2];

void setup() {
  Serial.begin(115200);
  delay(300);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  // Se apagan los dos y se encienden de uno en uno, dándole a cada uno su dirección
  for (int i = 0; i < 2; i++) {
    pinMode(XSHUT_PIN[i], OUTPUT);
    digitalWrite(XSHUT_PIN[i], LOW);
  }
  delay(10);
  for (int i = 0; i < 2; i++) {
    digitalWrite(XSHUT_PIN[i], HIGH);
    delay(10);
    sensor[i].setTimeout(500);
    if (!sensor[i].init()) {
      Serial.printf("ERROR: no se detecto el sensor %c. Revisa SDA, SCL, XSHUT (GPIO%d) y la alimentacion.\n",
                    'A' + i, XSHUT_PIN[i]);
      while (true) delay(1000);
    }
    sensor[i].setAddress(DIRECCION[i]);
    Serial.printf("Sensor %c listo en la direccion 0x%02X\n", 'A' + i, DIRECCION[i]);
  }
  for (int i = 0; i < 2; i++) {
    sensor[i].setDistanceMode(VL53L1X::Long);
    sensor[i].setMeasurementTimingBudget(50000);
    sensor[i].startContinuous(50);
  }
}

void loop() {
  static unsigned long ultimo = 0;
  static int mm[2] = {0, 0};
  for (int i = 0; i < 2; i++) {
    if (sensor[i].dataReady()) {
      uint16_t d = sensor[i].read(false);
      mm[i] = sensor[i].timeoutOccurred() ? 0 : d;
    }
  }
  if (millis() - ultimo >= 200) {
    ultimo = millis();
    Serial.printf("A: %4d mm (%5.1f cm)   |   B: %4d mm (%5.1f cm)\n", mm[0], mm[0] / 10.0, mm[1], mm[1] / 10.0);
  }
}
