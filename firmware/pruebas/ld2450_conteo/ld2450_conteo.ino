/*
  PRUEBA 2: contar entradas y salidas con el radar HLK-LD2450
  -----------------------------------------------------------
  Misma conexión de la prueba 1 (radar TX -> GPIO16, radar RX -> GPIO17).

  Idea: se define una "línea imaginaria" a LINEA_Y milímetros frente al radar.
    - Si una persona pasa de LEJOS a CERCA del radar  -> ENTRADA (+1)
    - Si pasa de CERCA a LEJOS                         -> SALIDA  (-1)
  Si en tu instalación el sentido te queda al revés, cambia INVERTIR a true.

  Monitor Serie a 115200. Escribe la letra 'r' y Enter para reiniciar el contador.
*/

#define RADAR_RX_PIN 16
#define RADAR_TX_PIN 17

// ---- Ajustes (en milímetros) ----
#define LINEA_Y     1500   // distancia de la línea imaginaria al radar
#define HISTERESIS  150    // zona "muerta" alrededor de la línea (evita rebotes)
#define X_MAX       1000   // se ignora lo que esté más a los lados (±)
#define Y_MAX       4000   // se ignora lo que esté más lejos que esto
#define FRAMES_PERDIDO 8   // tramas sin ver a la persona antes de olvidarla (~0.8 s)
#define INVERTIR    false

HardwareSerial Radar(2);

const uint8_t CABECERA[4] = {0xAA, 0xFF, 0x03, 0x00};
const int LARGO_TRAMA = 30;

uint8_t trama[LARGO_TRAMA];
int posicion = 0;

// Estado por cada uno de los 3 "espacios" de objetivo del radar
// zona: 0 = desconocida, 1 = lejos, 2 = cerca
int zona[3] = {0, 0, 0};
int framesSinVer[3] = {0, 0, 0};

int personas = 0;

int16_t decodificar(uint16_t crudo) {
  int16_t valor = crudo & 0x7FFF;
  return (crudo & 0x8000) ? valor : -valor;
}

void registrarCruce(int zonaAnterior, int zonaNueva) {
  bool entrada = (zonaAnterior == 1 && zonaNueva == 2);
  if (INVERTIR) entrada = !entrada;

  if (entrada) {
    personas++;
    Serial.printf(">> ENTRADA  | Personas en el aula: %d\n", personas);
  } else {
    if (personas > 0) personas--;   // nunca negativo
    Serial.printf("<< SALIDA   | Personas en el aula: %d\n", personas);
  }
}

void procesarTrama() {
  for (int t = 0; t < 3; t++) {
    int base = 4 + t * 8;
    int16_t x = decodificar(trama[base]     | (trama[base + 1] << 8));
    int16_t y = decodificar(trama[base + 2] | (trama[base + 3] << 8));

    bool valido = !(x == 0 && y == 0) && abs(x) <= X_MAX && y > 0 && y <= Y_MAX;

    if (!valido) {
      // Esperamos unas tramas antes de olvidar, por si es un parpadeo del radar
      if (++framesSinVer[t] > FRAMES_PERDIDO) zona[t] = 0;
      continue;
    }
    framesSinVer[t] = 0;

    int zonaActual = 0;
    if (y > LINEA_Y + HISTERESIS)      zonaActual = 1;  // lejos
    else if (y < LINEA_Y - HISTERESIS) zonaActual = 2;  // cerca
    else continue;                                       // en la zona muerta

    if (zona[t] == 0) {
      zona[t] = zonaActual;                // primera vez que lo vemos
    } else if (zonaActual != zona[t]) {
      registrarCruce(zona[t], zonaActual);
      zona[t] = zonaActual;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Radar.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  Serial.println("Contador iniciado. Personas en el aula: 0");
}

void loop() {
  // Reinicio manual del contador
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r' || c == 'R') {
      personas = 0;
      Serial.println("Contador reiniciado a 0");
    }
  }

  while (Radar.available()) {
    uint8_t b = Radar.read();

    if (posicion < 4) {
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
        if (trama[28] == 0x55 && trama[29] == 0xCC) procesarTrama();
        posicion = 0;
      }
    }
  }
}
