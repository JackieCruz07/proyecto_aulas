#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "config.h"
#include "lector_rfid.h"

namespace {

MFRC522 lector(RFID_SS_PIN, RFID_RST_PIN);

unsigned long ultimaRevision = 0;   // última vez que se buscó una tarjeta
unsigned long ultimaSalud = 0;      // última vez que se comprobó que el lector responde
char ultimoUid[24] = "";            // última tarjeta vista
unsigned long ultimaVista = 0;      // cuándo se vio por última vez

unsigned long ultimoAviso = 0;       // para no llenar el Monitor Serie de avisos repetidos

unsigned long reiniciosEnPeriodo = 0;   // reinicios del lector desde el último resumen
unsigned long ultimoResumen = 0;

void configurarLector() {
  lector.PCD_Init();
  lector.PCD_SetAntennaGain(MFRC522::RxGain_avg);   // sensibilidad media (33 dB)
}

// Si el chip se reinició (por un bajón de energía o un cable flojo) la antena queda apagada
// y no lee nada hasta reiniciar la placa. Aquí se detecta y se vuelve a configurar solo.
void vigilarLector() {
  if ((lector.PCD_ReadRegister(MFRC522::TxControlReg) & 0x03) != 0x03) {
    reiniciosEnPeriodo++;
    configurarLector();
  }
  // Un solo resumen cada 10 segundos para no llenar el Monitor Serie
  if (reiniciosEnPeriodo > 0 && millis() - ultimoResumen > 10000) {
    ultimoResumen = millis();
    Serial.printf("Lector RFID: se reinicio %lu veces en los ultimos segundos (si pasa seguido, revisa 3.3V, GND y RST).\n",
                  reiniciosEnPeriodo);
    reiniciosEnPeriodo = 0;
  }
}

bool lectorResponde() {
  byte version = lector.PCD_ReadRegister(MFRC522::VersionReg);
  return !(version == 0x00 || version == 0xFF);
}

// Igual que PICC_IsNewCardPresent(), pero con "WUPA": también despierta las tarjetas
// que ya leímos. Con PICC_IsNewCardPresent() una tarjeta ya leída no volvía a detectarse
// mientras siguiera dentro del campo del lector.
bool hayTarjetaEnElCampo() {
  byte atqa[2];
  byte tam = sizeof(atqa);
  lector.PCD_WriteRegister(MFRC522::TxModeReg, 0x00);
  lector.PCD_WriteRegister(MFRC522::RxModeReg, 0x00);
  lector.PCD_WriteRegister(MFRC522::ModWidthReg, 0x26);
  MFRC522::StatusCode resultado = lector.PICC_WakeupA(atqa, &tam);
  return resultado == MFRC522::STATUS_OK || resultado == MFRC522::STATUS_COLLISION;
}

}  // namespace

namespace LectorRfid {

void iniciar() {
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  configurarLector();

  if (lectorResponde()) {
    Serial.printf("Lector RFID listo (version 0x%02X)\n", lector.PCD_ReadRegister(MFRC522::VersionReg));
  } else {
    Serial.println("ERROR: no se detecta el lector RFID (revisa cables y 3.3V).");
  }
}

bool responde() { return lectorResponde(); }

bool leerTarjeta(char* uid, size_t tam) {
  unsigned long ahora = millis();
  if (ahora - ultimaRevision < 100) return false;   // se revisa 10 veces por segundo
  ultimaRevision = ahora;

  // Cada cierto tiempo se comprueba el lector y, si dejó de responder, se reinicia
  if (ahora - ultimaSalud > REVISION_LECTOR_MS) {
    ultimaSalud = ahora;
    if (!lectorResponde()) {
      Serial.println("El lector RFID no responde: reiniciandolo...");
      configurarLector();
    }
  }

  vigilarLector();
  if (!hayTarjetaEnElCampo()) return false;

  // La tarjeta contestó: se lee su UID (hasta 2 intentos) y, si falla, se dice por qué
  MFRC522::StatusCode resultado = lector.PICC_Select(&lector.uid, 0);
  if (resultado != MFRC522::STATUS_OK) resultado = lector.PICC_Select(&lector.uid, 0);
  if (resultado != MFRC522::STATUS_OK) {
    if (ahora - ultimoAviso > 2000) {
      ultimoAviso = ahora;
      Serial.printf("Tarjeta detectada pero no se pudo leer su UID (motivo: %s).\n",
                    reinterpret_cast<const char*>(MFRC522::GetStatusCodeName(resultado)));
    }
    return false;
  }

  char actual[24] = {0};
  size_t n = lector.uid.size;
  if (n > 10) n = 10;
  for (size_t i = 0; i < n; i++) sprintf(actual + i * 2, "%02X", lector.uid.uidByte[i]);

  lector.PICC_HaltA();
  lector.PCD_StopCrypto1();

  // Una tarjeta cuenta como "nueva" si es otra distinta, o si la misma se alejó
  // del lector el tiempo suficiente. Así, dejarla pegada no registra varias veces,
  // pero retirarla y volver a acercarla sí.
  bool esNueva = strcmp(actual, ultimoUid) != 0 || (ahora - ultimaVista > AUSENCIA_TARJETA_MS);
  strcpy(ultimoUid, actual);
  ultimaVista = ahora;
  if (!esNueva || tam < n * 2 + 1) return false;

  strcpy(uid, actual);
  return true;
}

}  // namespace LectorRfid
