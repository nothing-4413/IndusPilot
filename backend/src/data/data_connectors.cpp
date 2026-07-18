#include "induspilot/data/data_connectors.hpp"

namespace induspilot::data {

DependencyStatus DataConnectors::probe() const {
    // 当前为骨架探测：真实连接检查将在数据库模块接入后替换。
    return DependencyStatus{true, true, true, false};
}

std::string DataConnectors::describe() const {
    return "MySQL/Redis/MongoDB 连接器占位实现";
}

}  // namespace induspilot::data