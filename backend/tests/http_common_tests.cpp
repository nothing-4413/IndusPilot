#include "induspilot/http/http_common.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

int main() {
    const std::string traceId = "trace-quote\" slash\\ line\nnext";
    const std::string path = "/api/v1/assets/a\"\\\n001";
    const auto request = drogon::HttpRequest::newHttpRequest();
    request->addHeader("X-Trace-Id", traceId);
    request->setPath(path);

    std::ostringstream captured;
    auto* originalBuffer = std::cout.rdbuf(captured.rdbuf());
    induspilot::http::writeRequestLog(request);
    std::cout.rdbuf(originalBuffer);

    Json::CharReaderBuilder readerBuilder;
    Json::Value event;
    std::string errors;
    std::istringstream logLine(captured.str());
    assert(Json::parseFromStream(readerBuilder, logLine, &event, &errors));
    assert(event["event"].asString() == "http_request");
    assert(event["traceId"].asString() == traceId);
    assert(event["path"].asString() == path);
    assert(event["user"].asString() == "anonymous");
    assert(captured.str().find('\n') == captured.str().size() - 1);
    return 0;
}
