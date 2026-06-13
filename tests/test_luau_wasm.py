import pytest

import luau_wasm


def test_execute_captures_print_output():
    assert luau_wasm.execute('print("Hello", "Luau")') == "Hello\tLuau\n"


def test_execute_prints_return_values():
    assert luau_wasm.execute("return 2 + 2") == "4\n"


def test_execute_raises_luau_error_for_compile_errors():
    with pytest.raises(luau_wasm.LuauError) as excinfo:
        luau_wasm.execute("local =")

    assert "Expected identifier" in str(excinfo.value)


def test_execute_raises_luau_error_for_runtime_errors():
    with pytest.raises(luau_wasm.LuauError) as excinfo:
        luau_wasm.execute('error("boom")')

    assert "boom" in str(excinfo.value)
