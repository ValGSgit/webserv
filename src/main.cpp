#include "../includes/webserv.hpp"
#include "../includes/config/ConfigParser.hpp"
#include "../includes/http/HttpRequest.hpp"
#include "../includes/http/HttpResponse.hpp"
#include "../includes/cgi/CgiHandler.hpp"
#include "../includes/utils/Utils.hpp"
#include "../includes/server/ServerManager.hpp"

volatile sig_atomic_t g_running = 1;
ServerManager* g_server_manager = NULL;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n🛑 Signal received, shutting down server gracefully..." << std::endl;
        g_running = 0;
        
        if (g_server_manager) {
            g_server_manager->requestShutdown();
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN);

    std::cout << "🌐 WebServ - HTTP Server with epoll" << std::endl;
    std::cout << "📝 Config file: " << argv[1] << std::endl;
    
    ServerManager manager;
    g_server_manager = &manager;
    
    if (!manager.initialize(argv[1])) {
        std::cerr << "❌ Failed to initialize server" << std::endl;
        return 1;
    }
    
    manager.printServerStatus();
    manager.printServerSockets();
    
    manager.run();
    
    g_server_manager = NULL;
    
    return 0;
}
