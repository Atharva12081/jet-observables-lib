"""Small local benchmark for the optional C++ observable prototype."""

from __future__ import annotations

import time
from pathlib import Path
import sys

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
if str(SRC) not in sys.path:
    sys.path.insert(0, str(SRC))

from jetobsmc.accelerated import HAS_CPP_ACCELERATION, jet_width_fast, tau21_fast
from jetobsmc.jet import Jet
from jetobsmc.observables.shapes import jet_width
from jetobsmc.observables.substructure import nsubjettiness_tau21


def random_jet(rng: np.random.Generator, n_particles: int) -> Jet:
    pt = rng.uniform(5.0, 120.0, size=n_particles)
    eta = rng.normal(0.0, 0.9, size=n_particles)
    phi = rng.uniform(-np.pi, np.pi, size=n_particles)
    px = pt * np.cos(phi)
    py = pt * np.sin(phi)
    pz = pt * np.sinh(eta)
    energy = np.sqrt(px * px + py * py + pz * pz)
    return Jet(np.column_stack([energy, px, py, pz]))


def benchmark(function, jets: list[Jet]) -> float:
    start = time.perf_counter()
    for jet in jets:
        function(jet)
    return (time.perf_counter() - start) * 1000.0 / len(jets)


def main() -> None:
    rng = np.random.default_rng(20260319)
    jets = [random_jet(rng, 64) for _ in range(1000)]

    width_python = benchmark(jet_width, jets)
    width_cpp = benchmark(jet_width_fast, jets)
    tau21_python = benchmark(nsubjettiness_tau21, jets)
    tau21_cpp = benchmark(tau21_fast, jets)

    print("C++ acceleration available:", HAS_CPP_ACCELERATION)
    print(f"jet_width python  [ms/jet]: {width_python:.6f}")
    print(f"jet_width cpp     [ms/jet]: {width_cpp:.6f}")
    print(f"tau21 python      [ms/jet]: {tau21_python:.6f}")
    print(f"tau21 cpp         [ms/jet]: {tau21_cpp:.6f}")
    print(f"jet_width absolute diff   : {abs(jet_width(jets[0]) - jet_width_fast(jets[0])):.6e}")
    print(
        f"tau21 absolute diff       : "
        f"{abs(nsubjettiness_tau21(jets[0]) - tau21_fast(jets[0])):.6e}"
    )


if __name__ == "__main__":
    main()
