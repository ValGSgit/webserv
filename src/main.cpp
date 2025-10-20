#include "../includes/webserv.hpp"
#include "../includes/config/ConfigParser.hpp"
#include "../includes/http/HttpRequest.hpp"
#include "../includes/http/HttpResponse.hpp"
#include "../includes/cgi/CgiHandler.hpp"
#include "../includes/utils/Utils.hpp"
#include "../includes/server/ServerManager.hpp"

volatile sig_atomic_t g_running = 1;
ServerManager* g_server_manager = NULL; // Global pointer for signal handler

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n🛑 Signal received, shutting down server gracefully..." << std::endl;
        g_running = 0;
        
        // Signal-safe shutdown request
        if (g_server_manager) {
            g_server_manager->requestShutdown();
        }
    }
}

bool initialize(ServerManager &_server_manager, const std::string& config_file) {
            std::cout << " Initializing WebServ with advanced architecture..." << std::endl;
            std::cout << " Config file: " << config_file << std::endl;
            
            if (!_server_manager.initialize(config_file)) {
                std::cerr << " Failed to initialize ServerManager" << std::endl;
                return false;
            }
            return true;
}
        
void run(ServerManager &_server_manager) {
    std::cout << "\n🚀 Starting WebServ with poll-based I/O..." << std::endl;
    std::cout << "📡 Press Ctrl+C to stop server" << std::endl;
    std::cout << "📊 The server will print status updates during operation" << std::endl;
            
    // Set global pointer for signal handler
    g_server_manager = &_server_manager;
            
    // ServerManager::run() takes care of the main event loop
    _server_manager.run();
            
    // Clear global pointer
    g_server_manager = NULL;
            
    std::cout << " Server main loop exited, performing cleanup..." << std::endl;
    _server_manager.shutdown();
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

    // Parse configuration
    std::cout << "Parsing configuration file: " << argv[1] << std::endl;
    std::string configFile = argv[1];
    

    ServerManager manager;
    if (!initialize(manager, configFile)) {
        std::cout << "Server Manager Failed to Initialize" << std::endl;
    } else {
        run(manager);
    }

    return 0;
}
