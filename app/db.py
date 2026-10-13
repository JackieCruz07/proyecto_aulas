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


def inicializar(app):
    app.teardown_appcontext(cerrar_db)
    with open(RUTA_ESQUEMA, encoding="utf-8") as archivo:
        esquema = archivo.read()
    conexion = sqlite3.connect(app.config["BASE_DATOS"])
    conexion.executescript(esquema)
    conexion.commit()
    conexion.close()
