import numpy as np

from jetobsmc.accelerated import HAS_CPP_ACCELERATION, cpp_status, jet_width_fast, tau21_fast
from jetobsmc.jet import Jet
from jetobsmc.observables.shapes import jet_width
from jetobsmc.observables.substructure import nsubjettiness_tau21


def sample_jet() -> Jet:
    return Jet(
        np.array(
            [
                [42.0, 18.0, 3.0, 38.0],
                [33.0, 10.0, 6.0, 30.0],
                [19.0, -4.0, 3.0, 17.0],
                [14.0, 2.0, -1.0, 12.0],
            ]
        )
    )


def test_cpp_status_is_stable_string() -> None:
    assert cpp_status() in {"available", "not-built"}


def test_fast_helpers_match_python_fallback() -> None:
    jet = sample_jet()
    assert np.isclose(jet_width_fast(jet), jet_width(jet), rtol=1e-12, atol=1e-12)
    assert np.isclose(tau21_fast(jet), nsubjettiness_tau21(jet), rtol=1e-12, atol=1e-12)


def test_cpp_flag_is_boolean() -> None:
    assert isinstance(HAS_CPP_ACCELERATION, bool)
