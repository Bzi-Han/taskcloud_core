function setTaskPassport(passport) end

local tests = {
    data = '这是一条测试消息',
    key = '9uFCkzsu3NMtL.jC',
    iv = 'pf7L-yDtb4-KW4Js',
    mode = 'CBC',
    padding = 'PKCS',

    jsonData = [[
        {
            "retcode": 0,
            "message": "OK",
            "data": {
              "code": "Fxq6j",
              "risk_code": 375,
              "success": 1
            }
        }
    ]],
}

function tests:crypto()
    logger.operation('------------------------------start test module: crypto------------------------------')

    logger.info('message', self.data)

    local result = crypto.utf8ToGBK(self.data)
    logger.info('crypto.utf8ToGBK', result)

    result = crypto.gbkToUTF8(result)
    logger.info('crypto.gbkToUTF8', result)
    
    result = crypto.urlEncode(self.data)
    logger.info('crypto.urlEncode', result)
    
    result = crypto.urlDecode(result)
    logger.info('crypto.urlDecode', result)
    
    result = crypto.base64Encode(self.data)
    logger.info('crypto.base64Encode', result)
    
    result = crypto.base64Decode(result)
    logger.info('crypto.base64Decode', result)
    
    result = crypto.md5(self.data)
    logger.info('crypto.md5', result)
    
    result = crypto.sha1(self.data)
    logger.info('crypto.sha1', result)
    
    result = crypto.sha256(self.data)
    logger.info('crypto.sha256', result)
    
    result = crypto.sha512(self.data)
    logger.info('crypto.sha512', result)
    
    result = crypto.aesEncrypt(self.data, self.key, self.iv, self.mode, self.padding)
    logger.info('crypto.aesEncrypt', result)
    
    result = crypto.aesDecrypt(result, self.key, self.iv, self.mode, self.padding)
    logger.info('crypto.aesDecrypt', result)
    
    result = crypto.desEncrypt(self.data, string.sub(self.key, 9), string.sub(self.iv, 9), self.mode, self.padding)
    logger.info('crypto.desEncrypt', result)
    
    result = crypto.desDecrypt(result, string.sub(self.key, 9), string.sub(self.iv, 9), self.mode, self.padding)
    logger.info('crypto.desDecrypt', result)
    
    local rsaKeyPair = crypto.rsaGenerateKeyPair(1024)
    logger.info('crypto.rsaGenerateKeyPair', '->')
    logger.succeed('publicKey', rsaKeyPair.publicKey)
    logger.succeed('privateKey', rsaKeyPair.privateKey)

    result = crypto.rsaEncrypt(rsaKeyPair.publicKey, self.data, true)
    logger.info('crypto.rsaEncrypt', result)

    result = crypto.rsaDecrypt(rsaKeyPair.privateKey, result, true)
    logger.info('crypto.rsaDecrypt', result)

    logger.succeed('==============================end test module: crypto==============================')
end

function tests:json()
    logger.operation('------------------------------start test module: json------------------------------')

    logger.info('message', self.jsonData)

    local result = json.loads(self.jsonData)
    logger.info('json.loads', result, 'retcode:', result.retcode, 'message:', result.message, 'risk_code:', result.data.risk_code)

    logger.info('json.dumps', 'data:', json.dumps(result.data))

    logger.succeed('==============================end test module: json==============================')
end

function tests:requests()
    logger.operation('------------------------------start test module: requests------------------------------')

    local url = 'https://httpbin.org/get'
    local result = requests.get(url)
    logger.info('requests.get', url, result.code, result.content)

    url = 'https://httpbin.org/post'
    result = requests.post(url, {a = 123, b = 'test'})
    logger.info('requests.post', url, result.code, result.content)

    url = 'https://httpbin.org/put'
    result = requests.put(url, 'a=123&b=test')
    logger.info('requests.put', url, result.code, result.content)

    url = 'https://httpbin.org/delete'
    result = requests.delete(url)
    logger.info('requests.delete', url, result.code, result.content)

    logger.succeed('==============================end test module: requests==============================')
end

function tests:tools()
    logger.operation('------------------------------start test module: tools------------------------------')

    logger.info('message', self.data)

    logger.operation('logger.operation', self.data)
    logger.info('logger.info', self.data)
    logger.failed('logger.failed', self.data)
    logger.succeed('logger.succeed', self.data)

    logger.succeed('==============================end test module: tools==============================')
end

function tests:system()
    logger.operation('------------------------------start test module: system------------------------------')

    logger.info('system.delay 1000ms')
    system.delay(1000)
    logger.info('system.delay end')

    logger.succeed('==============================end test module: system==============================')
end

function tests:start()
    tests:crypto()
    logger.failed()

    tests:json()
    logger.failed()

    tests:requests()
    logger.failed()

    tests:tools()
    logger.failed()

    tests:system()
    logger.failed()
end

function main()
    tests:start()

    return true
end