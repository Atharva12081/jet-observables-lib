"""Optional C++ acceleration helpers.

The package remains fully usable without a compiled extension. When the
`jetobsmc._fastobs` module is available, these helpers expose a small C++
prototype for `jet_width` and proxy `tau` observables.
"""

from __future__ import annotations

from .jet import Jet
from .observables.shapes import jet_width
from .observables.substructure import (
    nsubjettiness_tau1,
    nsubjettiness_tau2,
    nsubjettiness_tau21,
)

try:  # pragma: no cover - depends on local build environment
    from ._fastobs import jet_width_cpp, tau1_proxy_cpp, tau2_proxy_cpp, tau21_proxy_cpp

    HAS_CPP_ACCELERATION = True
except ImportError:  # pragma: no cover - default pure-python path
    jet_width_cpp = None
    tau1_proxy_cpp = None
    tau2_proxy_cpp = None
    tau21_proxy_cpp = None
    HAS_CPP_ACCELERATION = False


def cpp_status() -> str:
    """Return a short human-readable status string."""
    return "available" if HAS_CPP_ACCELERATION else "not-built"


def jet_width_fast(jet: Jet) -> float:
    """Use the C++ prototype when available, else fall back to Python."""
    if HAS_CPP_ACCELERATION:
        return float(jet_width_cpp(jet.particles, jet.eta(), jet.phi()))
    return jet_width(jet)


def tau1_fast(jet: Jet) -> float:
    """Use the C++ prototype when available, else fall back to Python."""
    if HAS_CPP_ACCELERATION:
        return float(tau1_proxy_cpp(jet.particles))
    return nsubjettiness_tau1(jet)


def tau2_fast(jet: Jet) -> float:
    """Use the C++ prototype when available, else fall back to Python."""
    if HAS_CPP_ACCELERATION:
        return float(tau2_proxy_cpp(jet.particles))
    return nsubjettiness_tau2(jet)


def tau21_fast(jet: Jet) -> float:
    """Use the C++ prototype when available, else fall back to Python."""
    if HAS_CPP_ACCELERATION:
        return float(tau21_proxy_cpp(jet.particles))
    return nsubjettiness_tau21(jet)


__all__ = [
    "HAS_CPP_ACCELERATION",
    "cpp_status",
    "jet_width_fast",
    "tau1_fast",
    "tau2_fast",
    "tau21_fast",
]
