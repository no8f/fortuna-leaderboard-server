#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class ScoresController : public HttpController<ScoresController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ScoresController::postScore,   "/api/v1/scores", Post);
    ADD_METHOD_TO(ScoresController::getScores,   "/api/v1/scores", Get);
    ADD_METHOD_TO(ScoresController::getIndex,    "/",              Get);
    ADD_METHOD_TO(ScoresController::getScoresTxt,"/scores.txt",    Get);
    ADD_METHOD_TO(ScoresController::getHealth,   "/health",        Get);
    METHOD_LIST_END

    void postScore(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

    void getScores(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

    void getIndex(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);

    void getScoresTxt(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback);

    void getHealth(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);
};
