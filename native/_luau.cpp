#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "lua.h"
#include "lualib.h"
#include "luacode.h"

#include "Luau/Common.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <string>

static PyObject* LuauError = nullptr;

static int capturedPrint(lua_State* L)
{
    std::string* output = static_cast<std::string*>(lua_tolightuserdata(L, lua_upvalueindex(1)));
    if (!output)
        return 0;

    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++)
    {
        size_t len = 0;
        const char* s = luaL_tolstring(L, i, &len);
        if (i > 1)
            *output += "\t";
        output->append(s, len);
        lua_pop(L, 1);
    }

    *output += "\n";
    return 0;
}

static void setupState(lua_State* L, std::string& output)
{
    luaL_openlibs(L);

    lua_pushlightuserdata(L, &output);
    lua_pushcclosure(L, capturedPrint, "print", 1);
    lua_setglobal(L, "print");

    luaL_sandbox(L);
}

static void enableLuauFeatureFlags()
{
    for (Luau::FValue<bool>* flag = Luau::FValue<bool>::list; flag; flag = flag->next)
    {
        if (std::strncmp(flag->name, "Luau", 4) == 0)
            flag->value = true;
    }
}

static std::string runCode(lua_State* L, const char* source, size_t sourceSize)
{
    size_t bytecodeSize = 0;
    std::unique_ptr<char, decltype(&std::free)> bytecode(luau_compile(source, sourceSize, nullptr, &bytecodeSize), &std::free);
    if (!bytecode)
        throw std::bad_alloc();

    int result = luau_load(L, "=stdin", bytecode.get(), bytecodeSize, 0);
    if (result != 0)
    {
        size_t len = 0;
        const char* msg = lua_tolstring(L, -1, &len);
        std::string error(msg ? msg : "", msg ? len : 0);
        lua_pop(L, 1);
        return error;
    }

    lua_State* T = lua_newthread(L);

    lua_pushvalue(L, -2);
    lua_remove(L, -3);
    lua_xmove(L, T, 1);

    int status = lua_resume(T, nullptr, 0);

    if (status == 0)
    {
        int n = lua_gettop(T);

        if (n)
        {
            luaL_checkstack(T, LUA_MINSTACK, "too many results to print");
            lua_getglobal(T, "print");
            lua_insert(T, 1);
            lua_pcall(T, n, 0, 0);
        }

        lua_pop(L, 1);
        return std::string();
    }

    std::string error;

    lua_Debug ar;
    if (lua_getinfo(L, 0, "sln", &ar))
    {
        error += ar.short_src;
        error += ":";
        error += std::to_string(ar.currentline);
        error += ": ";
    }

    if (status == LUA_YIELD)
    {
        error += "thread yielded unexpectedly";
    }
    else if (const char* str = lua_tostring(T, -1))
    {
        error += str;
    }

    error += "\nstack backtrace:\n";
    error += lua_debugtrace(T);

    lua_pop(L, 1);
    return error;
}

static PyObject* pyExecute(PyObject*, PyObject* args)
{
    const char* source = nullptr;
    Py_ssize_t sourceSize = 0;

    if (!PyArg_ParseTuple(args, "s#:execute", &source, &sourceSize))
        return nullptr;

    try
    {
        enableLuauFeatureFlags();

        std::string output;
        std::unique_ptr<lua_State, void (*)(lua_State*)> globalState(luaL_newstate(), lua_close);
        if (!globalState)
            return PyErr_NoMemory();

        lua_State* L = globalState.get();
        setupState(L, output);
        luaL_sandboxthread(L);

        std::string error = runCode(L, source, static_cast<size_t>(sourceSize));
        if (!error.empty())
        {
            PyErr_SetString(LuauError, error.c_str());
            return nullptr;
        }

        return PyUnicode_FromStringAndSize(output.data(), static_cast<Py_ssize_t>(output.size()));
    }
    catch (const std::bad_alloc&)
    {
        return PyErr_NoMemory();
    }
    catch (const std::exception& e)
    {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return nullptr;
    }
}

static PyMethodDef LuauMethods[] = {
    {"execute", pyExecute, METH_VARARGS, PyDoc_STR("Execute Luau source and return captured output.")},
    {nullptr, nullptr, 0, nullptr},
};

static struct PyModuleDef LuauModule = {
    PyModuleDef_HEAD_INIT,
    "_luau",
    "Luau WebAssembly extension module.",
    -1,
    LuauMethods,
};

PyMODINIT_FUNC PyInit__luau(void)
{
    PyObject* module = PyModule_Create(&LuauModule);
    if (!module)
        return nullptr;

    LuauError = PyErr_NewException("luau_wasm.LuauError", nullptr, nullptr);
    if (!LuauError)
    {
        Py_DECREF(module);
        return nullptr;
    }

    Py_INCREF(LuauError);
    if (PyModule_AddObject(module, "LuauError", LuauError) < 0)
    {
        Py_DECREF(LuauError);
        Py_DECREF(module);
        return nullptr;
    }

    return module;
}
