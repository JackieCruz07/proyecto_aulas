/*
  PRUEBA 5: leer tarjetas con el lector RFID RC522
  ------------------------------------------------
  Conexiones (el RC522 funciona SOLO a 3.3V, nunca lo conectes a 5V):

    RC522 SDA (SS)  -> GPIO5   (D5)
    RC522 SCK       -> GPIO18  (D18)
    RC522 MOSI      -> GPIO23  (D23)
    RC522 MISO      -> GPIO19  (D19)
    RC522 RST       -> GPIO27  (D27)
    RC522 3.3V      -> 3V3
    RC522 GND       -> GND
    RC522 IRQ       -> sin conectar

  Librería necesaria: "MFRC522" de GithubCommunity
  (Arduino IDE -> Herramientas -> Administrar bibliotecas -> buscar "MFRC522").

  Abre el Monitor Serie a 115200 y acerca una tarjeta o llavero al lector.
*/

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN   5
#define RST_PIN  27
#define SCK_PIN  18
#define MISO_PIN 19
#define MOSI_PIN 23

MFRC522 lector(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  lector.PCD_Init();
  delay(100);

  // Prueba de comunicación con el chip del lector
  byte version = lector.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.printf("Version del chip RC522: 0x%02X\n", version);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("ERROR: no hay comunicacion con el lector. Revisa los cables y la alimentacion de 3.3V.");
  } else {
    Serial.println("Lector detectado. Acerca una tarjeta...");
  }
}

void loop() {
  // Si no hay tarjeta nueva, no hacemos nada
  if (!lector.PICC_IsNewCardPresent() || !lector.PICC_ReadCardSerial()) {
    return;
  }

  // Armamos el UID como texto: letras mayúsculas, sin espacios (ej. A1B2C3D4)
  String uid = "";
  for (byte i = 0; i < lector.uid.size; i++) {
    if (lector.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(lector.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.print("UID de la tarjeta: ");
  Serial.println(uid);
  Serial.print("Tipo: ");
  Serial.println(lector.PICC_GetTypeName(lector.PICC_GetType(lector.uid.sak)));
  Serial.println("---");

  // Le decimos a la tarjeta que ya la leímos (no se repite hasta retirarla y volver a acercarla)
  lector.PICC_HaltA();
  lector.PCD_StopCrypto1();
}
