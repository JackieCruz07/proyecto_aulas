"""Rutas HTTP. Cada archivo agrupa un tipo de petición y solo llama a los servicios."""
from . import consultas, dispositivos, gestion, paginas


def registrar_rutas(app):
    for modulo in (dispositivos, consultas, gestion, paginas):
        app.register_blueprint(modulo.bp)
