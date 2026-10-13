# Gestión de aulas con ESP32, radar LD2450 y RFID

Cada aula tiene un ESP32 que cuenta las personas con un radar y registra al docente con una tarjeta RFID.
Un servidor en Python guarda todo en una base de datos y muestra un panel web para gestionar las aulas.

```
ESP32 de cada aula  ──WiFi──►  Servidor Flask + SQLite  ◄──  Panel web
(radar + RFID)                    (carpeta app/)            (cualquier navegador)
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
    ├── aula_completo/      Programa final (config.h, conteo, lector_rfid, cliente_servidor)
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

## Firmware

1. Instala la librería **MFRC522** (GithubCommunity) en Arduino IDE.
2. Abre `firmware/aula_completo/aula_completo.ino` y edita `config.h`
   (WiFi, IP del servidor, `AULA_ID` y la misma `API_KEY` del servidor).
3. Sube el código a la placa **ESP32 Dev Module**.

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
