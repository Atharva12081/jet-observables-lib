#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace {

struct ParticleView {
    const char* data;
    Py_ssize_t stride0;
    Py_ssize_t stride1;
    Py_ssize_t rows;
};

double cell_as_double(const ParticleView& view, Py_ssize_t row, Py_ssize_t col) {
    const char* ptr = view.data + row * view.stride0 + col * view.stride1;
    return *reinterpret_cast<const double*>(ptr);
}

double wrap_delta_phi(double delta) {
    constexpr double pi = 3.14159265358979323846;
    constexpr double two_pi = 2.0 * pi;
    while (delta > pi) {
        delta -= two_pi;
    }
    while (delta < -pi) {
        delta += two_pi;
    }
    return delta;
}

double particle_pt(const ParticleView& view, Py_ssize_t row) {
    const double px = cell_as_double(view, row, 1);
    const double py = cell_as_double(view, row, 2);
    return std::hypot(px, py);
}

void particle_eta_phi(const ParticleView& view, Py_ssize_t row, double& eta, double& phi) {
    const double px = cell_as_double(view, row, 1);
    const double py = cell_as_double(view, row, 2);
    const double pz = cell_as_double(view, row, 3);
    const double pabs = std::sqrt(px * px + py * py + pz * pz);
    eta = 0.5 * std::log((pabs + pz) / std::max(pabs - pz, 1e-15));
    phi = std::atan2(py, px);
}

bool parse_particles(PyObject* obj, Py_buffer* buffer, ParticleView& view) {
    if (PyObject_GetBuffer(obj, buffer, PyBUF_STRIDES | PyBUF_FORMAT) != 0) {
        return false;
    }
    if (buffer->ndim != 2) {
        PyErr_SetString(PyExc_ValueError, "Expected a 2D particles array.");
        PyBuffer_Release(buffer);
        return false;
    }
    if (buffer->shape[1] != 4) {
        PyErr_SetString(PyExc_ValueError, "Expected particles with shape (N, 4).");
        PyBuffer_Release(buffer);
        return false;
    }
    if (buffer->itemsize != static_cast<Py_ssize_t>(sizeof(double))) {
        PyErr_SetString(PyExc_TypeError, "Expected particles with float64 / double storage.");
        PyBuffer_Release(buffer);
        return false;
    }
    if (buffer->format == nullptr || buffer->format[0] != 'd' || buffer->format[1] != '\0') {
        PyErr_SetString(PyExc_TypeError, "Expected particles buffer format 'd' (double).");
        PyBuffer_Release(buffer);
        return false;
    }
    view.data = static_cast<const char*>(buffer->buf);
    view.stride0 = buffer->strides[0];
    view.stride1 = buffer->strides[1];
    view.rows = buffer->shape[0];
    return true;
}

double tau_n_proxy_impl(const ParticleView& view, int n_axes) {
    if (view.rows == 0 || n_axes <= 0 || n_axes >= view.rows) {
        return 0.0;
    }

    std::vector<double> pts(view.rows);
    std::vector<double> etas(view.rows);
    std::vector<double> phis(view.rows);
    double pt_sum = 0.0;
    for (Py_ssize_t i = 0; i < view.rows; ++i) {
        pts[i] = particle_pt(view, i);
        pt_sum += pts[i];
        particle_eta_phi(view, i, etas[i], phis[i]);
    }
    if (pt_sum == 0.0) {
        return 0.0;
    }

    std::vector<Py_ssize_t> axis_idx(view.rows);
    for (Py_ssize_t i = 0; i < view.rows; ++i) {
        axis_idx[i] = i;
    }
    std::partial_sort(
        axis_idx.begin(),
        axis_idx.begin() + n_axes,
        axis_idx.end(),
        [&pts](Py_ssize_t a, Py_ssize_t b) { return pts[a] > pts[b]; });

    double total = 0.0;
    for (Py_ssize_t i = 0; i < view.rows; ++i) {
        double dr_min = std::numeric_limits<double>::infinity();
        for (int axis = 0; axis < n_axes; ++axis) {
            const Py_ssize_t a = axis_idx[axis];
            const double deta = etas[i] - etas[a];
            const double dphi = wrap_delta_phi(phis[i] - phis[a]);
            dr_min = std::min(dr_min, std::hypot(deta, dphi));
        }
        total += pts[i] * dr_min;
    }
    return total;
}

PyObject* jet_width_cpp(PyObject*, PyObject* args) {
    PyObject* particles_obj = nullptr;
    double jet_eta = 0.0;
    double jet_phi = 0.0;
    if (!PyArg_ParseTuple(args, "Odd", &particles_obj, &jet_eta, &jet_phi)) {
        return nullptr;
    }

    Py_buffer buffer{};
    ParticleView view{};
    if (!parse_particles(particles_obj, &buffer, view)) {
        return nullptr;
    }

    double result = 0.0;
    double denom = 0.0;
    for (Py_ssize_t i = 0; i < view.rows; ++i) {
        const double pt = particle_pt(view, i);
        denom += pt;
        double eta = 0.0;
        double phi = 0.0;
        particle_eta_phi(view, i, eta, phi);
        const double dr = std::hypot(eta - jet_eta, wrap_delta_phi(phi - jet_phi));
        result += pt * dr;
    }
    PyBuffer_Release(&buffer);
    if (denom == 0.0) {
        return PyFloat_FromDouble(0.0);
    }
    return PyFloat_FromDouble(result / denom);
}

PyObject* tau1_proxy_cpp(PyObject*, PyObject* args) {
    PyObject* particles_obj = nullptr;
    if (!PyArg_ParseTuple(args, "O", &particles_obj)) {
        return nullptr;
    }
    Py_buffer buffer{};
    ParticleView view{};
    if (!parse_particles(particles_obj, &buffer, view)) {
        return nullptr;
    }
    const double result = tau_n_proxy_impl(view, 1);
    PyBuffer_Release(&buffer);
    return PyFloat_FromDouble(result);
}

PyObject* tau2_proxy_cpp(PyObject*, PyObject* args) {
    PyObject* particles_obj = nullptr;
    if (!PyArg_ParseTuple(args, "O", &particles_obj)) {
        return nullptr;
    }
    Py_buffer buffer{};
    ParticleView view{};
    if (!parse_particles(particles_obj, &buffer, view)) {
        return nullptr;
    }
    const double result = tau_n_proxy_impl(view, 2);
    PyBuffer_Release(&buffer);
    return PyFloat_FromDouble(result);
}

PyObject* tau21_proxy_cpp(PyObject*, PyObject* args) {
    PyObject* particles_obj = nullptr;
    if (!PyArg_ParseTuple(args, "O", &particles_obj)) {
        return nullptr;
    }
    Py_buffer buffer{};
    ParticleView view{};
    if (!parse_particles(particles_obj, &buffer, view)) {
        return nullptr;
    }
    const double tau1 = tau_n_proxy_impl(view, 1);
    const double tau2 = tau_n_proxy_impl(view, 2);
    PyBuffer_Release(&buffer);
    if (tau1 <= 0.0) {
        return PyFloat_FromDouble(0.0);
    }
    return PyFloat_FromDouble(tau2 / tau1);
}

PyMethodDef module_methods[] = {
    {"jet_width_cpp", jet_width_cpp, METH_VARARGS, "Compute jet width from a float64 particles array and jet eta/phi."},
    {"tau1_proxy_cpp", tau1_proxy_cpp, METH_VARARGS, "Compute the tau1 proxy from a float64 particles array."},
    {"tau2_proxy_cpp", tau2_proxy_cpp, METH_VARARGS, "Compute the tau2 proxy from a float64 particles array."},
    {"tau21_proxy_cpp", tau21_proxy_cpp, METH_VARARGS, "Compute the tau21 proxy ratio from a float64 particles array."},
    {nullptr, nullptr, 0, nullptr},
};

PyModuleDef module_definition = {
    PyModuleDef_HEAD_INIT,
    "_fastobs",
    "Optional C++ acceleration prototypes for JetObsMC observables.",
    -1,
    module_methods,
};

}  // namespace

PyMODINIT_FUNC PyInit__fastobs(void) {
    return PyModule_Create(&module_definition);
}
