function setTaskPassport(passport) {}

class tests {
    data = '这是一条测试消息';
    key = '9uFCkzsu3NMtL.jC';
    iv = 'pf7L-yDtb4-KW4Js';
    mode = 'CBC';
    padding = 'PKCS';

    jsonData = `
        {
            "retcode": 0,
            "message": "OK",
            "data": {
              "code": "Fxq6j",
              "risk_code": 375,
              "success": 1
            }
        }
    `;

    crypto() {
        logger.operation('------------------------------start test module: crypto------------------------------');

        logger.info('message', this.data);

        let result = crypto.utf8ToGBK(this.data);
        logger.info('crypto.utf8ToGBK', result);

        result = crypto.gbkToUTF8(result);
        logger.info('crypto.gbkToUTF8', result);
        
        result = crypto.urlEncode(this.data);
        logger.info('crypto.urlEncode', result);
        
        result = crypto.urlDecode(result);
        logger.info('crypto.urlDecode', result);
        
        result = crypto.base64Encode(this.data);
        logger.info('crypto.base64Encode', result);
        
        result = crypto.base64Decode(result);
        logger.info('crypto.base64Decode', result);
        
        result = crypto.md5(this.data);
        logger.info('crypto.md5', result);
        
        result = crypto.sha1(this.data);
        logger.info('crypto.sha1', result);
        
        result = crypto.sha256(this.data);
        logger.info('crypto.sha256', result);
        
        result = crypto.sha512(this.data);
        logger.info('crypto.sha512', result);
        
        result = crypto.aesEncrypt(this.data, this.key, this.iv, this.mode, this.padding);
        logger.info('crypto.aesEncrypt', result);
        
        result = crypto.aesDecrypt(result, this.key, this.iv, this.mode, this.padding);
        logger.info('crypto.aesDecrypt', result);
        
        result = crypto.desEncrypt(this.data, this.key.substring(8), this.iv.substring(8), this.mode, this.padding);
        logger.info('crypto.desEncrypt', result);
        
        result = crypto.desDecrypt(result, this.key.substring(8), this.iv.substring(8), this.mode, this.padding);
        logger.info('crypto.desDecrypt', result);
        
        const rsaKeyPair = crypto.rsaGenerateKeyPair(1024);
        logger.info('crypto.rsaGenerateKeyPair', '->');
        logger.succeed('publicKey', rsaKeyPair.publicKey);
        logger.succeed('privateKey', rsaKeyPair.privateKey);

        result = crypto.rsaEncrypt(rsaKeyPair.publicKey, this.data, true);
        logger.info('crypto.rsaEncrypt', result);

        result = crypto.rsaDecrypt(rsaKeyPair.privateKey, result, true);
        logger.info('crypto.rsaDecrypt', result);

        logger.succeed('==============================end test module: crypto==============================');
    }

    json() {
        logger.operation('------------------------------start test module: json------------------------------');

        logger.info('message', this.jsonData);

        const result = json.loads(this.jsonData);
        logger.info('json.loads', result, 'retcode:', result.retcode, 'message:', result.message, 'risk_code:', result.data.risk_code);

        logger.info('json.dumps', 'data:', json.dumps(result.data));

        logger.succeed('==============================end test module: json==============================');
    }

    requests() {
        logger.operation('------------------------------start test module: requests------------------------------');

        let url = 'https://httpbin.org/get'
        let result = requests.get(url);
        logger.info('requests.get', url, result.code, result.content);

        url = 'https://httpbin.org/post'
        result = requests.post(url, {a: 123, b: 'test'});
        logger.info('requests.post', url, result.code, result.content);

        url = 'https://httpbin.org/put'
        result = requests.put(url, 'a=123&b=test');
        logger.info('requests.put', url, result.code, result.content);

        url = 'https://httpbin.org/delete'
        result = requests.delete(url);
        logger.info('requests.delete', url, result.code, result.content);

        logger.succeed('==============================end test module: requests==============================');
    }

    tools() {
        logger.operation('------------------------------start test module: tools------------------------------');

        logger.info('message', this.data);

        logger.operation('logger.operation', this.data);
        logger.info('logger.info', this.data);
        logger.failed('logger.failed', this.data);
        logger.succeed('logger.succeed', this.data);

        logger.succeed('==============================end test module: tools==============================');
    }

    system() {
        logger.operation('------------------------------start test module: system------------------------------');

        logger.info('system.delay 1000ms');
        system.delay(1000);
        logger.info('system.delay end');

        logger.succeed('==============================end test module: system==============================');
    }

    start() {
        this.crypto();
        logger.failed();

        this.json();
        logger.failed();

        this.requests();
        logger.failed();

        this.tools();
        logger.failed();

        this.system();
        logger.failed();
    }
}

function main() {
    new tests().start();

    return true;
}