#include "induspilot/api/router.hpp"
#include "induspilot/app/application.hpp"
#include "induspilot/app/config.hpp"
#include "induspilot/modules/ai_service.hpp"
#include "induspilot/modules/identity_service.hpp"

#include <cassert>

int main() {
    induspilot::app::Application app(induspilot::app::AppConfig{});
    app.start();
    assert(app.isRunning());
    assert(app.router().handle("GET", "/health").success);
    assert(!app.router().handle("GET", "/missing").success);

    induspilot::modules::IdentityService identity;
    const auto login = identity.login({"admin", "admin123"});
    assert(login.success);
    assert(login.session.has_value());
    const auto permissions = identity.permissionsForRoles(login.session->user.roles);
    assert(identity.hasPermission(permissions, "asset:write"));
    assert(identity.validateSession(login.session->token).has_value());
    assert(identity.logout(login.session->token));
    assert(!identity.validateSession(login.session->token).has_value());

    induspilot::modules::AiService ai;
    assert(!ai.explainAlert("温度异常").available);

    app.stop();
    assert(!app.isRunning());
    return 0;
}