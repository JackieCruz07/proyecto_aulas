"""Funciones pequeñas que se usan en varias partes."""
from datetime import datetime

from flask import request

FORMATO = "%Y-%m-%d %H:%M:%S"


def ahora():
    return datetime.now().strftime(FORMATO)


def a_fecha(texto):
    return datetime.strptime(texto, FORMATO)


def segundos_desde(texto):
    return (datetime.now() - a_fecha(texto)).total_seconds()


def limpiar_aula(valor):
    return str(valor or "").strip().upper()


def limpiar_uid(valor):
    return str(valor or "").strip().upper().replace(" ", "").replace(":", "")


def fecha_valida(texto):
    """Devuelve el texto si es una fecha AAAA-MM-DD válida; si no, None."""
    try:
        datetime.strptime(texto, "%Y-%m-%d")
        return texto
    except (TypeError, ValueError):
        return None


def cuerpo_json():
    return request.get_json(silent=True) or {}
