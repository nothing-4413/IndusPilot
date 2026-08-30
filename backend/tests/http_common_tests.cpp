#include "induspilot/http/http_common.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

bool isGeneratedTraceId(const std::string& value) {
    return value.rfind("trace-", 0) == 0 && value.size() > std::string_view("trace-").size();
}

void assertGeneratedTraceForHeader(const std::string& name, const std::string& value) {
    const auto request = drogon::HttpRequest::newHttpRequest();
    request->addHeader(name, value);
    assert(isGeneratedTraceId(induspilot::http::traceIdFor(request)));
}

}  // namespace

int main() {
    const std::string traceId = "trace-accepted_123";
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

    const auto requestIdRequest = drogon::HttpRequest::newHttpRequest();
    requestIdRequest->addHeader("X-Request-Id", "request-id.42");
    assert(induspilot::http::traceIdFor(requestIdRequest) == "request-id.42");

    assertGeneratedTraceForHeader("X-Trace-Id", "invalid trace id");
    assertGeneratedTraceForHeader("X-Trace-Id", "invalid\ntrace");
    assertGeneratedTraceForHeader("X-Trace-Id", std::string("trace-") + "\xC3\xA9");
    assertGeneratedTraceForHeader("X-Request-Id", std::string(129, 'a'));

    const auto precedenceRequest = drogon::HttpRequest::newHttpRequest();
    precedenceRequest->addHeader("X-Trace-Id", "invalid trace id");
    precedenceRequest->addHeader("X-Request-Id", "otherwise-valid-request-id");
    const auto precedenceTraceId = induspilot::http::traceIdFor(precedenceRequest);
    assert(isGeneratedTraceId(precedenceTraceId));
    assert(precedenceTraceId != "otherwise-valid-request-id");
    return 0;
}
