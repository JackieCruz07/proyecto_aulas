"""Errores de la API: los servicios los lanzan y aquí se convierten en respuestas JSON."""
from flask import jsonify


class ErrorApi(Exception):
    def __init__(self, mensaje, codigo=400, **extra):
        super().__init__(mensaje)
        self.mensaje = mensaje
        self.codigo = codigo
        self.extra = extra


def registrar(app):
    @app.errorhandler(ErrorApi)
    def manejar_error_api(error):
        return jsonify(ok=False, error=error.mensaje, **error.extra), error.codigo
