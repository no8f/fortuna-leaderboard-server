#include "ScoresController.h"

#include <drogon/orm/DbClient.h>
#include <json/json.h>
#include <regex>
#include <fstream>
#include <sstream>

using namespace drogon;
using namespace drogon::orm;

// ── helpers ──────────────────────────────────────────────────────────────────

static HttpResponsePtr jsonResp(int status, const Json::Value &body)
{
    auto resp = HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(static_cast<HttpStatusCode>(status));
    return resp;
}

static HttpResponsePtr errorResp(int status, const std::string &msg)
{
    Json::Value body;
    body["ok"]    = false;
    body["error"] = msg;
    return jsonResp(status, body);
}

static const std::regex kNameRegex{"^[A-Za-z0-9_-]+$"};

// ── POST /api/v1/scores ──────────────────────────────────────────────────────

void ScoresController::postScore(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr)
    {
        callback(errorResp(400, "Invalid JSON body"));
        return;
    }
    const Json::Value &body = *jsonPtr;

    // Validate name
    if (!body.isMember("name") || !body["name"].isString())
    {
        callback(errorResp(400, "Field 'name' is required and must be a string"));
        return;
    }
    std::string name = body["name"].asString();
    if (name.empty() || name.size() > 15)
    {
        callback(errorResp(400, "Field 'name' must be 1–15 characters"));
        return;
    }
    if (!std::regex_match(name, kNameRegex))
    {
        callback(errorResp(400, "Field 'name' contains invalid characters (allowed: A-Z a-z 0-9 _ -)"));
        return;
    }

    // Validate score
    if (!body.isMember("score") || !body["score"].isInt())
    {
        callback(errorResp(400, "Field 'score' is required and must be an integer"));
        return;
    }
    int score = body["score"].asInt();
    if (score < 0)
    {
        callback(errorResp(400, "Field 'score' must be >= 0"));
        return;
    }

    // Validate sail_color
    if (!body.isMember("sail_color") || !body["sail_color"].isInt())
    {
        callback(errorResp(400, "Field 'sail_color' is required and must be an integer"));
        return;
    }
    int sailColor = body["sail_color"].asInt();

    // Async insert
    auto db = app().getDbClient("leaderboard");
    if (!db)
    {
        callback(errorResp(500, "Database unavailable"));
        return;
    }

    db->execSqlAsync(
        "INSERT INTO scores (name, score, sail_color) VALUES (?, ?, ?)",
        [callback](const Result &)
        {
            Json::Value ok;
            ok["ok"] = true;
            callback(jsonResp(200, ok));
        },
        [callback](const DrogonDbException &e)
        {
            LOG_ERROR << "DB insert error: " << e.base().what();
            callback(errorResp(500, "Database error"));
        },
        name, score, sailColor);
}

// ── GET /api/v1/scores ───────────────────────────────────────────────────────

void ScoresController::getScores(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    int limit = 10;
    auto limitParam = req->getOptionalParameter<int>("limit");
    if (limitParam)
    {
        limit = *limitParam;
        if (limit < 1)   limit = 1;
        if (limit > 100) limit = 100;
    }

    auto db = app().getDbClient("leaderboard");
    if (!db)
    {
        callback(errorResp(500, "Database unavailable"));
        return;
    }

    db->execSqlAsync(
        "SELECT name, score, sail_color FROM scores ORDER BY score DESC, id ASC LIMIT ?",
        [callback](const Result &r)
        {
            Json::Value scores(Json::arrayValue);
            for (const auto &row : r)
            {
                Json::Value entry;
                entry["name"]       = row["name"].as<std::string>();
                entry["score"]      = row["score"].as<int>();
                entry["sail_color"] = row["sail_color"].as<int>();
                scores.append(entry);
            }
            Json::Value resp;
            resp["scores"] = scores;
            callback(jsonResp(200, resp));
        },
        [callback](const DrogonDbException &e)
        {
            LOG_ERROR << "DB query error: " << e.base().what();
            callback(errorResp(500, "Database error"));
        },
        limit);
}

// ── GET / ────────────────────────────────────────────────────────────────────

void ScoresController::getIndex(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Serve public/index.html relative to the working directory.
    auto resp = HttpResponse::newFileResponse("./public/index.html",
                                              "",
                                              CT_TEXT_HTML);
    callback(resp);
}

// ── GET /scores.txt ──────────────────────────────────────────────────────────

void ScoresController::getScoresTxt(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto db = app().getDbClient("leaderboard");
    if (!db)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Database unavailable\n");
        callback(resp);
        return;
    }

    db->execSqlAsync(
        "SELECT name, score, sail_color FROM scores ORDER BY score DESC, id ASC LIMIT 10",
        [callback](const Result &r)
        {
            std::ostringstream oss;
            for (const auto &row : r)
            {
                oss << row["name"].as<std::string>()
                    << ' '
                    << row["score"].as<int>()
                    << ' '
                    << row["sail_color"].as<int>()
                    << '\n';
            }
            auto resp = HttpResponse::newHttpResponse();
            resp->setContentTypeCodeAndCustomString(CT_TEXT_PLAIN, "text/plain; charset=utf-8");
            resp->setBody(oss.str());
            callback(resp);
        },
        [callback](const DrogonDbException &e)
        {
            LOG_ERROR << "DB query error: " << e.base().what();
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Database error\n");
            callback(resp);
        });
}
