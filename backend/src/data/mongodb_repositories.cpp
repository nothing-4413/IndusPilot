#include "induspilot/data/mongodb_repositories.hpp"

#ifdef INDUSPILOT_WITH_MONGODB

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/options/index.hpp>
#include <mongocxx/options/update.hpp>

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace induspilot::data {
namespace {

mongocxx::instance& driverInstance() {
    static mongocxx::instance instance{};
    return instance;
}

std::string stringField(const bsoncxx::document::view& document, const char* name) {
    const auto element = document[name];
    if (!element || element.type() != bsoncxx::type::k_string) {
        return {};
    }
    const auto value = element.get_string().value;
    return {value.data(), value.length()};
}

std::string boundedProbeUri(const std::string& uri, int timeoutMs) {
    const auto boundedTimeout = std::max(1, timeoutMs);
    const auto separator = uri.find('?') == std::string::npos ? '?' : '&';
    return uri + separator + "serverSelectionTimeoutMS=" + std::to_string(boundedTimeout) +
        "&connectTimeoutMS=" + std::to_string(boundedTimeout);
}

}  // namespace

std::string sanitizeMongoProbeFailure(const std::string& diagnostic) {
    constexpr std::size_t kMaxMongoProbeReasonLength = 512;
    std::string sanitized = diagnostic;
    for (const std::string scheme : {std::string{"mongodb://"}, std::string{"mongodb+srv://"}}) {
        std::size_t searchStart = 0;
        for (;;) {
            const auto schemePosition = sanitized.find(scheme, searchStart);
            if (schemePosition == std::string::npos) {
                break;
            }
            const auto uriStart = schemePosition;
            const auto uriEnd = sanitized.find_first_of(" \t\r\n'\"),;]}", schemePosition + scheme.size());
            const auto uriLength = uriEnd == std::string::npos ? std::string::npos : uriEnd - uriStart;
            const std::string replacement = "<mongodb-uri-redacted>";
            sanitized.replace(uriStart, uriLength, replacement);
            searchStart = uriStart + replacement.size();
        }
    }

    std::string reason = "MongoDB authenticated ping failed: " + sanitized;
    if (reason.size() > kMaxMongoProbeReasonLength) {
        reason.resize(kMaxMongoProbeReasonLength - 3);
        reason += "...";
    }
    return reason;
}

namespace {

void reconcileAiInteractionIndexes(
    mongocxx::client& client,
    const std::string& database,
    const std::shared_ptr<AiInteractionMetricsSink>& metrics) {
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto startedAt = std::chrono::steady_clock::now();
    try {
        auto collection = client[database]["ai_interactions"];
        auto identityOptions = mongocxx::options::index{};
        identityOptions.unique(true);
        collection.create_index(document{} << "interactionCode" << 1 << finalize, identityOptions);
        collection.create_index(document{}
                                << "relatedType" << 1
                                << "relatedId" << 1
                                << "createdAt" << -1
                                << finalize);
        if (metrics) {
            metrics->recordAiInteractionOperation(
                "reconcile",
                true,
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startedAt).count());
        }
    } catch (const std::exception& error) {
        if (metrics) {
            metrics->recordAiInteractionOperation(
                "reconcile",
                false,
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startedAt).count());
        }
        throw std::runtime_error(
            "MongoDB AI interaction index reconciliation failed for " + database +
            ".ai_interactions. Resolve duplicate interactionCode values or incompatible existing indexes, then retry: " +
            error.what());
    }
}

}  // namespace

MongoProbeResult probeMongoDb(const std::string& uri, const std::string& database, int timeoutMs) {
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    if (database.empty()) {
        return {false, "MongoDB authenticated ping failed: database is empty"};
    }

    try {
        mongocxx::client client((driverInstance(), mongocxx::uri{boundedProbeUri(uri, timeoutMs)}));
        const auto response = client[database].run_command(document{} << "ping" << 1 << finalize);
        const auto ok = response["ok"];
        if (!ok || (ok.type() != bsoncxx::type::k_int32 && ok.type() != bsoncxx::type::k_int64 &&
                    ok.type() != bsoncxx::type::k_double)) {
            return {false, "MongoDB authenticated ping returned an invalid response"};
        }
        const auto okValue = ok.type() == bsoncxx::type::k_int32
            ? static_cast<double>(ok.get_int32().value)
            : ok.type() == bsoncxx::type::k_int64
                ? static_cast<double>(ok.get_int64().value)
                : ok.get_double().value;
        return evaluateMongoProbeOk(okValue);
    } catch (const std::exception& error) {
        return {false, sanitizeMongoProbeFailure(error.what())};
    } catch (...) {
        return {false, "MongoDB authenticated ping failed: unknown driver error"};
    }
}

MongoProbeResult evaluateMongoProbeOk(const double okValue) {
    if (!(okValue > 0.0)) {
        return {false, "MongoDB authenticated ping returned ok <= 0"};
    }
    return {true, "MongoDB authenticated ping succeeded"};
}

MongoAiInteractionRepository::MongoAiInteractionRepository(
    const std::string& uri,
    const std::string& database,
    const int timeoutMs,
    std::shared_ptr<AiInteractionMetricsSink> metrics)
    : client_((driverInstance(), mongocxx::uri{boundedProbeUri(uri, timeoutMs)})), database_(database), metrics_(std::move(metrics)) {
    if (database_.empty()) {
        throw std::invalid_argument("mongodb.database must not be empty for AI interaction storage");
    }
    // Keep the HTTP runtime alive during temporary auth/network failures so readiness can report 503.
    // Structural index errors still fail startup after a successful authenticated probe.
    if (probeMongoDb(uri, database_, timeoutMs).available) {
        ensureIndexes();
    }
}

void MongoAiInteractionRepository::ensureIndexes() const {
    std::lock_guard lock(indexMutex_);
    if (indexesReconciled_) {
        return;
    }
    reconcileAiInteractionIndexes(client_, database_, metrics_);
    indexesReconciled_ = true;
}

domain::AiInteraction MongoAiInteractionRepository::save(domain::AiInteraction interaction) {
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_document;

    const auto startedAt = std::chrono::steady_clock::now();
    try {
        ensureIndexes();
        const auto now = bsoncxx::types::b_date{std::chrono::system_clock::now()};
        const auto filter = document{}
            << "interactionCode" << interaction.id
            << finalize;
        const auto updates = document{}
            << "$set" << open_document
            << "interactionCode" << interaction.id
            << "relatedType" << interaction.relatedType
            << "relatedId" << interaction.relatedId
            << "prompt" << interaction.input
            << "response" << interaction.output
            << "updatedAt" << now
            << close_document
            << "$setOnInsert" << open_document
            << "createdAt" << now
            << close_document
            << finalize;
        auto options = mongocxx::options::update{};
        options.upsert(true);
        client_[database_]["ai_interactions"].update_one(filter.view(), updates.view(), options);
        if (metrics_) {
            metrics_->recordAiInteractionOperation(
                "write",
                true,
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startedAt).count());
        }
        return interaction;
    } catch (...) {
        if (metrics_) {
            metrics_->recordAiInteractionOperation(
                "write",
                false,
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startedAt).count());
        }
        throw;
    }
}

std::vector<domain::AiInteraction> MongoAiInteractionRepository::list() const {
    return list(Query{}).interactions;
}

MongoAiInteractionRepository::Page MongoAiInteractionRepository::list(const Query& query) const {
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;

    const auto startedAt = std::chrono::steady_clock::now();
    try {
        ensureIndexes();
        document filter;
        if (query.relatedType) {
            filter << "relatedType" << *query.relatedType;
        }
        if (query.relatedId) {
            filter << "relatedId" << *query.relatedId;
        }
        const auto filterDocument = filter << finalize;
        auto options = mongocxx::options::find{};
        options.sort(document{} << "createdAt" << -1 << finalize);
        if (query.limit) {
            options.limit(static_cast<std::int64_t>(*query.limit));
        }
        options.skip(static_cast<std::int64_t>(query.offset));
        auto collection = client_[database_]["ai_interactions"];
        Page page;
        page.total = static_cast<std::size_t>(collection.count_documents(filterDocument.view()));
        for (const auto& item : collection.find(filterDocument.view(), options)) {
            page.interactions.push_back({
                stringField(item, "interactionCode"),
                stringField(item, "relatedType"),
                stringField(item, "relatedId"),
                stringField(item, "prompt"),
                stringField(item, "response")});
        }
        if (metrics_) {
            metrics_->recordAiInteractionOperation(
                "read",
                true,
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startedAt).count());
        }
        return page;
    } catch (...) {
        if (metrics_) {
            metrics_->recordAiInteractionOperation(
                "read",
                false,
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startedAt).count());
        }
        throw;
    }
}

}  // namespace induspilot::data

#endif  // INDUSPILOT_WITH_MONGODB
