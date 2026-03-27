#include <drogon/drogon.h>

int main() {
    drogon::app().setThreadNum(1);
    drogon::app().addListener("0.0.0.0", 8080);
    drogon::app().setLogLevel(trantor::Logger::kInfo);
    drogon::app().run();
    return 0;
}
