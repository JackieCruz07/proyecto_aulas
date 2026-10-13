/*
  PRUEBA 3: contador de entradas/salidas + pantalla de radar visual
  -----------------------------------------------------------------
  Misma conexión de antes (radar TX -> GPIO16, radar RX -> GPIO17).

  El ESP32 crea su propia red WiFi (no necesita internet ni WiFi de la escuela):
    Red:        Radar-Aula
    Contraseña: 12345678
    Página:     http://192.168.4.1   (ábrela en el navegador del celular o laptop)

  En la página verás el radar con los puntos de cada persona, la línea de conteo
  y el contador de personas. También puedes seguir usando el Monitor Serie (115200).
*/

#include <WiFi.h>
#include <WebServer.h>

#define RADAR_RX_PIN 16
#define RADAR_TX_PIN 17

// ---- Ajustes (milímetros) ----
#define LINEA_Y     1500
#define HISTERESIS  150
#define X_MAX       1000
#define Y_MAX       4000
#define FRAMES_PERDIDO 8
#define INVERTIR    false

const char* WIFI_NOMBRE = "Radar-Aula";
const char* WIFI_CLAVE  = "12345678";   // mínimo 8 caracteres

HardwareSerial Radar(2);
WebServer servidor(80);

const uint8_t CABECERA[4] = {0xAA, 0xFF, 0x03, 0x00};
const int LARGO_TRAMA = 30;
uint8_t trama[LARGO_TRAMA];
int posicion = 0;

int zona[3] = {0, 0, 0};          // 0 = desconocida, 1 = lejos, 2 = cerca
int framesSinVer[3] = {0, 0, 0};
int personas = 0;

// Últimos datos para dibujar
int16_t objX[3], objY[3], objV[3];
bool objActivo[3] = {false, false, false};
bool objValido[3] = {false, false, false};

// ---------- Página web ----------
const char PAGINA[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Radar del aula</title>
<style>
  body{font-family:sans-serif;background:#111;color:#eee;text-align:center;margin:0;padding:10px}
  #n{font-size:64px;font-weight:bold;color:#4ade80;margin:4px 0}
  canvas{background:#0b1d12;border:1px solid #2d5a3d;max-width:100%}
  button{font-size:16px;padding:8px 16px;margin-top:8px}
  small{color:#9ca3af}
</style></head><body>
<div>Personas en el aula</div>
<div id="n">0</div>
<canvas id="c" width="600" height="400"></canvas><br>
<button onclick="fetch('/reset')">Reiniciar contador</button><br>
<small>Punto verde: cuenta para el conteo. Punto gris: fuera del área de conteo.<br>
Radar abajo al centro; el frente del sensor apunta hacia arriba.</small>
<script>
const c=document.getElementById('c'), g=c.getContext('2d');
const W=c.width, H=c.height, MAXX=3000, MAXY=4000;
const k=W/(2*MAXX);               // píxeles por milímetro
const colores=['#4ade80','#38bdf8','#facc15'];
function base(l,h){
  g.clearRect(0,0,W,H);
  g.strokeStyle='#2d5a3d'; g.fillStyle='#6b8f7a'; g.lineWidth=1; g.font='12px sans-serif';
  for(let r=1000;r<=4000;r+=1000){
    g.beginPath(); g.arc(W/2,H,r*k,Math.PI,2*Math.PI); g.stroke();
    g.fillText((r/1000)+' m', W/2+4, H-r*k+12);
  }
  // abanico de +-60 grados
  g.beginPath(); g.moveTo(W/2,H);
  g.lineTo(W/2-Math.sin(Math.PI/3)*MAXY*k, H-Math.cos(Math.PI/3)*MAXY*k); g.stroke();
  g.beginPath(); g.moveTo(W/2,H);
  g.lineTo(W/2+Math.sin(Math.PI/3)*MAXY*k, H-Math.cos(Math.PI/3)*MAXY*k); g.stroke();
  // línea de conteo y zona muerta
  g.fillStyle='rgba(250,204,21,0.12)';
  g.fillRect(0,H-(l+h)*k,W,2*h*k);
  g.strokeStyle='#facc15'; g.beginPath(); g.moveTo(0,H-l*k); g.lineTo(W,H-l*k); g.stroke();
}
async function actualizar(){
  try{
    const d=await (await fetch('/data')).json();
    document.getElementById('n').textContent=d.p;
    base(d.l,d.h);
    d.t.forEach((o,i)=>{
      const px=W/2+o[0]*k, py=H-o[1]*k;
      g.fillStyle=o[3]?colores[i]:'#6b7280';
      g.beginPath(); g.arc(px,py,9,0,2*Math.PI); g.fill();
      g.fillStyle='#fff'; g.fillText('#'+(i+1)+' '+(o[1]/1000).toFixed(2)+' m',px+12,py+4);
    });
  }catch(e){}
  setTimeout(actualizar,100);
}
actualizar();
</script></body></html>
)rawliteral";

void enviarPagina() { servidor.send_P(200, "text/html", PAGINA); }

void enviarDatos() {
  String s = "{\"p\":" + String(personas) + ",\"l\":" + String(LINEA_Y) +
             ",\"h\":" + String(HISTERESIS) + ",\"t\":[";
  bool primero = true;
  for (int i = 0; i < 3; i++) {
    if (!objActivo[i]) continue;
    if (!primero) s += ",";
    primero = false;
    s += "[" + String(objX[i]) + "," + String(objY[i]) + "," + String(objV[i]) +
         "," + String(objValido[i] ? 1 : 0) + "]";
  }
  s += "]}";
  servidor.send(200, "application/json", s);
}

void reiniciarContador() {
  personas = 0;
  servidor.send(200, "text/plain", "ok");
}

// ---------- Radar ----------
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
    if (personas > 0) personas--;
    Serial.printf("<< SALIDA   | Personas en el aula: %d\n", personas);
  }
}

void procesarTrama() {
  for (int t = 0; t < 3; t++) {
    int base = 4 + t * 8;
    int16_t x = decodificar(trama[base]     | (trama[base + 1] << 8));
    int16_t y = decodificar(trama[base + 2] | (trama[base + 3] << 8));
    int16_t v = decodificar(trama[base + 4] | (trama[base + 5] << 8));

    bool existe = !(x == 0 && y == 0);
    bool valido = existe && abs(x) <= X_MAX && y > 0 && y <= Y_MAX;

    objActivo[t] = existe;
    objValido[t] = valido;
    objX[t] = x; objY[t] = y; objV[t] = v;

    if (!valido) {
      if (++framesSinVer[t] > FRAMES_PERDIDO) zona[t] = 0;
      continue;
    }
    framesSinVer[t] = 0;

    int zonaActual = 0;
    if (y > LINEA_Y + HISTERESIS)      zonaActual = 1;
    else if (y < LINEA_Y - HISTERESIS) zonaActual = 2;
    else continue;

    if (zona[t] == 0) {
      zona[t] = zonaActual;
    } else if (zonaActual != zona[t]) {
      registrarCruce(zona[t], zonaActual);
      zona[t] = zonaActual;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Radar.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);

  WiFi.softAP(WIFI_NOMBRE, WIFI_CLAVE);
  servidor.on("/", enviarPagina);
  servidor.on("/data", enviarDatos);
  servidor.on("/reset", reiniciarContador);
  servidor.begin();

  Serial.print("Red WiFi: "); Serial.println(WIFI_NOMBRE);
  Serial.print("Abre en el navegador: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  servidor.handleClient();

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r' || c == 'R') { personas = 0; Serial.println("Contador reiniciado a 0"); }
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
