"""API de solo lectura que usa el panel."""
from flask import Blueprint, Response, jsonify, request

from ..servicios import panel

bp = Blueprint("consultas", __name__, url_prefix="/api")


@bp.get("/estado")
def estado():
    return jsonify(panel.estado_aulas())


@bp.get("/historial")
def historial():
    return jsonify(panel.historial_conteos(request.args.get("aula"), request.args.get("horas", 12, type=int)))


@bp.get("/registros")
def registros():
    filtros = panel.normalizar_filtros(request.args)
    return jsonify(panel.listar_registros(filtros, request.args.get("limite", 100, type=int)))


@bp.get("/registros.csv")
def registros_csv():
    texto = panel.exportar_registros_csv(panel.normalizar_filtros(request.args))
    return Response(
        texto,
        mimetype="text/csv",
        headers={"Content-Disposition": "attachment; filename=registros_docentes.csv"},
    )


@bp.get("/docentes")
def docentes():
    return jsonify(panel.listar_docentes())
