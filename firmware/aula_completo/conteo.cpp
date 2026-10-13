#include <Arduino.h>
#include "config.h"
#include "conteo.h"

namespace {

HardwareSerial Radar(2);

// Cada trama del radar mide 30 bytes: cabecera (4) + 3 objetivos de 8 bytes + cierre (2)
const uint8_t CABECERA[4] = {0xAA, 0xFF, 0x03, 0x00};
constexpr int LARGO_TRAMA = 30;

uint8_t trama[LARGO_TRAMA];
int posicion = 0;

// Zona de cada uno de los 3 objetivos: 0 = desconocida, 1 = lejos, 2 = cerca
int zona[3] = {0, 0, 0};
int framesSinVer[3] = {0, 0, 0};

volatile int totalPersonas = 0;
volatile bool cambioPendiente = true;   // true al inicio para enviar el primer dato

unsigned long totalBytes = 0;
unsigned long totalTramas = 0;

bool diagnostico = false;               // si es true, muestra lo que ve el radar
unsigned long ultimoDiagnostico = 0;

// El radar manda cada valor con el bit 15 como signo (1 = positivo)
int16_t decodificar(uint16_t crudo) {
  int16_t valor = crudo & 0x7FFF;
  return (crudo & 0x8000) ? valor : -valor;
}

void registrarCruce(int zonaAnterior, int zonaNueva) {
  bool entrada = (zonaAnterior == 1 && zonaNueva == 2);
  if (INVERTIR) entrada = !entrada;

  if (entrada) {
    totalPersonas = totalPersonas + 1;
    Serial.printf(">> ENTRADA  | Personas en el aula: %d\n", (int)totalPersonas);
  } else {
    if (totalPersonas > 0) totalPersonas = totalPersonas - 1;
    Serial.printf("<< SALIDA   | Personas en el aula: %d\n", (int)totalPersonas);
  }
  cambioPendiente = true;
}

void procesarTrama() {
  bool imprimir = diagnostico && (millis() - ultimoDiagnostico >= 250);
  bool huboObjetivo = false;

  for (int t = 0; t < 3; t++) {
    int base = 4 + t * 8;
    int16_t x = decodificar(trama[base]     | (trama[base + 1] << 8));
    int16_t y = decodificar(trama[base + 2] | (trama[base + 3] << 8));

    bool existe = !(x == 0 && y == 0);
    bool valido = existe && abs(x) <= X_MAX && y > 0 && y <= Y_MAX;

    int zonaActual = 0;   // 1 = lejos de la línea, 2 = cerca, 0 = zona muerta o fuera del área
    if (valido) {
      if (y > LINEA_Y + HISTERESIS)      zonaActual = 1;
      else if (y < LINEA_Y - HISTERESIS) zonaActual = 2;
    }

    if (imprimir && existe) {
      const char* donde = !valido ? "FUERA DEL AREA (se ignora)"
                        : zonaActual == 1 ? "lejos de la linea"
                        : zonaActual == 2 ? "cerca de la linea"
                        : "zona muerta";
      Serial.printf("[T%d] x=%5d mm  y=%5d mm  -> %s\n", t + 1, x, y, donde);
      huboObjetivo = true;
    }

    if (!valido) {
      if (++framesSinVer[t] > FRAMES_PERDIDO) zona[t] = 0;
      continue;
    }
    framesSinVer[t] = 0;
    if (zonaActual == 0) continue;               // en la zona muerta: no se decide nada

    if (zona[t] == 0) {
      zona[t] = zonaActual;                      // primera vez que lo vemos fuera de la zona muerta
    } else if (zonaActual != zona[t]) {
      registrarCruce(zona[t], zonaActual);
      zona[t] = zonaActual;
    }
  }

  if (imprimir) {
    ultimoDiagnostico = millis();
    if (!huboObjetivo) Serial.println("[radar] sin objetivos");
  }
}

// Recibe un byte; devuelve true cuando acaba de completarse una trama válida en 'trama'
bool alimentarParser(uint8_t b) {
  if (posicion < 4) {                          // buscando la cabecera
    if (b == CABECERA[posicion]) {
      trama[posicion++] = b;
    } else if (b == CABECERA[0]) {
      trama[0] = b;
      posicion = 1;
    } else {
      posicion = 0;
    }
    return false;
  }
  trama[posicion++] = b;                       // completando la trama
  if (posicion < LARGO_TRAMA) return false;
  posicion = 0;
  return trama[28] == 0x55 && trama[29] == 0xCC;
}

bool radarAbierto = false;

void abrirRadar(unsigned long baudios) {
  if (radarAbierto) Radar.end();
  radarAbierto = true;
  Radar.setRxBufferSize(2048);
  Radar.begin(baudios, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  posicion = 0;
}

// Escucha 'ms' milisegundos y cuenta cuántas tramas válidas llegan
int contarTramas(unsigned long ms) {
  int tramas = 0;
  unsigned long fin = millis() + ms;
  while (millis() < fin) {
    while (Radar.available()) {
      if (alimentarParser(Radar.read())) tramas++;
    }
    delay(1);
  }
  posicion = 0;
  return tramas;
}

// Prueba las velocidades que admite el LD2450 y devuelve la que produce tramas válidas (0 si ninguna)
unsigned long buscarVelocidad() {
  const unsigned long candidatas[] = {256000, 115200, 460800, 230400, 57600, 38400, 19200, 9600};
  for (unsigned long baudios : candidatas) {
    if (baudios == RADAR_BAUDIOS) continue;    // esa ya se probó
    abrirRadar(baudios);
    int n = contarTramas(1200);
    Serial.printf("  %lu baudios: %d tramas validas\n", baudios, n);
    if (n >= 3) return baudios;
  }
  return 0;
}

}  // namespace

namespace Conteo {

void iniciar() {
  abrirRadar(RADAR_BAUDIOS);
  if (!RADAR_BAUDIOS_AUTO) return;

  Serial.printf("Comprobando el radar a %lu baudios...\n", RADAR_BAUDIOS);
  if (contarTramas(1500) >= 3) {
    Serial.println("Radar OK.");
    return;
  }
  Serial.println("El radar no responde a esa velocidad. Buscando la correcta...");
  unsigned long encontrada = buscarVelocidad();
  if (encontrada != 0) {
    Serial.printf("Radar encontrado a %lu baudios. Para que arranque mas rapido, pon RADAR_BAUDIOS = %lu en config.h.\n",
                  encontrada, encontrada);
  } else {
    abrirRadar(RADAR_BAUDIOS);
    Serial.println("No llegan tramas validas a ninguna velocidad: revisa los cables TX/RX y los 5V del radar.");
  }
}

void actualizar() {
  while (Radar.available()) {
    uint8_t b = Radar.read();
    totalBytes++;

    if (alimentarParser(b)) {
      totalTramas++;
      procesarTrama();
    }
  }
}

unsigned long bytesRecibidos()  { return totalBytes; }
unsigned long tramasRecibidas() { return totalTramas; }

int  personas()              { return totalPersonas; }
bool hayCambioPendiente()    { return cambioPendiente; }
void limpiarCambioPendiente(){ cambioPendiente = false; }

void reiniciar() {
  totalPersonas = 0;
  cambioPendiente = true;
}

void alternarDiagnostico() {
  diagnostico = !diagnostico;
  if (diagnostico) {
    Serial.printf("Diagnostico ACTIVADO. Linea de conteo a %d mm (zona muerta de %d mm a cada lado).\n",
                  LINEA_Y, HISTERESIS);
    Serial.println("Para contar, la persona debe pasar de 'lejos' a 'cerca' (entrada) o al reves (salida).");
  } else {
    Serial.println("Diagnostico desactivado.");
  }
}

}  // namespace Conteo
