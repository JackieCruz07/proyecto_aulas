CREATE TABLE IF NOT EXISTS aulas (
    id TEXT PRIMARY KEY,
    nombre TEXT,
    personas INTEGER DEFAULT 0,
    ultima_conexion TEXT
);

CREATE TABLE IF NOT EXISTS docentes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    nombre TEXT NOT NULL,
    uid TEXT UNIQUE NOT NULL
);

CREATE TABLE IF NOT EXISTS registros_docente (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    aula_id TEXT NOT NULL,
    docente_id INTEGER NOT NULL,
    entrada TEXT NOT NULL,
    salida TEXT
);

CREATE TABLE IF NOT EXISTS conteos (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    aula_id TEXT NOT NULL,
    personas INTEGER NOT NULL,
    fecha TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS ix_conteos ON conteos (aula_id, fecha);
