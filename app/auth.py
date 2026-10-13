"""Acceso: contraseña del panel para personas y clave de API para los ESP32."""
import hmac
from functools import wraps

from flask import (Blueprint, current_app, jsonify, redirect, render_template,
                   request, session, url_for)

from .errores import ErrorApi

bp = Blueprint("auth", __name__)

# Vistas que no piden contraseña del panel (los ESP32 usan su propia clave de API)
ENDPOINTS_LIBRES = {"auth.login", "dispositivos.conteo", "dispositivos.tarjeta", "static"}


def _iguales(a, b):
    return hmac.compare_digest(str(a).encode(), str(b).encode())


@bp.route("/login", methods=["GET", "POST"])
def login():
    error = None
    if request.method == "POST":
        if _iguales(request.form.get("clave", ""), current_app.config["CLAVE_PANEL"]):
            session.clear()
            session["ok"] = True
            session.permanent = True
            return redirect(url_for("paginas.panel"))
        error = "La contraseña no es correcta."
    return render_template("login.html", error=error), (401 if error else 200)


@bp.get("/logout")
def logout():
    session.clear()
    return redirect(url_for("auth.login"))


def proteger_panel():
    """Se ejecuta antes de cada petición."""
    if request.endpoint is None or request.endpoint in ENDPOINTS_LIBRES or session.get("ok"):
        return None
    if request.path.startswith("/api/"):
        return jsonify(ok=False, error="No autorizado"), 401
    return redirect(url_for("auth.login"))


def requerir_api_key(vista):
    """Decorador para las rutas que usan los ESP32."""
    @wraps(vista)
    def envoltura(*args, **kwargs):
        if not _iguales(request.headers.get("X-API-Key", ""), current_app.config["API_KEY"]):
            raise ErrorApi("Clave de API incorrecta", 401)
        return vista(*args, **kwargs)
    return envoltura
