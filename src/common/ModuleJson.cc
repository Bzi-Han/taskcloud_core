#include "ModuleJson.h"

namespace ModuleJson::Detail
{
    JsonReader::JsonReader(lua_State *luaState)
        : m_readerType(ReaderType::lua),
          m_luaState(luaState)
    {
    }

    JsonReader::JsonReader(PyObject *mainModule)
        : m_readerType(ReaderType::python),
          m_mainModule(mainModule)
    {
    }

    JsonReader::JsonReader(JSContext *jsContext)
        : m_readerType(ReaderType::javascript),
          m_jsContext(jsContext)
    {
    }

    bool JsonReader::Default()
    {
        return false;
    }

    bool JsonReader::Null()
    {
        return value(nullptr);
    }

    bool JsonReader::Bool(bool v)
    {
        return value(v);
    }

    bool JsonReader::Int(int v)
    {
        return value(v);
    }

    bool JsonReader::Uint(unsigned v)
    {
        return value(v);
    }

    bool JsonReader::Int64(int64_t v)
    {
        return value(v);
    }

    bool JsonReader::Uint64(uint64_t v)
    {
        return value(v);
    }

    bool JsonReader::Double(double v)
    {
        return value(v);
    }

    bool JsonReader::RawNumber(const Ch *str, rapidjson::SizeType len, bool copy)
    {
        return value(std::string(str, len));
    }

    bool JsonReader::String(const Ch *str, rapidjson::SizeType len, bool copy)
    {
        return value(std::string(str, len));
    }

    bool JsonReader::StartObject()
    {
        return beginNode(false);
    }

    bool JsonReader::Key(const Ch *str, rapidjson::SizeType len, bool copy)
    {
        m_readerInfoStack.top().objectKey = {str, len};

        return true;
    }

    bool JsonReader::EndObject(rapidjson::SizeType)
    {
        return endNode();
    }

    bool JsonReader::StartArray()
    {
        return beginNode(true);
    }

    bool JsonReader::EndArray(rapidjson::SizeType)
    {
        return endNode();
    }

    bool JsonReader::beginNode(bool isArray)
    {
        m_readerInfoStack.push({isArray, 0, ""});
        switch (m_readerType)
        {
        case ReaderType::lua:
            m_luaReaderStack.emplace(m_luaState, luabridge::newTable(m_luaState));
            break;
        case ReaderType::python:
            if (isArray)
                m_pythonReaderStack.push(pybind11::list{});
            else
                m_pythonReaderStack.push(pybind11::dict{});
            break;
        case ReaderType::javascript:
            m_jsReaderStack.emplace(m_jsContext);
            break;
        }

        return true;
    }

    bool JsonReader::endNode()
    {
        switch (m_readerType)
        {
        case ReaderType::lua:
        {
            if (1 >= m_luaReaderStack.size())
            {
                m_readerInfoStack.pop();
                break;
            }

            auto reader = m_luaReaderStack.top();
            m_luaReaderStack.pop();
            m_readerInfoStack.pop();

            if (m_readerInfoStack.top().isArray)
                m_luaReaderStack.top().append(reader);
            else
                m_luaReaderStack.top()[m_readerInfoStack.top().objectKey] = reader;

            break;
        }
        case ReaderType::python:
        {
            if (1 >= m_pythonReaderStack.size())
            {
                m_readerInfoStack.pop();
                break;
            }

            auto reader = m_pythonReaderStack.top();
            m_pythonReaderStack.pop();
            m_readerInfoStack.pop();

            if (m_readerInfoStack.top().isArray)
                pybind11::cast<pybind11::list>(m_pythonReaderStack.top()).append(reader);
            else
                pybind11::cast<pybind11::dict>(m_pythonReaderStack.top())[m_readerInfoStack.top().objectKey.c_str()] = reader;

            break;
        }
        case ReaderType::javascript:
        {
            if (1 >= m_jsReaderStack.size())
            {
                m_readerInfoStack.pop();
                break;
            }

            auto reader = std::move(m_jsReaderStack.top());
            m_jsReaderStack.pop();
            m_readerInfoStack.pop();

            if (m_readerInfoStack.top().isArray)
                m_jsReaderStack.top().append(reader);
            else
                m_jsReaderStack.top().addObject(m_readerInfoStack.top().objectKey.c_str(), reader);

            break;
        }
        }

        return true;
    }

    template <typename any_t>
    bool JsonReader::value(any_t value)
    {
        switch (m_readerType)
        {
        case ReaderType::lua:
            if (m_readerInfoStack.top().isArray)
                m_luaReaderStack.top()[++m_readerInfoStack.top().arraySize] = value;
            else
            {
                if constexpr (std::is_same_v<any_t, std::nullptr_t>)
                    m_luaReaderStack.top()[m_readerInfoStack.top().objectKey] = luabridge::Nil();
                else
                    m_luaReaderStack.top()[m_readerInfoStack.top().objectKey] = value;
            }
            break;
        case ReaderType::python:
            if (m_readerInfoStack.top().isArray)
                pybind11::cast<pybind11::list>(m_pythonReaderStack.top())[++m_readerInfoStack.top().arraySize] = value;
            else
                pybind11::cast<pybind11::dict>(m_pythonReaderStack.top())[m_readerInfoStack.top().objectKey.c_str()] = value;
            break;
        case ReaderType::javascript:
            if (m_readerInfoStack.top().isArray)
                m_jsReaderStack.top().setProperty(++m_readerInfoStack.top().arraySize, value);
            else
                m_jsReaderStack.top().setProperty(m_readerInfoStack.top().objectKey.c_str(), value);
            break;
        }

        return true;
    }
}

namespace ModuleJson::Bindings
{
    luabridge::LuaRef lua_loads(const luabridge::LuaRef &data)
    {
        if (!data.isString())
            luaL_error(data.state(), "json.loads: expected string");

        Detail::JsonReader reader(data.state());
        rapidjson::StringStream dataStream(data.cast<const char *>());

        rapidjson::Reader{}.Parse(dataStream, reader);

        return reader;
    }

    pybind11::object python_loads(const std::string &data)
    {
        Detail::JsonReader reader(PyEval_GetGlobals());
        rapidjson::StringStream dataStream(data.data());

        rapidjson::Reader{}.Parse(dataStream, reader);

        return reader;
    }

    JSValue javascript_loads(const quickjs::value<std::string> &data)
    {
        Detail::JsonReader reader(data.context);
        rapidjson::StringStream dataStream(data.value.data());

        rapidjson::Reader{}.Parse(dataStream, reader);

        return reader;
    }
}

namespace ModuleJson
{
    void bind(lua_State *luaState)
    {
        luabridge::getGlobalNamespace(luaState)
            .beginNamespace("json")
            .addFunction("loads", &Bindings::lua_loads)
            .endNamespace();
    }

    void bind(PyObject *mainModule)
    {
        auto module = pybind11::cast<pybind11::module>(mainModule);

        auto jsonModule = module.def_submodule("json");
        jsonModule.def("loads", &Bindings::python_loads);
    }

    void bind(JSContext *context)
    {
        auto jsonModule = quickjs::object(context);

        jsonModule.addFunction<Bindings::javascript_loads>("loads");

        quickjs::object::getGlobal(context).addObject("json", jsonModule);
    }

    rapidjson::Document loads(const std::string &data)
    {
        return std::move(rapidjson::Document{}.Parse(data.c_str()));
    }
}