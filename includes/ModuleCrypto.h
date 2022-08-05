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
#include <cryptopp/rsa.h>
#include <cryptopp/cpu.h>
#include <cryptopp/randpool.h>
#include <cryptopp/rdrand.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

#include <windows.h>

#define PLATFORM_WINDOWS

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)

#include <iconv.h>

#define PLATFORM_LINUX

#endif

#include <string>
#include <string_view>
#include <sstream>

namespace ModuleCrypto
{
    namespace Detail
    {
        bool isBase64String(const std::string &data);

        std::string rsaBase64KeyNormalize(const std::string_view &data);

        template <typename algorithm_t>
        std::string messageDigest(const std::string &data);

        template <typename algorithm_t>
        std::string symmetric(const std::string &data, const std::string &key, const std::string &iv, const std::string &padding);

        template <bool is_encrypt, typename algorithm_t, typename decoder_t, typename encoder_t = CryptoPP::Base64Encoder>
        std::string asymmetric(const std::string &data, const std::string &key);
    }

    namespace Bindings
    {
        luabridge::LuaRef luaRsaGenerateKeyPair(lua_State *luaState);

        pybind11::dict pyRsaGenerateKeyPair(pybind11::args args);

        JSValue jsRsaGenerateKeyPair(quickjs::args args);
    }

    void bind(lua_State *luaState);

    void bind(PyObject *mainModule);

    void bind(JSContext *context);

    std::string gbkToUTF8(const std::string &gbk);
    std::string utf8ToGBK(const std::string &utf8);

    std::string urlEncode(const std::string &data);
    std::string urlDecode(const std::string &data);

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

    /**
     * @name rsaGenerateKeyPair
     * @brief generate a key pair
     *
     * @param keySize key size, default is 1024.
     * @param hex whether to use hexadecimal encoding, the default is true. otherwise, use base64 encoding.
     * @param seed user input seed, default is empty.
     *
     * @return KeyPair<PublicKey，PrivateKey>
     */
    std::pair<std::string, std::string> rsaGenerateKeyPair(size_t keySize = 1024, bool hex = true, std::string seed = "");

    std::string rsaEncrypt(const std::string &data, const std::string &publicKey, bool oaep = true);
    std::string rsaDecrypt(const std::string &data, const std::string &privateKey, bool oaep = true);
}

#endif // !MODULE_CRYPTO_H