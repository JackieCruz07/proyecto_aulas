"""API que usan los ESP32 (se identifican con la clave X-API-Key)."""
from flask import Blueprint, jsonify

from ..auth import requerir_api_key
from ..servicios import dispositivos
from ..utilidades import cuerpo_json

bp = Blueprint("dispositivos", __name__, url_prefix="/api")


@bp.post("/conteo")
@requerir_api_key
def conteo():
    dispositivos.registrar_conteo(cuerpo_json())
    return jsonify(ok=True)


@bp.post("/rfid")
@requerir_api_key
def tarjeta():
    return jsonify(ok=True, **dispositivos.registrar_tarjeta(cuerpo_json()))
