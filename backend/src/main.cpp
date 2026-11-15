#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include <sodium.h>

#include "AuthService.h"
#include "Controllers.h"
#include "Database.h"
#include "HttpServer.h"

int main(int argc, char* argv[]) {
    if (sodium_init() < 0) return 1;
    int port = argc > 1 ? std::atoi(argv[1]) : (std::getenv("PORT") ? std::atoi(std::getenv("PORT")) : 8080);
    if (port < 1 || port > 65535) { std::cerr << "Invalid port\n"; return 1; }

    std::string dataDir = std::getenv("DATA_DIR") ? std::getenv("DATA_DIR") : "data";
    Database db(dataDir + "/db.json", "data/seed.json");
    try {
        db.load();
    } catch (const std::exception& e) {
        std::cerr << "Could not load data: " << e.what() << '\n'
                  << "Run the server from the backend folder so it can find data/seed.json.\n";
        return 1;
    }

    AuthService auth(db);
    HttpServer server;

    std::vector<std::unique_ptr<Controller> > controllers;
    controllers.push_back(std::unique_ptr<Controller>(new UserController(db, auth)));
    controllers.push_back(std::unique_ptr<Controller>(new ForumController(db, auth)));
    controllers.push_back(std::unique_ptr<Controller>(new ChatController(db, auth)));
    controllers.push_back(std::unique_ptr<Controller>(new MentorshipController(db, auth)));
    controllers.push_back(std::unique_ptr<Controller>(new NotificationController(db, auth)));
    for (std::size_t i = 0; i < controllers.size(); ++i) controllers[i]->registerRoutes(server);

    server.get("/api/health", [](const HttpRequest&, HttpResponse& res) {
        Json j = Json::object();
        j["status"] = "ok";
        res.send(j);
    });

    std::cout << "SeniorSphere backend running on http://localhost:" << port << std::endl;
    if (!server.listen(port)) {
        std::cerr << "Could not listen on port " << port << " (is it already in use?)\n";
        return 1;
    }
    return 0;
}
