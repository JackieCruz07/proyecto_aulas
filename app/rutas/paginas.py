"""Páginas HTML."""
from flask import Blueprint, render_template

bp = Blueprint("paginas", __name__)


@bp.get("/")
def panel():
    return render_template("panel.html")
