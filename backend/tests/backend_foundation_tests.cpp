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
    assert(identity.authenticate("admin", "admin123"));

    induspilot::modules::AiService ai;
    assert(!ai.explainAlert("温度异常").available);

    app.stop();
    assert(!app.isRunning());
    return 0;
}