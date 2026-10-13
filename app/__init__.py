"""Fábrica de la aplicación Flask."""
from flask import Flask

from config import Config

from . import auth, db, errores
from .rutas import registrar_rutas


def crear_app(configuracion=Config):
    app = Flask(__name__)
    app.config.from_object(configuracion)

    db.inicializar(app)
    errores.registrar(app)
    app.register_blueprint(auth.bp)
    app.before_request(auth.proteger_panel)
    registrar_rutas(app)
    return app
