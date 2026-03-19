import os
import sysconfig

from setuptools import Extension, find_packages, setup
from setuptools.command.build_ext import build_ext


class OptionalBuildExt(build_ext):
    """Allow pure-Python installation to continue if the C++ module fails."""

    def run(self):
        try:
            super().run()
        except Exception as exc:  # pragma: no cover - install-time path
            print(f"[jetobsmc] optional C++ extension build skipped: {exc}")

    def build_extension(self, ext):
        try:
            super().build_extension(ext)
        except Exception as exc:  # pragma: no cover - install-time path
            print(f"[jetobsmc] optional extension {ext.name} skipped: {exc}")

    def get_ext_filename(self, ext_name):
        filename = super().get_ext_filename(ext_name)
        ext_suffix = sysconfig.get_config_var("EXT_SUFFIX")
        if not ext_suffix:
            return filename
        base, _ = os.path.splitext(filename)
        return base + ext_suffix


fastobs_extension = Extension(
    "jetobsmc._fastobs",
    sources=["src/jetobsmc/_fastobs.cpp"],
    language="c++",
    include_dirs=[sysconfig.get_paths()["include"]],
    extra_compile_args=["-std=c++17"],
)

setup(
    name="jetobsmc",
    version="0.3.0",
    description="JetObsMC: unified jet observable toolkit for Monte Carlo validation workflows.",
    url="https://github.com/Atharva12081/JetObsMC",
    license="MIT",
    long_description=open("README.md", encoding="utf-8").read(),
    long_description_content_type="text/markdown",
    python_requires=">=3.10",
    install_requires=["numpy>=1.24"],
    packages=find_packages(where="src"),
    package_dir={"": "src"},
    classifiers=[
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3 :: Only",
        "Operating System :: OS Independent",
    ],
    extras_require={
        "dev": [
            "pytest>=7.0",
            "notebook>=7.0",
            "matplotlib>=3.8",
            "scikit-learn>=1.4",
            "black>=24.0",
        ]
    },
    ext_modules=[fastobs_extension],
    cmdclass={"build_ext": OptionalBuildExt},
)
