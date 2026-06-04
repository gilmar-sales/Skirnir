#include <iostream>
#include <ranges>

#include <Skirnir/Configuration.hpp>

struct DatabaseOptions
{
    std::string host    = "localhost";
    int         port    = 5432;
    bool        ssl     = false;
    double      timeout = 30.0;
};

int main()
{
    const char* json = R"({
        "Database": {
            "host": "db.example.com",
            "port": 3306,
            "ssl": true,
            "timeout": 5.5
        }
    })";

    auto config = skr::ConfigurationBuilder().AddJsonString(json).Build();
    auto db     = config->Bind<DatabaseOptions>("Database");

    std::cout << "host=" << db.host << "\n";
    std::cout << "port=" << db.port << "\n";
    std::cout << "ssl=" << std::boolalpha << db.ssl << "\n";
    std::cout << "timeout=" << db.timeout << "\n";
    return 0;
}
