"""Conexión a SQLite y creación de las tablas."""
import os
import sqlite3

from flask import current_app, g

RUTA_ESQUEMA = os.path.join(os.path.dirname(__file__), "esquema.sql")


def obtener_db():
    """Una conexión por petición; se cierra sola al terminar."""
    if "db" not in g:
        g.db = sqlite3.connect(current_app.config["BASE_DATOS"])
        g.db.row_factory = sqlite3.Row
    return g.db


def cerrar_db(_error=None):
    conexion = g.pop("db", None)
    if conexion is not None:
        conexion.close()

COLUMNAS_NUEVAS = {
    "aulas": [
        ("personas_tof", "INTEGER"),               # conteo de los sensores VL53L1X (NULL si el aula no los tiene)
        ("sin_confirmar", "INTEGER DEFAULT 0"),    # pasos que el radar y el ToF no confirmaron entre sí
    ],
}


def migrar(conexion):
    for tabla, columnas in COLUMNAS_NUEVAS.items():
        existentes = {fila[1] for fila in conexion.execute(f"PRAGMA table_info({tabla})")}
        for nombre, tipo in columnas:
            if nombre not in existentes:
                conexion.execute(f"ALTER TABLE {tabla} ADD COLUMN {nombre} {tipo}")


def inicializar(app):
    app.teardown_appcontext(cerrar_db)
    with open(RUTA_ESQUEMA, encoding="utf-8") as archivo:
        esquema = archivo.read()
    conexion = sqlite3.connect(app.config["BASE_DATOS"])
    conexion.executescript(esquema)
    migrar(conexion)
    conexion.commit()
    conexion.close()
