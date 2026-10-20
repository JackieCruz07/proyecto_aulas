#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "conteo.h"

namespace {

HardwareSerial Radar(2);

// Cada trama del radar mide 30 bytes: cabecera (4) + 3 objetivos de 8 bytes + cierre (2)
const uint8_t CABECERA[4] = {0xAA, 0xFF, 0x03, 0x00};
constexpr int LARGO_TRAMA = 30;

// Cola de eventos de cruce: la drena verificacion.cpp (radar + ToF fusionados)
constexpr int LARGO_COLA = 8;
struct Evento { int dir; unsigned long ms; };
Evento colaEventos[LARGO_COLA];
int colaCabeza = 0;
int colaCola = 0;

uint8_t trama[LARGO_TRAMA];
int posicion = 0;

// Estado de cada una de las 3 "ranuras" de objetivo del radar.
// zona: 0 = desconocida, 1 = lejos del radar (dentro del salón), 2 = cerca (en el umbral)
int zona[3] = {0, 0, 0};
int framesEnZona[3] = {0, 0, 0};        // tramas seguidas viéndolo en la zona candidata
int candidata[3] = {0, 0, 0};           // zona candidata a ser la nueva (evita cruces de un solo frame)
int framesSinVer[3] = {0, 0, 0};
bool conPosicion[3] = {false, false, false};
int16_t ultX[3] = {0, 0, 0}, ultY[3] = {0, 0, 0};
unsigned long ultimoCruceRanura[3] = {0, 0, 0};

int provisional = 0;                    // lo que vería el radar solo (diagnóstico)
unsigned long totalBytes = 0;
unsigned long totalTramas = 0;
unsigned long ultimoTramaMs = 0;

bool diagnostico = false;
unsigned long ultimoDiagnostico = 0;

// El radar manda cada valor con el bit 15 como signo (1 = positivo)
int16_t decodificar(uint16_t crudo) {
  int16_t valor = crudo & 0x7FFF;
  return (crudo & 0x8000) ? valor : -valor;
}

void encolar(int dir, unsigned long ms) {
  int siguiente = (colaCola + 1) % LARGO_COLA;
  if (siguiente == colaCabeza) {                    // cola llena: se descarta el evento más viejo
    colaCabeza = (colaCabeza + 1) % LARGO_COLA;
    Serial.println("!! El radar cruzo mas rapido de lo que se procesa: evento viejo descartado.");
  }
  colaEventos[colaCola] = {dir, ms};
  colaCola = siguiente;
}

void registrarCruce(int ranura, int zonaAnterior, int zonaNueva) {
  unsigned long ahora = millis();
  if (ahora - ultimoCruceRanura[ranura] < MIN_MS_CRUCE_RANURA) {
    // Rebote o cambio de ranura: una persona no cruza dos veces tan seguido
    return;
  }
  ultimoCruceRanura[ranura] = ahora;

  // Con el radar dentro del salón mirando hacia adentro:
  // quien ENTRA se aleja (cerca -> lejos), quien SALE se acerca (lejos -> cerca).
  bool entrada = (zonaAnterior == 2 && zonaNueva == 1);
  if (INVERTIR) entrada = !entrada;

  int dir = entrada ? 1 : -1;
  provisional += dir;
  if (provisional < 0) provisional = 0;
  encolar(dir, ahora);

  Serial.printf("%s [radar, ranura %d] | %s -> %s | y: %d mm | provisional radar: %d\n",
                entrada ? ">> CANDIDATO ENTRADA" : "<< CANDIDATO SALIDA", ranura + 1,
                zonaAnterior == 2 ? "cerca" : "lejos", zonaNueva == 2 ? "cerca" : "lejos",
                (int)ultY[ranura], provisional);
}

void olvidarRanura(int t) {
  zona[t] = 0; candidata[t] = 0; framesEnZona[t] = 0; conPosicion[t] = false;
  framesSinVer[t] = 0;
}

void procesarTrama() {
  bool imprimir = diagnostico && (millis() - ultimoDiagnostico >= 250);
  bool huboObjetivo = false;

  for (int t = 0; t < 3; t++) {
    int base = 4 + t * 8;
    int16_t x = decodificar(trama[base]     | (trama[base + 1] << 8));
    int16_t y = decodificar(trama[base + 2] | (trama[base + 3] << 8));

    // Puerta de entrada de datos: dentro del pasillo en ancho, dentro del rango
    // útil en distancia y dentro del cono de ±ANGULO_MAXIMO grados. El LD2450
    // tiene cono físico de ±60°; estrecharlo aquí evita seguir a gente de más.
    bool existe = !(x == 0 && y == 0);
    bool enCono = existe && y > 0 && abs(x) * 1000 <= y * (int)(tan(ANGULO_MAXIMO * DEG_TO_RAD) * 1000 + 0.5f);
    bool valido = existe && abs(x) <= X_MAX && y >= Y_MIN && y <= Y_MAX && enCono;

    if (imprimir && existe) {
      const char* donde = !enCono ? "FUERA DEL CONO (se ignora)"
                        : !valido ? "FUERA DEL PASILLO (se ignora)"
                        : y > LINEA_Y + HISTERESIS ? "lejos del radar (dentro del salon)"
                        : y < LINEA_Y - HISTERESIS ? "cerca del radar (en el umbral)"
                        : "zona muerta de la linea";
      Serial.printf("[T%d] x=%5d mm  y=%5d mm  -> %s\n", t + 1, x, y, donde);
      huboObjetivo = true;
    }

    if (!valido) {
      if (++framesSinVer[t] > FRAMES_PERDIDO) olvidarRanura(t);
      continue;
    }
    framesSinVer[t] = 0;

    // Un salto de más de MAX_SALTO_MM entre tramas no lo hace una persona
    // caminando: la ranura cambió de persona. Se vuelve a seguirla desde cero
    // para no inventar un cruce al saltar de una persona a otra.
    if (conPosicion[t] && (abs(x - ultX[t]) > MAX_SALTO_MM || abs(y - ultY[t]) > MAX_SALTO_MM)) {
      zona[t] = 0; candidata[t] = 0; framesEnZona[t] = 0;
    }
    ultX[t] = x; ultY[t] = y; conPosicion[t] = true;

    int zonaActual = 0;   // 1 = lejos, 2 = cerca, 0 = en la zona muerta de la línea
    if (y > LINEA_Y + HISTERESIS)      zonaActual = 1;
    else if (y < LINEA_Y - HISTERESIS) zonaActual = 2;

    if (zonaActual == 0) {               // en la zona muerta no se decide nada
      candidata[t] = 0; framesEnZona[t] = 0;
      continue;
    }

    if (zona[t] == 0) {
      zona[t] = zonaActual;              // primera vez que lo vemos fuera de la zona muerta
      continue;
    }
    if (zonaActual == zona[t]) {         // sigue en su zona: se estabiliza
      candidata[t] = 0; framesEnZona[t] = 0;
      continue;
    }

    // Zona distinta: se exige verla MIN_FRAMES_EN_ZONA tramas seguidas antes de
    // creer el cruce (un solo frame ruidoso no cuenta).
    if (candidata[t] != zonaActual) { candidata[t] = zonaActual; framesEnZona[t] = 1; }
    else if (++framesEnZona[t] >= MIN_FRAMES_EN_ZONA) {
      registrarCruce(t, zona[t], zonaActual);
      zona[t] = zonaActual;
      candidata[t] = 0; framesEnZona[t] = 0;
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
      ultimoTramaMs = millis();
      procesarTrama();
    }
  }
}

bool proximoEvento(int& dir, unsigned long& ms) {
  if (colaCabeza == colaCola) return false;
  dir = colaEventos[colaCabeza].dir;
  ms = colaEventos[colaCabeza].ms;
  colaCabeza = (colaCabeza + 1) % LARGO_COLA;
  return true;
}

int  personasSolo()          { return provisional; }
unsigned long bytesRecibidos()  { return totalBytes; }
unsigned long tramasRecibidas() { return totalTramas; }
unsigned long ultimoDatoMs()    { return ultimoTramaMs; }

void reiniciar() {
  provisional = 0;
  colaCabeza = colaCola = 0;
  for (int t = 0; t < 3; t++) {
    ultimoCruceRanura[t] = 0;
    olvidarRanura(t);
  }
}

void alternarDiagnostico() {
  diagnostico = !diagnostico;
  if (diagnostico) {
    Serial.printf("Diagnostico ACTIVADO. Pasillo: linea a %d mm (muerta +- %d mm), ancho +- %d mm, rango %d-%d mm, cono +-%d grados.\n",
                  LINEA_Y, HISTERESIS, X_MAX, Y_MIN, Y_MAX, ANGULO_MAXIMO);
    Serial.println("Con el radar dentro del salon: entrar es 'cerca -> lejos', salir es 'lejos -> cerca'.");
  } else {
    Serial.println("Diagnostico desactivado.");
  }
}

}  // namespace Conteo
