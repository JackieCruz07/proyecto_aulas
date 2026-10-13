/*
  AULA COMPLETA: radar (conteo) + RFID (docente) + envío al servidor

  Este archivo solo coordina; cada tarea vive en su propio módulo:
    config.h             -> todos los ajustes (WiFi, pines, umbrales)
    conteo.*             -> radar y conteo de entradas/salidas
    lector_rfid.*        -> lector de tarjetas
    cliente_servidor.*   -> WiFi y envío de datos al servidor

  Librería necesaria: MFRC522 (GithubCommunity).
  Antes de subirlo, edita config.h.
*/

#include "config.h"
#include "conteo.h"
#include "lector_rfid.h"
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
  Serial.printf("[estado %lus] radar: %lu tramas | lector RFID: %s | WiFi: %s | personas: %d | memoria libre: %u\n",
                millis() / 1000, Conteo::tramasRecibidas(),
                LectorRfid::responde() ? "ok" : "SIN RESPUESTA",
                ClienteServidor::hayWifi() ? "ok" : "SIN WIFI",
                Conteo::personas(), (unsigned)ESP.getFreeHeap());
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
    Serial.println("Contador reiniciado a 0");
  } else if (c == 'd' || c == 'D') {   // 'd' muestra u oculta lo que ve el radar
    Conteo::alternarDiagnostico();
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
  ClienteServidor::iniciar();
  Serial.println("Comandos del Monitor Serie: 'r' reinicia el conteo, 'd' muestra lo que ve el radar, 'e' oculta el informe de estado.");
}

void loop() {
  atenderComandosSerie();
  Conteo::actualizar();
  atenderTarjeta();
  apagarLedSiToca();
  informarEstado();
}
