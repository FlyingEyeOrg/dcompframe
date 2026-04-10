#include "dcompframe/application.h"

int main() {
    dcompframe::Application app;
    if (!app.initialize("demo/demo-config.json")) {
        return 1;
    }

    return app.run();
}
