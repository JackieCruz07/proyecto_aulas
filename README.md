# Gestión de aulas con ESP32, radar LD2450 y RFID

Cada aula tiene un ESP32 que cuenta las personas con un radar más dos sensores ToF en la puerta
(la fusión de los dos decide el conteo oficial) y registra al docente con una tarjeta RFID.
Un servidor en Python guarda todo en una base de datos y muestra un panel web para gestionar las aulas.

```
ESP32 de cada aula  ──WiFi──►  Servidor Flask + SQLite  ◄──  Panel web
(radar + ToF + RFID)               (carpeta app/)            (cualquier navegador)
```

## Estructura del proyecto

```
proyecto_aulas/
├── run.py                  Punto de entrada del servidor
├── config.py               Claves, ruta de la base de datos y tiempos
├── requirements.txt        Dependencias (pip install -r requirements.txt)
│
├── app/                    SERVIDOR
│   ├── __init__.py         Fábrica de la aplicación (une todas las piezas)
│   ├── auth.py             Contraseña del panel y clave de API de los ESP32
│   ├── db.py               Conexión a SQLite
│   ├── esquema.sql         Tablas de la base de datos
│   ├── errores.py          Errores de la API convertidos a JSON
│   ├── utilidades.py       Funciones pequeñas compartidas
│   ├── rutas/              Capa HTTP: reciben la petición y responden (sin lógica)
│   │   ├── dispositivos.py   API de los ESP32 (conteo, tarjeta)
│   │   ├── consultas.py      API de lectura del panel
│   │   ├── gestion.py        Altas, ediciones y bajas
│   │   └── paginas.py        Páginas HTML
│   ├── servicios/          Capa de lógica: reglas de negocio (entrada/salida, validaciones)
│   │   ├── dispositivos.py
│   │   ├── panel.py
│   │   └── gestion.py
│   ├── repositorios/       Capa de datos: todo el SQL
│   │   ├── aulas.py · conteos.py · docentes.py · registros.py
│   ├── templates/          DISEÑO (HTML)
│   │   ├── base.html · login.html · panel.html
│   │   └── partials/       Una parte por pestaña y los diálogos
│   └── static/
│       ├── css/            DISEÑO (estilos): variables, base, layout, componentes, tablero…
│       └── js/             COMPORTAMIENTO del panel
│           ├── principal.js    Pestañas y actualización automática
│           ├── api.js · datos.js · ui.js · utilidades.js · acciones.js
│           └── vistas/         Un archivo por pestaña
│
├── tests/                  Pruebas automáticas del servidor
└── firmware/               ESP32
    ├── aula_completo/      Programa final (config.h, conteo, tof, verificacion, lector_rfid, cliente_servidor)
    │                          verificacion = fusión radar+ToF que decide el conteo oficial
    └── pruebas/            Sketches para probar cada pieza por separado
```

**Regla de las capas:** una petición baja en orden `rutas → servicios → repositorios`.
Cada capa solo conoce a la de abajo, así cada archivo hace una sola cosa.

| Si quieres cambiar… | Edita… |
|---|---|
| Colores o fuentes | `app/static/css/variables.css` |
| El aspecto de una pestaña | `app/templates/partials/_<pestaña>.html` y su CSS |
| Cómo se comporta una pestaña | `app/static/js/vistas/<pestaña>.js` |
| La regla de entrada/salida del docente | `app/servicios/dispositivos.py` |
| Una consulta a la base de datos | `app/repositorios/` |
| Una ruta nueva de la API | `app/rutas/` |
| Claves y tiempos del servidor | `config.py` |
| WiFi, pines y umbrales del ESP32 | `firmware/aula_completo/config.h` |

## Servidor

```
pip install -r requirements.txt
python run.py
```

Abre `http://localhost:5000`. Antes de usarlo, cambia en `config.py` (o con variables de entorno)
`API_KEY`, `CLAVE_PANEL` y `CLAVE_SECRETA`.

## Cableado del ESP32

| Módulo | Pin del módulo | Pin del ESP32 |
|---|---|---|
| Radar LD2450 | TX / RX / 5V / GND | GPIO16 / GPIO17 / 5V / GND |
| Lector RC522 (3.3V, nunca 5V) | SDA / SCK / MOSI / MISO / RST | GPIO5 / GPIO18 / GPIO23 / GPIO19 / **GPIO27** |
| Sensores VL53L1X A y B (comparten bus) | SDA / SCL | GPIO21 / GPIO22 |
| VL53L1X A | XSHUT | GPIO25 |
| VL53L1X B | XSHUT | GPIO26 |
| VL53L1X A y B | VIN / GND | 3V3 / GND |

El pin GPIO22 lo usa el bus I2C de los VL53L1X, por eso el RST del lector RFID quedó en GPIO27.
Los pines se cambian en `firmware/aula_completo/config.h`.

## Montaje: cómo se instala el equipo

```
        FUERA                          │  SALÓN
                                       │
   ┌──── ToF A (arriba, lado afuera) ──┼── ToF B (arriba, lado adentro) ────┐
   │                                  │                                    │
   │            puerta ───────────────┼────────────                        │
   │                                  │                                    │
   │                    radar LD2450 a ~135 cm, dentro del salón, ◄━━━━━━━━┿━━ mirando hacia adentro
   │                                  │                                    │
   └──────────────────────────────────┴────────────────────────────────────┘
```

- **Dos ToF arriba del marco de la puerta**, uno tras otro en el sentido de caminata:
  **A** del lado de fuera y **B** del lado de dentro, apuntando hacia abajo al umbral.
  El orden en que se activan dice el sentido: A→B = entrada, B→A = salida.
- **Radar LD2450 en el mismo marco, a unos 135 cm del piso, DENTRO del salón y mirando
  hacia adentro.** Por eso quien **entra** pasa de **cerca a lejos** del radar y quien
  **sale** pasa de **lejos a cerca**.

## Conteo por fusión (radar + ToF): quién cuenta y quién confirma

`verificacion.cpp` es quien decide el conteo oficial que llega al servidor:

| | Lo propone | Lo confirma | Si no llega la confirmación |
|---|---|---|---|
| **ENTRADA** | ToF (cruce A→B) | radar (cerca→lejos) | se descarta: alguien solo se asomó |
| **SALIDA** | radar (lejos→cerca) | ToF (cruce B→A) | se descarta: se acercaron y se regresaron |

La confirmación puede tardar hasta `CONFIRMACION_ESPERA_MS` (5 s). Los cruces descartados
se anotan como `sin_confirmar`; el panel muestra el conteo del ToF junto al oficial y avisa
"Revisar" si difieren en 2 personas o más. Si un sensor se cae (radar sin tramas 5 s, o ToF
sin conectar), **el otro cuenta solo** con un aviso en el Monitor Serie.

El radar solo sigue a quien esté dentro del **pasillo de la puerta**: cono de ±25°
(`ANGULO_MAXIMO`), ancho ±600 mm (`X_MAX`), rango 20 cm–1.8 m (`Y_MIN`/`Y_MAX`) y línea de
conteo a 70 cm del radar (`LINEA_Y`). Así ignora a la gente de las mesas (que están a ~150 cm
y quedan fuera de `Y_MAX`) y evita los
"falsos" de varias personas a la vez (más filtros: permanencia mínima en la zona, salto
imposible entre tramas y espera entre cruces de la misma ranura).

Comandos del Monitor Serie relacionados: `t` muestra las distancias de los ToF,
`k` recalibra los ToF (con la puerta vacía), `d` muestra lo que ve el radar.

## Firmware

1. Instala las librerías **MFRC522** (GithubCommunity) y **VL53L1X** (Pololu) en Arduino IDE.
2. Abre `firmware/aula_completo/aula_completo.ino` y edita `config.h`
   (WiFi, IP del servidor, `AULA_ID` y la misma `API_KEY` del servidor).
3. Sube el código a la placa **ESP32 Dev Module**.

## Si algo no cuenta o no se lee

En el Monitor Serie del ESP32 (115200 baudios):

- Escribe `d` para ver lo que detecta el radar (posición de cada persona y si está "lejos", "cerca", en la "zona muerta" de la línea de conteo, o fuera del cono/pasillo). Vuelve a escribir `d` para ocultarlo.
- Escribe `r` para reiniciar todos los contadores a 0 (radar, ToF y fusión), y `e` para ocultar o mostrar el informe de estado que sale cada 5 segundos. Ahí aparece el `modo:` (radar+tof, solo tof o solo radar).
- Si el radar manda bytes pero el programa no recibe tramas, casi siempre es la **velocidad (baudios)** del radar: la app HLKRadarTool permite cambiarla. `aula_completo` la busca sola al arrancar (`RADAR_BAUDIOS_AUTO` en `config.h`) y `diagnostico_hardware` la muestra con el comando `b`.
- Si el radar o el lector RFID no responden, carga primero `firmware/pruebas/diagnostico_hardware`: prueba ambos sin WiFi y muestra con números qué funciona (tramas del radar por segundo, tarjetas leídas, motivo del último reinicio).
- Para que cuente una **entrada**, los ToF deben ver el cruce A→B por la puerta **y** el radar debe ver a la persona pasar de "cerca" a "lejos" de la línea (`LINEA_Y`, a 70 cm del radar). Caminar de lado frente al radar, o moverse sin cruzar la línea, no cuenta. Para una **salida** es al revés: primero el radar (lejos→cerca) y luego los ToF (B→A).
- Si el Monitor Serie muestra "Descartada ENTRADA/SALIDA", alguien se asomó a la puerta o se acercó al radar sin salir: es el filtro de confirmación haciendo su trabajo. Si pasan muchas veces seguidas, revisa con `d` y `t` que ambos sensores estén viendo el mismo cruce.
- Si los ToF calibran con un valor fuera de rango (menos de 1200 mm o más de 3000 mm), el sensor no está arriba del marco apuntando al umbral: corrige su ángulo y pulsa `k` con la puerta vacía.
- Una tarjeta se vuelve a leer cuando se aleja del lector unos instantes. El servidor ignora la misma tarjeta si se pasa dos veces en menos de `SEGUNDOS_ANTIREBOTE` (3 s por defecto, en `config.py`).

## Pruebas

```
pip install -r requirements-dev.txt
pytest
```

## API

| Método y ruta | Quién la usa | Descripción |
|---|---|---|
| `POST /api/conteo` | ESP32 (X-API-Key) | Reporta personas en el aula |
| `POST /api/rfid` | ESP32 (X-API-Key) | Registra entrada o salida de un docente |
| `GET /api/estado` | Panel | Estado de todas las aulas |
| `GET /api/historial?aula=&horas=` | Panel | Conteos en el tiempo |
| `GET /api/registros` · `/api/registros.csv` | Panel | Registro de docentes (filtros: aula, docente_id, desde, hasta) |
| `GET/POST/PUT/DELETE /api/docentes` | Panel | Gestión de docentes |
| `POST/PUT/DELETE /api/aulas` | Panel | Gestión de aulas |
| `POST /api/registros/<id>/cerrar` | Panel | Marcar salida manual |
