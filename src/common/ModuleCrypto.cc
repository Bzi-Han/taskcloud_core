#include "ModuleCrypto.h"

namespace ModuleCrypto::Detail
{
    template <typename algorithm_t>
    constexpr std::string messageDigest(const std::string &data)
    {
        std::string result;
        algorithm_t algorithm;

        CryptoPP::StringSource(
            data,
            true,
            new CryptoPP::HashFilter(
                algorithm,
                new CryptoPP::HexEncoder(
                    new CryptoPP::StringSink(result))));

        return result;
    }

    template <typename algorithm_t>
    constexpr std::string symmetric(const std::string &data, const std::string &key, const std::string &iv, const std::string &padding)
    {
        std::string result;
        algorithm_t algorithm;

        auto blockPaddingScheme = CryptoPP::BlockPaddingSchemeDef::DEFAULT_PADDING;
        if (!padding.empty())
        {
            if ("PKCS" == padding)
                blockPaddingScheme = CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING;
            else if ("ZEROS" == padding)
                blockPaddingScheme = CryptoPP::BlockPaddingSchemeDef::ZEROS_PADDING;
            else if ("NO" == padding)
                blockPaddingScheme = CryptoPP::BlockPaddingSchemeDef::NO_PADDING;
            else if ("W3C" == padding)
                blockPaddingScheme = CryptoPP::BlockPaddingSchemeDef::W3C_PADDING;
            else
                throw std::runtime_error("Unknown padding scheme");
        }

        if (iv.empty())
            algorithm.SetKey((CryptoPP::byte *)key.data(), key.size());
        else
            algorithm.SetKeyWithIV((CryptoPP::byte *)key.data(), key.size(), (CryptoPP::byte *)iv.data(), iv.size());

        CryptoPP::StringSource(
            data,
            true,
            new CryptoPP::StreamTransformationFilter(
                algorithm,
                new CryptoPP::StringSink(result),
                blockPaddingScheme));

        return result;
    }
}

namespace ModuleCrypto
{
    void bind(lua_State *luaState)
    {
        luabridge::getGlobalNamespace(luaState)
            .beginNamespace("crypto")
            .addFunction("base64Encode", &base64Encode)
            .addFunction("base64Decode", &base64Decode)
            .addFunction("md5", &md5)
            .addFunction("sha1", &sha1)
            .addFunction("sha256", &sha256)
            .addFunction("sha512", &sha512)
            .addFunction("aesEncrypt", &aesEncrypt)
            .addFunction("aesDecrypt", &aesDecrypt)
            .addFunction("desEncrypt", &desEncrypt)
            .addFunction("desDecrypt", &desDecrypt)
            .endNamespace();
    }

    void bind(PyObject *mainModule)
    {
        auto module = pybind11::cast<pybind11::module>(mainModule);

        auto cryptoModule = module.def_submodule("crypto");
        cryptoModule.def("base64Encode", &base64Encode);
        cryptoModule.def("base64Decode", &base64Decode);
        cryptoModule.def("md5", &md5);
        cryptoModule.def("sha1", &sha1);
        cryptoModule.def("sha256", &sha256);
        cryptoModule.def("sha512", &sha512);
        cryptoModule.def("aesEncrypt", &aesEncrypt);
        cryptoModule.def("aesDecrypt", &aesDecrypt);
        cryptoModule.def("desEncrypt", &desEncrypt);
        cryptoModule.def("desDecrypt", &desDecrypt);
    }

    void bind(JSContext *context)
    {
        auto cryptoModule = quickjs::object(context);

        cryptoModule.addFunction<base64Encode>("base64Encode");
        cryptoModule.addFunction<base64Decode>("base64Decode");
        cryptoModule.addFunction<md5>("md5");
        cryptoModule.addFunction<sha1>("sha1");
        cryptoModule.addFunction<sha256>("sha256");
        cryptoModule.addFunction<sha512>("sha512");
        cryptoModule.addFunction<aesEncrypt>("aesEncrypt");
        cryptoModule.addFunction<aesDecrypt>("aesDecrypt");
        cryptoModule.addFunction<desEncrypt>("desEncrypt");
        cryptoModule.addFunction<desDecrypt>("desDecrypt");

        quickjs::object::getGlobal(context).addObject("crypto", cryptoModule);
    }

    std::string base64Encode(const std::string &data)
    {
        std::string result;

        CryptoPP::StringSource(
            data,
            true,
            new CryptoPP::Base64Encoder(
                new CryptoPP::StringSink(result)));

        return result;
    }

    std::string base64Decode(const std::string &data)
    {
        std::string result;

        CryptoPP::StringSource(
            data,
            true,
            new CryptoPP::Base64Decoder(
                new CryptoPP::StringSink(result)));

        return result;
    }

    std::string md5(const std::string &data)
    {
        return Detail::messageDigest<CryptoPP::Weak1::MD5>(data);
    }

    std::string sha1(const std::string &data)
    {
        return Detail::messageDigest<CryptoPP::SHA1>(data);
    }

    std::string sha256(const std::string &data)
    {
        return Detail::messageDigest<CryptoPP::SHA256>(data);
    }

    std::string sha512(const std::string &data)
    {
        return Detail::messageDigest<CryptoPP::SHA512>(data);
    }

    std::string aesEncrypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &mode, const std::string &padding)
    {
        std::string result;

        if ("ECB" == mode)
            result = Detail::symmetric<CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption>(data, key, "", padding);
        else if ("CBC" == mode)
            result = Detail::symmetric<CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption>(data, key, iv, padding);
        else if ("CFB" == mode)
            result = Detail::symmetric<CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption>(data, key, iv, padding);
        else if ("CTR" == mode)
            result = Detail::symmetric<CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption>(data, key, iv, padding);
        else if ("OFB" == mode)
            result = Detail::symmetric<CryptoPP::OFB_Mode<CryptoPP::AES>::Encryption>(data, key, iv, padding);
        else
            throw std::runtime_error("Unknown mode");

        return base64Encode(result);
    }

    std::string aesDecrypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &mode, const std::string &padding)
    {
        std::string result;

        if ("ECB" == mode)
            result = Detail::symmetric<CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption>(base64Decode(data), key, "", padding);
        else if ("CBC" == mode)
            result = Detail::symmetric<CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption>(base64Decode(data), key, iv, padding);
        else if ("CFB" == mode)
            result = Detail::symmetric<CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption>(base64Decode(data), key, iv, padding);
        else if ("CTR" == mode)
            result = Detail::symmetric<CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption>(base64Decode(data), key, iv, padding);
        else if ("OFB" == mode)
            result = Detail::symmetric<CryptoPP::OFB_Mode<CryptoPP::AES>::Decryption>(base64Decode(data), key, iv, padding);
        else
            throw std::runtime_error("Unknown mode");

        return result;
    }

    std::string desEncrypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &mode, const std::string &padding)
    {
        std::string result;

        if ("ECB" == mode)
            result = Detail::symmetric<CryptoPP::ECB_Mode<CryptoPP::DES>::Encryption>(data, key, "", padding);
        else if ("CBC" == mode)
            result = Detail::symmetric<CryptoPP::CBC_Mode<CryptoPP::DES>::Encryption>(data, key, iv, padding);
        else if ("CFB" == mode)
            result = Detail::symmetric<CryptoPP::CFB_Mode<CryptoPP::DES>::Encryption>(data, key, iv, padding);
        else if ("CTR" == mode)
            result = Detail::symmetric<CryptoPP::CTR_Mode<CryptoPP::DES>::Encryption>(data, key, iv, padding);
        else if ("OFB" == mode)
            result = Detail::symmetric<CryptoPP::OFB_Mode<CryptoPP::DES>::Encryption>(data, key, iv, padding);
        else
            throw std::runtime_error("Unknown mode");

        return base64Encode(result);
    }

    std::string desDecrypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &mode, const std::string &padding)
    {
        std::string result;

        if ("ECB" == mode)
            result = Detail::symmetric<CryptoPP::ECB_Mode<CryptoPP::DES>::Decryption>(base64Decode(data), key, "", padding);
        else if ("CBC" == mode)
            result = Detail::symmetric<CryptoPP::CBC_Mode<CryptoPP::DES>::Decryption>(base64Decode(data), key, iv, padding);
        else if ("CFB" == mode)
            result = Detail::symmetric<CryptoPP::CFB_Mode<CryptoPP::DES>::Decryption>(base64Decode(data), key, iv, padding);
        else if ("CTR" == mode)
            result = Detail::symmetric<CryptoPP::CTR_Mode<CryptoPP::DES>::Decryption>(base64Decode(data), key, iv, padding);
        else if ("OFB" == mode)
            result = Detail::symmetric<CryptoPP::OFB_Mode<CryptoPP::DES>::Decryption>(base64Decode(data), key, iv, padding);
        else
            throw std::runtime_error("Unknown mode");

        return result;
    }
}