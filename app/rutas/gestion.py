"""API del panel para crear, editar y eliminar."""
from flask import Blueprint, jsonify

from ..servicios import gestion
from ..utilidades import cuerpo_json

bp = Blueprint("gestion", __name__, url_prefix="/api")


@bp.post("/docentes")
def crear_docente():
    gestion.crear_docente(cuerpo_json())
    return jsonify(ok=True)


@bp.put("/docentes/<int:docente_id>")
def editar_docente(docente_id):
    gestion.editar_docente(docente_id, cuerpo_json())
    return jsonify(ok=True)


@bp.delete("/docentes/<int:docente_id>")
def eliminar_docente(docente_id):
    gestion.eliminar_docente(docente_id)
    return jsonify(ok=True)


@bp.post("/aulas")
def crear_aula():
    gestion.crear_aula(cuerpo_json())
    return jsonify(ok=True)


@bp.put("/aulas/<aula_id>")
def renombrar_aula(aula_id):
    gestion.renombrar_aula(aula_id, cuerpo_json())
    return jsonify(ok=True)


@bp.delete("/aulas/<aula_id>")
def eliminar_aula(aula_id):
    gestion.eliminar_aula(aula_id)
    return jsonify(ok=True)


@bp.post("/registros/<int:registro_id>/cerrar")
def cerrar_registro(registro_id):
    gestion.cerrar_registro(registro_id)
    return jsonify(ok=True)
