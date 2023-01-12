#include "ModuleCrypto.h"

namespace ModuleCrypto::Detail
{
    bool isBase64String(const std::string &data)
    {
        return std::string::npos != data.find('=') || std::string::npos != data.find('+') || std::string::npos != data.find('/');
    };

    std::string rsaBase64KeyNormalize(const std::string_view &data)
    {
        std::stringstream result;
        std::string_view key = data;

        auto startPos = data.find("KEY-----");
        auto endPos = startPos;

        if (std::string::npos != startPos)
        {
            startPos += 8;
            endPos = data.rfind("-----END");

            if (std::string::npos == endPos)
                throw std::runtime_error("invalid rsa key");

            key = data.substr(startPos, endPos - startPos);
        }

        for (auto c : key)
        {
            if ('\n' == c || '\r' == c || ' ' == c || '\t' == c)
                continue;

            result << c;
        }

        return result.str();
    }

    template <typename algorithm_t>
    std::string messageDigest(const std::string &data)
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
    std::string symmetric(const std::string &data, const std::string &key, const std::string &iv, const std::string &padding)
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

    template <bool is_encrypt, typename algorithm_t, typename decoder_t, typename encoder_t>
    std::string asymmetric(const std::string &data, const std::string &key)
    {
        std::string result;
        CryptoPP::RandomPool randomPool;

        if (CryptoPP::HasRDSEED())
        {
            CryptoPP::byte seed[8]{};
            CryptoPP::RDSEED rdseed;

            rdseed.GenerateBlock(seed, 8);
            randomPool.IncorporateEntropy(seed, sizeof(seed));
        }

        CryptoPP::StringSource rsaKey(key, true, new decoder_t);
        if constexpr (is_encrypt)
        {
            algorithm_t encryptor(rsaKey);

            CryptoPP::StringSource(
                data,
                true,
                new CryptoPP::PK_EncryptorFilter(
                    randomPool,
                    encryptor,
                    new encoder_t(
                        new CryptoPP::StringSink(result))));
        }
        else
        {
            algorithm_t decryptor(rsaKey);

            CryptoPP::StringSource(
                data,
                true,
                new decoder_t(
                    new CryptoPP::PK_DecryptorFilter(
                        randomPool,
                        decryptor,
                        new CryptoPP::StringSink(result))));
        }

        return result;
    }
}

namespace ModuleCrypto::Bindings
{
    luabridge::LuaRef luaRsaGenerateKeyPair(lua_State *luaState)
    {
        auto paramsCount = lua_gettop(luaState);

        size_t keySize = 1024;
        bool hex = true;
        std::string seed;

        if (1 <= paramsCount)
        {
            keySize = luaL_checkinteger(luaState, 1);
            if (2 <= paramsCount)
                hex = lua_toboolean(luaState, 2);
            if (3 <= paramsCount)
                seed = luaL_checkstring(luaState, 3);
        }

        auto keyPair = rsaGenerateKeyPair(keySize, hex, seed);
        auto result = luabridge::LuaRef::newTable(luaState);

        result["publicKey"] = keyPair.first;
        result["privateKey"] = keyPair.second;

        return result;
    }

    pybind11::dict pyRsaGenerateKeyPair(pybind11::args args)
    {
        size_t keySize = 1024;
        bool hex = true;
        std::string seed;

        if (1 <= args.size())
        {
            keySize = args[0].cast<size_t>();
            if (2 <= args.size())
                hex = args[1].cast<bool>();
            if (3 <= args.size())
                seed = args[2].cast<std::string>();
        }

        auto keyPair = rsaGenerateKeyPair(keySize, hex, seed);
        pybind11::dict result;

        result["publicKey"] = keyPair.first;
        result["privateKey"] = keyPair.second;

        return result;
    }

    JSValue jsRsaGenerateKeyPair(quickjs::args args)
    {
        size_t keySize = 1024;
        bool hex = true;
        std::string seed;

        if (1 <= args.size())
        {
            keySize = args[0].cast<size_t>();
            if (2 <= args.size())
                hex = args[1].cast<bool>();
            if (3 <= args.size())
                seed = args[2].cast<std::string>();
        }

        auto keyPair = rsaGenerateKeyPair(keySize, hex, seed);
        quickjs::object result(args);

        result.setProperty("publicKey", keyPair.first);
        result.setProperty("privateKey", keyPair.second);

        return result;
    }
}

namespace ModuleCrypto
{
    void bind(lua_State *luaState)
    {
        luabridge::getGlobalNamespace(luaState)
            .beginNamespace("crypto")
            .addFunction("gbkToUTF8", &gbkToUTF8)
            .addFunction("utf8ToGBK", &utf8ToGBK)
            .addFunction("urlEncode", &urlEncode)
            .addFunction("urlDecode", &urlDecode)
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
            .addFunction("rsaGenerateKeyPair", &Bindings::luaRsaGenerateKeyPair)
            .addFunction("rsaEncrypt", &rsaEncrypt)
            .addFunction("rsaDecrypt", &rsaDecrypt)
            .endNamespace();
    }

    void bind(PyObject *mainModule)
    {
        auto module = pybind11::cast<pybind11::module>(mainModule);

        auto cryptoModule = module.def_submodule("crypto");
        cryptoModule.def("gbkToUTF8", &gbkToUTF8);
        cryptoModule.def("utf8ToGBK", &utf8ToGBK);
        cryptoModule.def("urlEncode", &urlEncode);
        cryptoModule.def("urlDecode", &urlDecode);
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
        cryptoModule.def("rsaGenerateKeyPair", &Bindings::pyRsaGenerateKeyPair);
        cryptoModule.def("rsaEncrypt", &rsaEncrypt);
        cryptoModule.def("rsaDecrypt", &rsaDecrypt);
    }

    void bind(JSContext *context)
    {
        auto cryptoModule = quickjs::object(context);

        cryptoModule.addFunction<gbkToUTF8>("gbkToUTF8");
        cryptoModule.addFunction<utf8ToGBK>("utf8ToGBK");
        cryptoModule.addFunction<urlEncode>("urlEncode");
        cryptoModule.addFunction<urlDecode>("urlDecode");
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
        cryptoModule.addFunction<Bindings::jsRsaGenerateKeyPair>("rsaGenerateKeyPair");
        cryptoModule.addFunction<rsaEncrypt>("rsaEncrypt");
        cryptoModule.addFunction<rsaDecrypt>("rsaDecrypt");

        quickjs::object::getGlobal(context).addObject("crypto", cryptoModule);
    }

    std::string gbkToUTF8(const std::string &gbk)
    {
#if defined(PLATFORM_WINDOWS)

        int convertLength = MultiByteToWideChar(CP_ACP, 0, gbk.data(), -1, nullptr, 0);
        wchar_t *convertStringBuffer = new wchar_t[convertLength]{};
        MultiByteToWideChar(CP_ACP, 0, gbk.data(), static_cast<int>(gbk.size()), convertStringBuffer, convertLength);

        convertLength = WideCharToMultiByte(CP_UTF8, 0, convertStringBuffer, convertLength, nullptr, 0, nullptr, nullptr);
        std::string result(convertLength, 0);
        WideCharToMultiByte(CP_UTF8, 0, convertStringBuffer, -1, result.data(), convertLength, nullptr, nullptr);
        delete[] convertStringBuffer;

#elif defined(PLATFORM_LINUX)

        std::string result;
        iconv_t cd = iconv_open("UTF-8", "GBK");

        if ((iconv_t)-1 == cd)
            return {};

        size_t inSize = gbk.size();
        size_t outSize = 4 * inSize;
        char *inBuffer = const_cast<char *>(gbk.data());
        result.resize(outSize);
        char *outBuffer = const_cast<char *>(result.data());

        size_t status = iconv(cd, &inBuffer, &inSize, &outBuffer, &outSize);
        if ((size_t)-1 == status)
            return {};

        result.resize(outSize);
        iconv_close(cd);

#endif

        return result;
    }

    std::string utf8ToGBK(const std::string &utf8)
    {
#if defined(PLATFORM_WINDOWS)

        int convertLength = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), -1, nullptr, 0);
        wchar_t *convertStringBuffer = new wchar_t[convertLength]{};
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), convertStringBuffer, convertLength);

        convertLength = WideCharToMultiByte(CP_ACP, 0, convertStringBuffer, convertLength, nullptr, 0, nullptr, nullptr);
        std::string result(convertLength, 0);
        WideCharToMultiByte(CP_ACP, 0, convertStringBuffer, -1, result.data(), convertLength, nullptr, nullptr);
        delete[] convertStringBuffer;

#elif defined(PLATFORM_LINUX)

        std::string result;
        iconv_t cd = iconv_open("GBK", "UTF-8");

        if ((iconv_t)-1 == cd)
            return {};

        size_t inSize = utf8.size();
        size_t outSize = 4 * inSize;
        char *inBuffer = const_cast<char *>(utf8.data());
        result.resize(outSize);
        char *outBuffer = const_cast<char *>(result.data());

        size_t status = iconv(cd, &inBuffer, &inSize, &outBuffer, &outSize);
        if ((size_t)-1 == status)
            return {};

        result.resize(outSize);
        iconv_close(cd);

#endif

        return result;
    }

    std::string urlEncode(const std::string &data)
    {
        std::stringstream result;

        for (const auto c : data)
        {
            if (('0' <= c && '9' >= c) || ('a' <= c && 'z' >= c) || ('A' <= c && 'Z' >= c) || '-' == c || '_' == c || '.' == c || '~' == c)
            {
                result << c;

                continue;
            }

            switch (c)
            {
            case ' ':
                result << '+';

                break;
            default:
            {
                unsigned char l = c, h = c;
                l = l >> 4;
                h = h % 16;

                result << '%'
                       << (char)(l > 9 ? l + 55 : l + 48)
                       << (char)(h > 9 ? h + 55 : h + 48);

                break;
            }
            }
        }

        return result.str();
    }

    std::string urlDecode(const std::string &data)
    {
        std::stringstream result;

        for (auto it = data.begin(); it != data.end(); it++)
        {
            if (('0' <= *it && '9' >= *it) || ('a' <= *it && 'z' >= *it) || ('A' <= *it && 'Z' >= *it) || '-' == *it || '_' == *it || '.' == *it || '~' == *it)
            {
                result << *it;

                continue;
            }

            switch (*it)
            {
            case '+':
                result << ' ';

                break;
            case '%':
            {
                unsigned char sum = 0;

                for (size_t i = 0; i < 2; i++)
                {
                    ++it;

                    if (*it >= '0' && *it <= '9')
                    {
                        if (0 == i)
                            sum += (unsigned char)(*it - '0') * 16;
                        else
                            sum += (unsigned char)(*it - '0');
                    }
                    else if (*it >= 'A' && *it <= 'F')
                    {
                        if (0 == i)
                            sum += ((unsigned char)(*it - 'A') + 10) * 16;
                        else
                            sum += (unsigned char)(*it - 'A') + 10;
                    }
                    else if (*it >= 'a' && *it <= 'f')
                    {
                        if (0 == i)
                            sum += ((unsigned char)(*it - 'a') + 10) * 16;
                        else
                            sum += (unsigned char)(*it - 'a') + 10;
                    }
                }

                result << sum;

                break;
            }
            }
        }

        return result.str();
    }

    std::string base64Encode(const std::string &data)
    {
        std::string result;

        CryptoPP::StringSource(
            data,
            true,
            new CryptoPP::Base64Encoder(
                new CryptoPP::StringSink(result)));

        return '\n' == result[result.length() - 1] ? result.substr(0, result.length() - 1) : result;
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

    std::pair<std::string, std::string> rsaGenerateKeyPair(size_t keySize, bool hex, std::string seed)
    {
        std::string publicKey, privateKey;
        CryptoPP::RandomPool randomPool;

        if (!seed.empty())
            randomPool.IncorporateEntropy(reinterpret_cast<const CryptoPP::byte *>(seed.data()), seed.size());

        if (hex)
        {
            CryptoPP::RSAES_OAEP_SHA_Decryptor decryptor(randomPool, keySize);
            CryptoPP::HexEncoder privateEncoder(new CryptoPP::StringSink(privateKey));

            decryptor.AccessMaterial().Save(privateEncoder);
            privateEncoder.MessageEnd();

            CryptoPP::RSAES_OAEP_SHA_Encryptor encryptor(decryptor);
            CryptoPP::HexEncoder publicEncoder(new CryptoPP::StringSink(publicKey));

            encryptor.AccessMaterial().Save(publicEncoder);
            publicEncoder.MessageEnd();
        }
        else
        {
            CryptoPP::RSAES_OAEP_SHA_Decryptor decryptor(randomPool, keySize);
            CryptoPP::Base64Encoder privateEncoder(new CryptoPP::StringSink(privateKey));

            decryptor.AccessMaterial().Save(privateEncoder);
            privateEncoder.MessageEnd();

            CryptoPP::RSAES_OAEP_SHA_Encryptor encryptor(decryptor);
            CryptoPP::Base64Encoder publicEncoder(new CryptoPP::StringSink(publicKey));

            encryptor.AccessMaterial().Save(publicEncoder);
            publicEncoder.MessageEnd();
        }

        return std::make_pair(publicKey, privateKey);
    }

    std::string rsaEncrypt(const std::string &publicKey, const std::string &data, bool oaep)
    {
        bool isBase64 = Detail::isBase64String(publicKey);

        if (oaep)
        {
            if (isBase64)
                return Detail::asymmetric<true, CryptoPP::RSAES_OAEP_SHA_Encryptor, CryptoPP::Base64Decoder>(data, Detail::rsaBase64KeyNormalize(publicKey));
            else
                return Detail::asymmetric<true, CryptoPP::RSAES_OAEP_SHA_Encryptor, CryptoPP::HexDecoder, CryptoPP::HexEncoder>(data, publicKey);
        }
        else
        {
            if (isBase64)
                return Detail::asymmetric<true, CryptoPP::RSAES_PKCS1v15_Encryptor, CryptoPP::Base64Decoder>(data, Detail::rsaBase64KeyNormalize(publicKey));
            else
                return Detail::asymmetric<true, CryptoPP::RSAES_PKCS1v15_Encryptor, CryptoPP::HexDecoder, CryptoPP::HexEncoder>(data, publicKey);
        }
    }

    std::string rsaDecrypt(const std::string &privateKey, const std::string &data, bool oaep)
    {
        bool isBase64 = Detail::isBase64String(privateKey);

        if (oaep)
        {
            if (isBase64)
                return Detail::asymmetric<false, CryptoPP::RSAES_OAEP_SHA_Decryptor, CryptoPP::Base64Decoder>(data, Detail::rsaBase64KeyNormalize(privateKey));
            else
                return Detail::asymmetric<false, CryptoPP::RSAES_OAEP_SHA_Decryptor, CryptoPP::HexDecoder>(data, privateKey);
        }
        else
        {
            if (isBase64)
                return Detail::asymmetric<false, CryptoPP::RSAES_PKCS1v15_Decryptor, CryptoPP::Base64Decoder>(data, Detail::rsaBase64KeyNormalize(privateKey));
            else
                return Detail::asymmetric<false, CryptoPP::RSAES_PKCS1v15_Decryptor, CryptoPP::HexDecoder>(data, privateKey);
        }
    }
}