function setTaskPassport(passport) end

function main()
    logger.operation('******************************start test Lua******************************')
    os.execute('local ../../tests/test.lua')
    logger.succeed('\n')

    logger.operation('******************************start test Python******************************')
    os.execute('local ../../tests/test.py')
    logger.succeed('\n')

    logger.operation('******************************start test Javascript******************************')
    os.execute('local ../../tests/test.js')
    logger.succeed('\n')

    return true
end