#include <drogon/drogon.h>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace drogon;

// Read sql/schema.sql and execute each statement against the named DB client.
static void initSchema()
{
    std::ifstream f("./sql/schema.sql");
    if (!f.is_open())
    {
        std::cerr << "[main] WARNING: Cannot open sql/schema.sql - skipping schema init\n";
        return;
    }
    std::ostringstream buf;
    buf << f.rdbuf();
    std::string fullSql = buf.str();

    auto db = app().getDbClient("leaderboard");
    if (!db)
    {
        std::cerr << "[main] ERROR: Cannot get DB client 'leaderboard' for schema init\n";
        return;
    }

    // Split on ';' and execute each non-empty statement synchronously.
    std::istringstream ss(fullSql);
    std::string stmt;
    while (std::getline(ss, stmt, ';'))
    {
        // Trim whitespace
        auto start = stmt.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        stmt = stmt.substr(start);
        if (stmt.empty()) continue;

        try
        {
            // execSqlSync is available in Drogon's DbClient for synchronous use.
            db->execSqlSync(stmt);
        }
        catch (const std::exception &e)
        {
            std::cerr << "[main] Schema stmt error: " << e.what()
                      << "\n  stmt: " << stmt << '\n';
        }
    }
    std::cout << "[main] Schema initialized.\n";
}

int main()
{
    app().loadConfigFile("./config.json");

    // Run schema init after framework is configured but before serving requests.
    app().registerBeginningAdvice([]() {
        initSchema();
    });

    app().run();
    return 0;
}
