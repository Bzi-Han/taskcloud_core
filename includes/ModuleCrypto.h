#ifndef MODULE_CRYPTO_H // !MODULE_CRYPTO_H
#define MODULE_CRYPTO_H

#include <luajit/src/lua.hpp>
#include <luabridge/Source/LuaBridge/LuaBridge.h>
#include <Python.h>
#include <pybind11/include/pybind11/pybind11.h>
#include <quickjs-cmake/quickjs/quickjs.h>
#include <quickjsbind.h>
#include <cryptopp/hex.h>
#include <cryptopp/base64.h>
#include <cryptopp/md5.h>
#include <cryptopp/sha.h>
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/des.h>

#include <string>

namespace ModuleCrypto
{
    namespace Detail
    {
        template <typename algorithm_t>
        std::string messageDigest(const std::string &data);

        template <typename algorithm_t>
        std::string symmetric(const std::string &data, const std::string &key, const std::string &iv, const std::string &padding);
    }

    void bind(lua_State *luaState);

    void bind(PyObject *mainModule);

    void bind(JSContext *context);

    std::string base64Encode(const std::string &data);
    std::string base64Decode(const std::string &data);

    std::string md5(const std::string &data);
    std::string sha1(const std::string &data);
    std::string sha256(const std::string &data);
    std::string sha512(const std::string &data);

    std::string aesEncrypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &mode, const std::string &padding);
    std::string aesDecrypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &mode, const std::string &padding);

    std::string desEncrypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &mode, const std::string &padding);
    std::string desDecrypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &mode, const std::string &padding);
}

#endif // !MODULE_CRYPTO_H