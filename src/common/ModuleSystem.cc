#include "ModuleSystem.h"

namespace ModuleSystem
{
    void bind(lua_State *luaState)
    {
        luabridge::getGlobalNamespace(luaState)
            .beginNamespace("system")
            .addFunction("delay", delay)
            .endNamespace();
    }

    void bind(PyObject *mainModule)
    {
        auto module = pybind11::cast<pybind11::module>(mainModule);

        auto systemModule = module.def_submodule("system");
        systemModule.def("delay", delay);
    }

    void bind(JSContext *context)
    {
        auto systemModule = quickjs::object(context);

        systemModule.addFunction<delay>("delay");

        quickjs::object::getGlobal(context).addObject("system", systemModule);
    }

    void delay(size_t milliseconds)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
}