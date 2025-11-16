#include <open62541pp/open62541pp.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <csignal>
#include <atomic>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include "version.h"

using namespace opcua;

using ServerNode = Node<Server>;

static Server* g_server = nullptr;
static std::atomic<bool> g_running{true};

void stopHandler(int /*sig*/) {
    g_running = false;
    if (g_server) {
        std::cout << "\nStopping server...\n";
        g_server->stop();
    }
}

// ========================================
// SENSOR MANAGER (50ms, 100% RANDOM)
// ========================================
class SensorManager {
private:
    struct Sensor {
        ServerNode node;
        double baseValue;
        double range;
        std::string unit;
    };

    std::vector<Sensor> sensors;
    std::mutex mutex;
    std::mt19937 gen;
    std::uniform_real_distribution<double> dist;

public:
    SensorManager() : gen(std::random_device{}()), dist(-1.0, 1.0) {}

    void addSensor(ServerNode& node, double base, double rangePercent, const std::string& unit) {
        sensors.push_back({node, base, base * rangePercent, unit});
    }

    void update() {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& s : sensors) {
            double offset = dist(gen) * s.range;
            double value = s.baseValue + offset;

            if (s.unit == "Humidity" || s.unit == "%") value = std::clamp(value, 0.0, 100.0);
            else value = std::max(0.0, value);

            s.node.writeValue(Variant::fromScalar(std::round(value * 100) / 100.0));
        }
    }
};

int main(int argc, char* argv[]) {
    // === FORCE UTF-8 IN SHELL ===
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // ========================================
    // DEFAULT VALUES
    // ========================================
    uint16_t port = 4840;
    double tempBase = 30.0, humBase = 60.0, presBase = 1013.0;
    double currentBase = 5.0, voltageBase = 230.0;

    // ========================================
    // PARSE ARGUMENTS OR INTERACTIVE MODE
    // ========================================
    bool interactive = (argc == 1);

    if (!interactive) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help") {
                std::cout << "Usage: OPCUASimulationServer.exe [options]\n"
                             "  --port 5000        OPC UA port\n"
                             "  --temp 35          Base temperature (°C)\n"
                             "  --hum 45           Base humidity (%)\n"
                             "  --pres 1010        Base pressure (hPa)\n"
                             "  --current 8.5      Base current (A)\n"
                             "  --voltage 220      Base voltage (V)\n"
                             "  --help             Show this help\n";
                return 0;
            }
            if (arg == "--port" && i + 1 < argc) port = static_cast<uint16_t>(std::stoi(argv[++i]));
            else if (arg == "--temp" && i + 1 < argc) tempBase = std::stod(argv[++i]);
            else if (arg == "--hum" && i + 1 < argc) humBase = std::stod(argv[++i]);
            else if (arg == "--pres" && i + 1 < argc) presBase = std::stod(argv[++i]);
            else if (arg == "--current" && i + 1 < argc) currentBase = std::stod(argv[++i]);
            else if (arg == "--voltage" && i + 1 < argc) voltageBase = std::stod(argv[++i]);
        }
    } else {
        std::cout << "=== INTERACTIVE CONFIGURATION ===\n";
        std::cout << "Press Enter to use default value.\n\n";

        // --- PORT (SPECIAL CASE) ---
        std::string input;
        std::cout << "Port [4840]: ";
        std::getline(std::cin, input);
        if (!input.empty()) {
            try {
                port = static_cast<uint16_t>(std::stoi(input));
            } catch (...) {
                std::cout << "Invalid port. Using 4840.\n";
                port = 4840;
            }
        }

        // --- DOUBLE VALUES ---
        auto ask = [](const std::string& name, double& value, double def) {
            std::string input;
            std::cout << name << " [" << def << "]: ";
            std::getline(std::cin, input);
            if (!input.empty()) {
                try {
                    value = std::stod(input);
                } catch (...) {
                    std::cout << "Invalid value. Using default.\n";
                }
            }
        };

        ask("Temperature base", tempBase, 30.0);
        ask("Humidity base", humBase, 60.0);
        ask("Pressure base", presBase, 1013.0);
        ask("Current base", currentBase, 5.0);
        ask("Voltage base", voltageBase, 230.0);

        std::cout << "\nStarting server with your values...\n\n";
    }

    // ========================================
    // SERVER
    // ========================================
    Server server(port);
    g_server = &server;

    server.setApplicationName("Sensors");
    server.setApplicationUri("urn:example:sensors");
    server.setProductUri("urn:example:sensors");

    ServerNode objectsFolder{server, ObjectId::ObjectsFolder};
    NodeId sensorsFolderId(1, "Sensors");
    ServerNode sensorsFolder = objectsFolder.addObject(sensorsFolderId, "Sensors");

    auto createVariable = [&](const NodeId& id, const std::string& name, double value, const std::string& unit) {
        VariableAttributes attr;
        attr.setDisplayName(LocalizedText("", name));
        attr.setDescription(LocalizedText("", name + " (simulated)"));
        attr.setValue(Variant::fromScalar(value));
        attr.setDataType(DataTypeId::Double);
        attr.setValueRank(ValueRank::Scalar);
        attr.setAccessLevel(AccessLevel::CurrentRead);
        return sensorsFolder.addVariable(id, name, attr);
    };

    NodeId tempId(1, "Temperature"), humId(1, "Humidity"), presId(1, "Pressure");
    NodeId currId(1, "Current"), voltId(1, "Voltage");

    ServerNode tempNode = createVariable(tempId, "Temperature", tempBase, "°C");
    ServerNode humNode = createVariable(humId, "Humidity", humBase, "%");
    ServerNode presNode = createVariable(presId, "Pressure", presBase, "hPa");
    ServerNode currNode = createVariable(currId, "Current", currentBase, "A");
    ServerNode voltNode = createVariable(voltId, "Voltage", voltageBase, "V");

    // ========================================
    // SENSOR MANAGER
    // ========================================
    SensorManager sensorManager;
    sensorManager.addSensor(tempNode, tempBase, 0.20, "°C");
    sensorManager.addSensor(humNode, humBase, 0.20, "%");
    sensorManager.addSensor(presNode, presBase, 0.05, "hPa");
    sensorManager.addSensor(currNode, currentBase, 0.15, "A");
    sensorManager.addSensor(voltNode, voltageBase, 0.10, "V");

    // ========================================
    // THREADS
    // ========================================
    std::thread sensorThread([&]() {
        while (g_running) {
            sensorManager.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

// === SHOW DINAMIC INFO ===
std::thread serverThread([&]() {
    std::cout << "=== " << APP_NAME << " v" << APP_VERSION << " ===\n";
    std::cout << "by " << APP_AUTHOR << " - " << APP_COMPANY << "\n";
    std::cout << APP_DESCRIPTION << "\n\n";
    std::cout << "Source Code: " << AAP_REPO_URL << "\n\n";
    std::cout << "URL: opc.tcp://localhost:" << port << "\n";
    std::cout << "Update: every 50ms (100% RANDOM ±20%)\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    while (g_running) {
        server.runIterate();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
});

    std::signal(SIGINT, stopHandler);
#ifdef _WIN32
    std::signal(SIGBREAK, stopHandler);
#endif

    serverThread.join();
    sensorThread.join();

    std::cout << "Server stopped.\n";
    return 0;
}