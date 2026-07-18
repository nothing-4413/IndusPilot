#include "induspilot/api/api_types.hpp"

#include <sstream>

namespace induspilot::api {

std::string toJson(const ApiResponse& response) {
    std::ostringstream out;
    out << "{\"success\":" << (response.success ? "true" : "false")
        << ",\"code\":\"" << response.code
        << "\",\"message\":\"" << response.message
        << "\",\"data\":" << response.data << "}";
    return out.str();
}

std::string toJson(const HealthCheck& health) {
    std::ostringstream out;
    out << "{\"service\":\"" << health.service << "\",\"dependencies\":{";
    bool first = true;
    for (const auto& item : health.dependencies) {
        if (!first) {
            out << ",";
        }
        first = false;
        out << "\"" << item.first << "\":" << (item.second ? "true" : "false");
    }
    out << "},\"warnings\":[";
    for (std::size_t i = 0; i < health.warnings.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << "\"" << health.warnings[i] << "\"";
    }
    out << "]}";
    return out.str();
}

}  // namespace induspilot::api