#include <Arduino.h>
#include <Wire.h>
#include <VL53L1X.h>
#include "config.h"
#include "tof.h"

namespace {

constexpr int MUESTRAS_CALIBRACION = 15;
constexpr int DISTANCIA_MAX_VALIDA = 4000;

VL53L1X sensores[2];                  // 0 = A (lado de FUERA de la puerta), 1 = B (lado de DENTRO)
bool listo = false;

int base[2] = {0, 0};                 // distancia "libre" de cada sensor (puerta vacía, mide al umbral/piso)
int distancia[2] = {0, 0};            // última lectura válida (0 = sin lectura)
bool bloqueado[2] = {false, false};   // true si hay algo más cerca que la distancia libre
int seguidasBloqueado[2] = {0, 0};
int seguidasLibre[2] = {0, 0};

// Un cruce es: se activa un sensor, y dentro de poco se activa el otro.
enum Estado { LIBRE, UNO, ESPERAR_LIBRE };
Estado estado = LIBRE;
int primero = 0;                      // cuál sensor se activó primero
unsigned long tPrimero = 0;
unsigned long tLibreDesde = 0;
const char* NOMBRE_ESTADO[] = {"libre", "un sensor activo", "esperando que se libere"};

int contador = 0;

// Cola de eventos de cruce: la drena verificacion.cpp
constexpr int LARGO_COLA = 8;
struct Evento { int dir; unsigned long ms; };
Evento colaEventos[LARGO_COLA];
int colaCabeza = 0;
int colaCola = 0;

bool diagnostico = false;
unsigned long ultimoDiagnostico = 0;

VL53L1X::DistanceMode modoDistancia() {
  if (TOF_MODO == 0) return VL53L1X::Short;
  if (TOF_MODO == 1) return VL53L1X::Medium;
  return VL53L1X::Long;
}

bool lecturaValida(uint16_t mm) { return mm > 0 && mm < DISTANCIA_MAX_VALIDA; }

void encolar(int dir, unsigned long ms) {
  int siguiente = (colaCola + 1) % LARGO_COLA;
  if (siguiente == colaCabeza) {                    // cola llena: se descarta el evento más viejo
    colaCabeza = (colaCabeza + 1) % LARGO_COLA;
    Serial.println("!! Los ToF cruzaron mas rapido de lo que se procesa: evento viejo descartado.");
  }
  colaEventos[colaCola] = {dir, ms};
  colaCola = siguiente;
}

void registrarPaso(int dir) {
  if (TOF_INVERTIR) dir = -dir;
  contador += dir;
  if (contador < 0) contador = 0;
  encolar(dir, millis());
  if (dir > 0) Serial.printf(">> [ToF] CANDIDATO ENTRADA (A->B) | Personas segun el ToF: %d\n", contador);
  else         Serial.printf("<< [ToF] CANDIDATO SALIDA  (B->A) | Personas segun el ToF: %d\n", contador);
}

// Mide la distancia "libre" de cada sensor (mediana de varias lecturas). La puerta debe estar vacía.
// Arriba del marco apuntando al piso esa distancia es la altura del umbral (~2 m).
bool calibrar() {
  int muestras[2][MUESTRAS_CALIBRACION];
  int n[2] = {0, 0};
  unsigned long fin = millis() + 5000;
  while (millis() < fin && (n[0] < MUESTRAS_CALIBRACION || n[1] < MUESTRAS_CALIBRACION)) {
    for (int i = 0; i < 2; i++) {
      if (n[i] >= MUESTRAS_CALIBRACION || !sensores[i].dataReady()) continue;
      uint16_t mm = sensores[i].read(false);
      if (!sensores[i].timeoutOccurred() && lecturaValida(mm)) muestras[i][n[i]++] = mm;
    }
    delay(5);
  }
  bool correcto = true;
  for (int i = 0; i < 2; i++) {
    if (n[i] < 8) {
      Serial.printf("ToF: el sensor %c no da lecturas validas (revisa su orientacion y su cableado).\n", 'A' + i);
      return false;
    }
    for (int a = 1; a < n[i]; a++) {                 // orden simple para sacar la mediana
      int v = muestras[i][a], b = a - 1;
      while (b >= 0 && muestras[i][b] > v) { muestras[i][b + 1] = muestras[i][b]; b--; }
      muestras[i][b + 1] = v;
    }
    base[i] = muestras[i][n[i] / 2];
    if (base[i] < TOF_MIN_BASE_MM) {
      Serial.printf("ToF: el sensor %c mide solo %d mm (minimo %d): no parece estar arriba del marco apuntando al umbral, o algo lo tapa.\n",
                    'A' + i, base[i], TOF_MIN_BASE_MM);
      correcto = false;
    } else if (base[i] > TOF_MAX_BASE_MM) {
      Serial.printf("ToF: el sensor %c mide %d mm (maximo %d): apunta demasiado lejos, reajusta su angulo.\n",
                    'A' + i, base[i], TOF_MAX_BASE_MM);
      correcto = false;
    }
  }
  if (!correcto) {
    Serial.println("ToF: corrige la posicion/angulo de los sensores y pulsa 'k' para recalibrar con la puerta vacia.");
    return false;
  }
  Serial.printf("ToF listo. Distancia libre: A = %d mm, B = %d mm. Se cuenta cuando se acorta mas de %d mm.\n",
                base[0], base[1], TOF_MARGEN_MM);
  for (int i = 0; i < 2; i++) { bloqueado[i] = false; seguidasBloqueado[i] = seguidasLibre[i] = 0; }
  estado = LIBRE;
  return true;
}

void leerSensores() {
  for (int i = 0; i < 2; i++) {
    if (!sensores[i].dataReady()) continue;
    uint16_t mm = sensores[i].read(false);
    bool valida = !sensores[i].timeoutOccurred() && lecturaValida(mm);
    distancia[i] = valida ? mm : 0;
    bool hayAlgo = valida && mm < base[i] - TOF_MARGEN_MM;

    // Se piden 2 lecturas seguidas iguales para cambiar de estado (evita falsos por ruido)
    if (hayAlgo) { seguidasBloqueado[i]++; seguidasLibre[i] = 0; }
    else         { seguidasLibre[i]++; seguidasBloqueado[i] = 0; }
    if (seguidasBloqueado[i] >= 2) bloqueado[i] = true;
    if (seguidasLibre[i] >= 2)     bloqueado[i] = false;
  }
}

void actualizarPaso(unsigned long ahora) {
  bool a = bloqueado[0], b = bloqueado[1];
  switch (estado) {
    case LIBRE:
      if (a && b) {                       // los dos a la vez: no se puede saber la dirección
        estado = ESPERAR_LIBRE;
      } else if (a || b) {
        primero = a ? 0 : 1;
        tPrimero = ahora;
        estado = UNO;
      }
      break;

    case UNO: {
      bool otroActivo = (primero == 0) ? b : a;
      if (otroActivo) {
        registrarPaso(primero == 0 ? +1 : -1);   // A primero = entrada (entra por A y luego B), y al revés salida
        estado = ESPERAR_LIBRE;
      } else if (ahora - tPrimero > TOF_ENTRE_SENSORES_MS) {
        estado = ESPERAR_LIBRE;                  // se activó uno solo: no es un cruce
      }
      break;
    }

    case ESPERAR_LIBRE:
      if (!a && !b) {
        if (tLibreDesde == 0) tLibreDesde = ahora;
        if (ahora - tLibreDesde >= TOF_LIBERAR_MS) { estado = LIBRE; tLibreDesde = 0; }
      } else {
        tLibreDesde = 0;
      }
      break;
  }
}

}  // namespace

namespace Tof {

void iniciar() {
  if (!TOF_ACTIVO) {
    Serial.println("ToF desactivado en config.h.");
    return;
  }
  Wire.begin(TOF_SDA_PIN, TOF_SCL_PIN);
  Wire.setClock(400000);

  // Los dos sensores nacen con la misma dirección. Se apagan con XSHUT y se encienden de uno
  // en uno: cada uno recibe su dirección antes de encender el siguiente.
  const int xshut[2] = {TOF_XSHUT_A_PIN, TOF_XSHUT_B_PIN};
  const uint8_t direccion[2] = {TOF_DIRECCION_A, TOF_DIRECCION_B};
  for (int i = 0; i < 2; i++) {
    pinMode(xshut[i], OUTPUT);
    digitalWrite(xshut[i], LOW);
  }
  delay(10);

  for (int i = 0; i < 2; i++) {
    digitalWrite(xshut[i], HIGH);
    delay(10);
    sensores[i].setTimeout(500);
    if (!sensores[i].init()) {
      Serial.printf("ToF: no se detecto el sensor %c. Revisa SDA, SCL, XSHUT y la alimentacion.\n", 'A' + i);
      return;
    }
    sensores[i].setAddress(direccion[i]);
  }
  for (int i = 0; i < 2; i++) {
    sensores[i].setDistanceMode(modoDistancia());
    sensores[i].setMeasurementTimingBudget(50000);
    sensores[i].startContinuous(50);
  }
  listo = calibrar();
}

void actualizar() {
  if (!listo) return;
  leerSensores();
  actualizarPaso(millis());

  if (diagnostico && millis() - ultimoDiagnostico >= 250) {
    ultimoDiagnostico = millis();
    Serial.printf("[ToF] A=%4d mm (libre %4d) %-8s | B=%4d mm (libre %4d) %-8s | %s | cuenta ToF=%d\n",
                  distancia[0], base[0], bloqueado[0] ? "ACTIVO" : "libre",
                  distancia[1], base[1], bloqueado[1] ? "ACTIVO" : "libre",
                  NOMBRE_ESTADO[estado], contador);
  }
}

bool disponible() { return listo; }

bool proximoEvento(int& dir, unsigned long& ms) {
  if (colaCabeza == colaCola) return false;
  dir = colaEventos[colaCabeza].dir;
  ms = colaEventos[colaCabeza].ms;
  colaCabeza = (colaCabeza + 1) % LARGO_COLA;
  return true;
}

int  personas()   { return contador; }
void reiniciar()  { contador = 0; colaCabeza = colaCola = 0; }

void recalibrar() {
  if (!listo) {
    // Puede que la calibración inicial fallara por una posición mal apuntada:
    // se reintenta (los sensores ya están iniciados).
    Serial.println("Reintentando la calibracion del ToF: la puerta debe estar vacia...");
    listo = calibrar();
    return;
  }
  Serial.println("Recalibrando el ToF: la puerta debe estar vacia...");
  listo = calibrar();
}

void alternarDiagnostico() {
  diagnostico = !diagnostico;
  Serial.println(diagnostico ? "Diagnostico del ToF activado." : "Diagnostico del ToF desactivado.");
}

}  // namespace Tof
