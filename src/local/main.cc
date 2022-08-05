#include "service.h"

#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <filesystem>

void help()
{
    std::cout << "Usage: local [options] script" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help              Show this help message and exit" << std::endl;
    std::cout << "  -l, --language          Set script language" << std::endl;
    std::cout << "  -p, --passport          Set script passport" << std::endl;
    std::cout << "  -c, --calls             Set the functions to be called. Multiple functions are separated by ','" << std::endl;
}

void parseArguments(int argc, char *argv[], std::string &language, std::string &passport, std::string &calls)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg(argv[i]);

        if ('-' != arg[0])
            break;

        if ("-h" == arg || "--help" == arg)
        {
            help();
            exit(0);
        }
        else if ("-l" == arg || "--language" == arg)
        {
            if (i + 1 < argc)
                language = argv[++i];
            else
            {
                std::cout << "Error: missing language" << std::endl;
                exit(1);
            }
        }
        else if ("-p" == arg || "--passport" == arg)
        {
            if (i + 1 < argc)
                passport = argv[++i];
            else
            {
                std::cout << "Error: missing passport" << std::endl;
                exit(1);
            }
        }
        else if ("-c" == arg || "--calls" == arg)
        {
            if (i + 1 < argc)
                calls = argv[++i];
            else
            {
                std::cout << "Error: missing calls" << std::endl;
                exit(1);
            }
        }
        else
        {
            std::cout << "Error: unknown argument " << arg << std::endl;
            exit(1);
        }
    }
}

Service::language_t checkLanguageType(const std::string_view &language)
{
    if ("lua" == language)
        return Service::language_t::lua;
    else if ("py" == language)
        return Service::language_t::python;
    else if ("js" == language)
        return Service::language_t::javascript;
    else
    {
        std::cout << "Error: unknown language " << language << std::endl;
        std::cout << "Available languages: lua, py, js" << std::endl;
        exit(1);
    }
}

int main(int argc, char **args)
{
    std::system("chcp 65001");

    // check arguments count
    if (2 > argc)
    {
        help();
        exit(1);
    }

    std::string language;
    std::string passport;
    std::string calls = "main";

    // parse arguments
    if ('-' == args[1][0])
        parseArguments(argc, args, language, passport, calls);

    // check language
    Service::language_t languageType = Service::language_t::lua;
    if (!language.empty())
        languageType = checkLanguageType(language);

    // check script path
    std::filesystem::path scriptPath(args[argc - 1]);
    if (!std::filesystem::exists(scriptPath))
    {
        std::cout << "Error: script " << scriptPath << " does not exist" << std::endl;
        exit(1);
    }

    // read script
    std::string script;
    std::ifstream scriptFile(scriptPath);
    if (!scriptFile.is_open())
    {
        std::cout << "Error: failed to open script " << scriptPath << std::endl;
        exit(1);
    }
    script.assign(std::istreambuf_iterator<char>(scriptFile), std::istreambuf_iterator<char>());
    scriptFile.close();

    // run script
    Service::run(
        0,
        0,
        0,
        languageType,
        "local",
        script,
        passport,
        calls);

    getchar();

    return 0;
}
