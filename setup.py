from pathlib import Path
import sys

from setuptools import Extension, setup


ROOT = Path(__file__).parent
LUAU = ROOT / "vendor" / "luau"


def cpp_sources(*parts):
    return sorted(str(path.relative_to(ROOT)) for path in (LUAU.joinpath(*parts)).glob("*.cpp"))


sources = [
    str(Path("native") / "_luau.cpp"),
    *cpp_sources("Common", "src"),
    *cpp_sources("Ast", "src"),
    *cpp_sources("Bytecode", "src"),
    *cpp_sources("Compiler", "src"),
    *cpp_sources("VM", "src"),
]

include_dirs = [
    str(Path("vendor") / "luau" / "Common" / "include"),
    str(Path("vendor") / "luau" / "Ast" / "include"),
    str(Path("vendor") / "luau" / "Bytecode" / "include"),
    str(Path("vendor") / "luau" / "Compiler" / "include"),
    str(Path("vendor") / "luau" / "VM" / "include"),
]

if sys.platform == "win32":
    extra_compile_args = ["/std:c++17", "/EHsc"]
    extra_link_args = []
else:
    extra_compile_args = ["-std=c++17", "-fexceptions", "-fno-math-errno"]
    extra_link_args = ["-fexceptions"]


setup(
    ext_modules=[
        Extension(
            "luau_wasm._luau",
            sources=sources,
            include_dirs=include_dirs,
            define_macros=[("LUA_USE_LONGJMP", "1")],
            extra_compile_args=extra_compile_args,
            extra_link_args=extra_link_args,
            language="c++",
        )
    ]
)
