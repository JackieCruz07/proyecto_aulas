#include <Arduino.h>
#include "config.h"
#include "conteo.h"
#include "tof.h"
#include "verificacion.h"

namespace {

// Fusión radar + ToF. Por dirección, un sensor propone y el otro confirma:
//   ENTRADA: propone el ToF (A->B en la puerta), confirma el radar (cerca->lejos).
//   SALIDA:  propone el radar (lejos->cerca), confirma el ToF (B->A en la puerta).
// Un cruce sin su contraparte dentro de la ventana se descarta: así no cuentan
// los que solo se asomaron a la puerta ni los que se acercaron al radar y regresaron.

constexpr int MAX_PEND = 6;          // cruces esperando confirmación por dirección

struct Cruce { bool activo; unsigned long ms; };
Cruce propE[MAX_PEND] = {};          // ENTRADAS propuestas por el ToF
Cruce confE[MAX_PEND] = {};          // ENTRADAS vistas por el radar (confirman al ToF)
Cruce propS[MAX_PEND] = {};          // SALIDAS propuestas por el radar
Cruce confS[MAX_PEND] = {};          // SALIDAS vistas por el ToF (confirman al radar)

int contador = 0;
unsigned long nConfirmados = 0;
unsigned long nRadarSolo = 0;
unsigned long nTofSolo = 0;
bool cambioPendiente = true;         // true al inicio para enviar el primer dato

enum Modo { FUSION, SOLO_TOF, SOLO_RADAR };
Modo modoActual = FUSION;

// true si 'msEvento' está dentro de la ventana respecto de 'ahora';
// 'adelantoOk' permite que el evento haya llegado un poco antes (rezagos de medición)
bool enVentana(unsigned long msEvento, unsigned long ahora, bool adelantoOk) {
  long d = (long)(ahora - msEvento);        // >0 = hace rato, <0 = llegó "antes"
  if (d >= 0) return (unsigned long)d <= CONFIRMACION_ESPERA_MS;
  return adelantoOk && (unsigned long)(-d) <= CONFIRMACION_ADELANTO_MS;
}

// Consume hasta MAX_GRUPO cruces activos de la lista que estén en la ventana
int consumir(Cruce* lista, unsigned long ms, unsigned long ahora) {
  int k = 0;
  for (int i = 0; i < MAX_PEND && k < MAX_GRUPO; i++) {
    if (!lista[i].activo || !enVentana(lista[i].ms, ahora, true)) continue;
    lista[i].activo = false;
    k++;
  }
  return k;
}

void guardar(Cruce* lista, unsigned long ms) {
  for (int i = 0; i < MAX_PEND; i++) {
    if (!lista[i].activo) { lista[i] = {true, ms}; return; }
  }
  for (int i = 1; i < MAX_PEND; i++) lista[i - 1] = lista[i];   // lista llena: se descarta el más viejo
  lista[MAX_PEND - 1] = {true, ms};
}

void confirmar(int dir, int k, const char* quien) {
  contador += (dir > 0) ? k : -k;
  if (contador < 0) contador = 0;
  nConfirmados++;
  cambioPendiente = true;
  Serial.printf("OK %s confirmada por %s (%d pers.) | Personas en el aula: %d\n",
                dir > 0 ? "ENTRADA" : "SALIDA", quien, k, contador);
}

void aplicarDirecto(int dir, const char* fuente) {
  contador += dir;
  if (contador < 0) contador = 0;
  nConfirmados++;
  cambioPendiente = true;
  Serial.printf("%s [%s, modo solo %s] | Personas en el aula: %d\n",
                dir > 0 ? ">> ENTRADA " : "<< SALIDA  ", fuente, fuente, contador);
}

// Caducan los cruces que nunca fueron confirmados: se descartan y se anotan
void caducar(Cruce* lista, unsigned long ahora, unsigned long& contadorSolo, const char* motivo) {
  for (int i = 0; i < MAX_PEND; i++) {
    if (!lista[i].activo || enVentana(lista[i].ms, ahora, false)) continue;
    lista[i].activo = false;
    contadorSolo++;
    Serial.printf("?? %s\n", motivo);
    cambioPendiente = true;
  }
}

void vaciar(Cruce* lista) { for (int i = 0; i < MAX_PEND; i++) lista[i].activo = false; }

const char* nombreModo(Modo m) {
  return m == FUSION ? "radar+tof" : m == SOLO_TOF ? "solo tof" : "solo radar";
}

}  // namespace

namespace Verificacion {

void actualizar() {
  unsigned long ahora = millis();

  // ---- Modo según qué sensores estén vivos ----
  unsigned long ultimaTrama = Conteo::ultimoDatoMs();
  bool radarOk = (ultimaTrama != 0) ? (ahora - ultimaTrama < RADAR_SIN_DATOS_MS)
                                    : (ahora < RADAR_SIN_DATOS_MS);   // arranque: aún buscando velocidad
  Modo nuevo = !Tof::disponible() ? SOLO_RADAR : (radarOk ? FUSION : SOLO_TOF);

  if (nuevo != modoActual) {
    // Al cambiar de modo se vacían las esperas: ya no hay dos sensores que se confirmen
    vaciar(propE); vaciar(confE); vaciar(propS); vaciar(confS);
    modoActual = nuevo;
    cambioPendiente = true;
    if (nuevo == SOLO_TOF)
      Serial.println("!! RADAR SIN DATOS: cuenta solo el ToF hasta que vuelva el radar (revisa su alimentacion).");
    else if (nuevo == SOLO_RADAR)
      Serial.println("!! ToF NO DISPONIBLE: cuenta solo el radar (sin confirmacion de cruce real por la puerta).");
    else
      Serial.println("Sensores completos: vuelve la fusion radar+ToF.");
  }

  // ---- Eventos del radar ----
  int dir; unsigned long ms;
  while (Conteo::proximoEvento(dir, ms)) {
    if (modoActual == SOLO_RADAR) { aplicarDirecto(dir, "radar"); continue; }
    if (modoActual == SOLO_TOF)   continue;                     // radar caído: sus eventos no cuentan
    // FUSION: la ENTRADA la propone el ToF (el radar la confirma) y la SALIDA
    // la propone el radar (el ToF la confirma). Cualquiera de los dos puede
    // llegar primero por los rezagos de medición, por eso también consume
    // confirmaciones/proposiciones que hayan llegado antes.
    int k = consumir(dir > 0 ? propE : confS, ms, ahora);
    if (k > 0)            confirmar(dir, k, "el radar");
    else if (dir > 0)     guardar(confE, ms);                   // entrada vista solo por el radar: quizás alguien del salon
    else                  guardar(propS, ms);                   // salida propuesta: espera el cruce del ToF
  }

  // ---- Eventos del ToF ----
  while (Tof::proximoEvento(dir, ms)) {
    if (modoActual == SOLO_TOF)   { aplicarDirecto(dir, "tof"); continue; }
    if (modoActual == SOLO_RADAR) continue;                     // ToF no disponible: sus eventos no cuentan
    // El ToF confirma entradas que el radar haya visto antes (adelanto) y
    // confirma salidas propuestas por el radar; si no hay nada que consumir,
    // guarda su propio evento para que el otro sensor lo consuma después.
    int k = consumir(dir > 0 ? confE : propS, ms, ahora);
    if (k > 0)            confirmar(dir, k, "el ToF");
    else if (dir > 0)     guardar(propE, ms);                   // entrada propuesta: espera que el radar la vea
    else                  guardar(confS, ms);                   // salida vista solo por el ToF: quizás alguien se asomó
  }

  // ---- Lo que nunca se confirmó se descarta ----
  if (modoActual == FUSION) {
    caducar(propE, ahora, nTofSolo,
            "Descartada ENTRADA: el ToF vio el cruce pero el radar no vio a la persona pasar al salon (se asomo o se quedo en el umbral).");
    caducar(confE, ahora, nRadarSolo,
            "Descartada entrada: el radar vio a alguien alejarse de la puerta pero el ToF no vio el cruce (alguien del salon).");
    caducar(propS, ahora, nRadarSolo,
            "Descartada SALIDA: el radar vio a alguien acercarse a la puerta pero el ToF no vio el cruce (se acerco y se regreso).");
    caducar(confS, ahora, nTofSolo,
            "Descartada salida: el ToF vio el cruce pero el radar no vio a nadie cerca de la puerta (se asomo).");
  }
}

void reiniciar() {
  vaciar(propE); vaciar(confE); vaciar(propS); vaciar(confS);
  contador = 0;
  nConfirmados = nRadarSolo = nTofSolo = 0;
  cambioPendiente = true;
}

int  personas()          { return contador; }
unsigned long confirmados()  { return nConfirmados; }
unsigned long radarSolo()    { return nRadarSolo; }
unsigned long tofSolo()      { return nTofSolo; }
unsigned long sinConfirmar() { return nRadarSolo + nTofSolo; }
const char* modo()           { return nombreModo(modoActual); }
bool hayCambioPendiente()    { return cambioPendiente; }
void limpiarCambioPendiente(){ cambioPendiente = false; }

}  // namespace Verificacion
