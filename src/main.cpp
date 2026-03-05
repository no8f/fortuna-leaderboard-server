#include <drogon/drogon.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <json/json.h>

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
    // ── Load and optionally patch config ─────────────────────────────────────
    std::ifstream configFile("./config.json");
    if (!configFile.is_open())
    {
        std::cerr << "[main] ERROR: Cannot open config.json\n";
        return 1;
    }

    Json::Value config;
    {
        Json::CharReaderBuilder rbuilder;
        std::string errs;
        if (!Json::parseFromStream(rbuilder, configFile, &config, &errs))
        {
            std::cerr << "[main] ERROR: Failed to parse config.json: " << errs << "\n";
            return 1;
        }
    }

    // ── Environment-variable overrides (LISTEN_ADDR / LISTEN_PORT) ───────────
    // Allows deployment-time customisation without editing config.json:
    //   LISTEN_ADDR=0.0.0.0 LISTEN_PORT=9090 ./fortuna-leaderboard-server
    static const char * const kDefaultAddr = "0.0.0.0";
    static const int           kDefaultPort = 8080;

    const char *envAddr = std::getenv("LISTEN_ADDR");
    const char *envPort = std::getenv("LISTEN_PORT");

    if (envAddr || envPort)
    {
        // Ensure the listeners array exists and has at least one entry to patch.
        if (!config.isMember("listeners") || !config["listeners"].isArray() ||
            config["listeners"].empty())
        {
            Json::Value listener;
            listener["address"] = kDefaultAddr;
            listener["port"]    = kDefaultPort;
            listener["https"]   = false;
            config["listeners"] = Json::Value(Json::arrayValue);
            config["listeners"].append(listener);
        }
        if (envAddr)
            config["listeners"][0]["address"] = envAddr;
        if (envPort)
        {
            try
            {
                config["listeners"][0]["port"] = std::stoi(envPort);
            }
            catch (const std::exception &)
            {
                std::cerr << "[main] WARNING: LISTEN_PORT value '" << envPort
                          << "' is not a valid integer – ignoring\n";
            }
        }
    }

    // ── Determine effective listen address/port for logging ───────────────────
    std::string listenAddr = kDefaultAddr;
    int         listenPort = kDefaultPort;
    if (config.isMember("listeners") && config["listeners"].isArray() &&
        !config["listeners"].empty())
    {
        const auto &l = config["listeners"][0];
        if (l.isMember("address") && l["address"].isString())
            listenAddr = l["address"].asString();
        if (l.isMember("port") && l["port"].isIntegral())
            listenPort = l["port"].asInt();
    }

    // ── Startup diagnostics ───────────────────────────────────────────────────
    std::cout << "[main] Starting Fortuna Leaderboard Server\n";
    std::cout << "[main] Listening on " << listenAddr << ":" << listenPort << "\n";
    std::cout << "[main] Document root : ./public/\n";
    std::cout << "[main] Health check  : http://" << listenAddr << ":" << listenPort << "/health\n";
    if (listenAddr != "0.0.0.0" && listenAddr != "::")
    {
        std::cerr << "[main] WARNING: Binding to a specific IP (" << listenAddr
                  << ") rather than 0.0.0.0 may prevent remote access.\n"
                  << "               Set LISTEN_ADDR=0.0.0.0 or update config.json to fix this.\n";
    }

    app().loadConfigJson(config);

    // Run schema init after framework is configured but before serving requests.
    app().registerBeginningAdvice([]() {
        initSchema();
    });

    app().run();
    return 0;
}
