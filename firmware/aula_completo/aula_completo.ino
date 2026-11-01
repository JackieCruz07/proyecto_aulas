/*
  AULA COMPLETA: radar + ToF fusionados (conteo) + RFID (docente) + envío al servidor

  Este archivo solo coordina; cada tarea vive en su propio módulo:
    config.h             -> todos los ajustes (WiFi, pines, umbrales, montaje)
    conteo.*             -> radar LD2450: sigue a quien cruza el pasillo y emite eventos
    tof.*                -> dos VL53L1X arriba del marco: emiten quién cruza y en qué sentido
    verificacion.*       -> FUSIÓN: decide el conteo oficial (entrada la cuenta el
                            ToF y la confirma el radar; salida la cuenta el radar
                            y la confirma el ToF; lo demás se descarta)
    lector_rfid.*        -> lector de tarjetas
    cliente_servidor.*   -> WiFi y envío de datos al servidor

  Librerías necesarias: MFRC522 (GithubCommunity) y VL53L1X (Pololu).
  Antes de subirlo, edita config.h.
*/

#include "config.h"
#include "conteo.h"
#include "lector_rfid.h"
#include "tof.h"
#include "verificacion.h"
#include "cliente_servidor.h"
#include <esp_system.h>

unsigned long ledApagarEn = 0;
unsigned long ultimoEstadoMs = 0;
bool mostrarEstado = true;

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

// Una línea cada 5 segundos para saber si todo sigue vivo. Con 'e' se oculta o se muestra.
void informarEstado() {
  if (!mostrarEstado || millis() - ultimoEstadoMs < 5000) return;
  ultimoEstadoMs = millis();
  Serial.printf("[estado %lus] radar: %lu tramas | lector RFID: %s | WiFi: %s | modo: %s | personas: %d | memoria libre: %u\n",
                millis() / 1000, Conteo::tramasRecibidas(),
                LectorRfid::responde() ? "ok" : "SIN RESPUESTA",
                ClienteServidor::hayWifi() ? "ok" : "SIN WIFI",
                Verificacion::modo(), Verificacion::personas(), (unsigned)ESP.getFreeHeap());
  if (Tof::disponible()) {
    Serial.printf("                ToF (su cuenta): %d | confirmados: %lu | descartados/sin confirmar: %lu (radar solo %lu, ToF solo %lu)\n",
                  Tof::personas(), Verificacion::confirmados(), Verificacion::sinConfirmar(),
                  Verificacion::radarSolo(), Verificacion::tofSolo());
  }
}

void atenderTarjeta() {
  char uid[24] = {0};
  if (!LectorRfid::leerTarjeta(uid, sizeof(uid))) return;

  digitalWrite(LED_PIN, HIGH);        // el LED se enciende al leer una tarjeta
  ledApagarEn = millis() + 400;

  if (!ClienteServidor::hayWifi()) {
    Serial.printf("Tarjeta %s leída, pero no hay WiFi: NO se registró.\n", uid);
    return;
  }
  Serial.printf("Tarjeta leída. UID: %s\n", uid);
  ClienteServidor::enviarTarjeta(uid);
}

void atenderComandosSerie() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == 'r' || c == 'R') {          // 'r' reinicia el conteo
    Conteo::reiniciar();
    Tof::reiniciar();
    Verificacion::reiniciar();
    Serial.println("Contadores reiniciados a 0 (radar, ToF y fusion)");
  } else if (c == 'd' || c == 'D') {   // 'd' muestra u oculta lo que ve el radar
    Conteo::alternarDiagnostico();
  } else if (c == 't' || c == 'T') {   // 't' muestra u oculta las distancias de los sensores ToF
    Tof::alternarDiagnostico();
  } else if (c == 'k' || c == 'K') {   // 'k' recalibra los ToF (con la puerta vacía)
    Tof::recalibrar();
  } else if (c == 'e' || c == 'E') {   // 'e' muestra u oculta el informe de estado
    mostrarEstado = !mostrarEstado;
    Serial.println(mostrarEstado ? "Informe de estado activado." : "Informe de estado oculto.");
  }
}

void apagarLedSiToca() {
  if (ledApagarEn != 0 && millis() > ledApagarEn) {
    digitalWrite(LED_PIN, LOW);
    ledApagarEn = 0;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("\nMotivo del ultimo reinicio: %s\n", motivoReinicio());
  pinMode(LED_PIN, OUTPUT);

  Conteo::iniciar();
  LectorRfid::iniciar();
  Tof::iniciar();
  ClienteServidor::iniciar();
  Serial.println("Comandos del Monitor Serie: 'r' reinicia el conteo, 'd' muestra lo que ve el radar, 't' muestra los ToF, 'k' recalibra los ToF, 'e' oculta el informe de estado.");
}

void loop() {
  atenderComandosSerie();
  Conteo::actualizar();
  Tof::actualizar();
  Verificacion::actualizar();
  atenderTarjeta();
  apagarLedSiToca();
  informarEstado();
}
