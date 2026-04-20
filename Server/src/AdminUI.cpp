#define NOMINMAX
#include "AdminUI.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/loop.hpp>

using namespace ftxui;

AdminUI::AdminUI(PerformanceMonitor& perfMonitor,
    SnapshotManager& snapshotManager)
    : perfMonitor(perfMonitor), snapshotManager(snapshotManager),
    running(false) {
    cpuHistory.resize(GRAPH_WIDTH, 0.0);
    memHistory.resize(GRAPH_WIDTH, 0.0);
}

AdminUI::~AdminUI() {
    stop();
}

std::string AdminUI::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

std::string AdminUI::centerText(const std::string& text, int width) {
    int padding = (width - (int)text.length()) / 2;
    if (padding < 0) padding = 0;
    return std::string(padding, ' ') + text;
}

void AdminUI::addLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::string timestamp = "[" + getCurrentTime() + "] " + message;
    logMessages.push_back(timestamp);
    if (logMessages.size() > 100) {
        logMessages.pop_front();
    }
}

void AdminUI::updateGraphHistory() {
    cpuHistory.erase(cpuHistory.begin());
    cpuHistory.push_back(perfMonitor.getCPUUsage());
    memHistory.erase(memHistory.begin());
    memHistory.push_back(perfMonitor.getMemoryUsageMB());
}


void AdminUI::showSplash() {
    auto screen = ScreenInteractive::Fullscreen();

    std::vector<std::string> asciiLogo = {
        "  ___       __  __                                 ____  ____  ",
        " |_ _|_ __ |  \\/  | ___ _ __ ___   ___  _ __ _   |  _ \\| __ )",
        "  | || '_ \\| |\\/| |/ _ \\ '_ ` _ \\ / _ \\| '__| | | | | |  _ \\ ",
        "  | || | | | |  | |  __/ | | | | | (_) | |  | |_| | |_| | |_) |",
        " |___|_| |_|_|  |_|\\___|_| |_| |_|\\___/|_|   \\__, |____/|____/ ",
        "                                               |___/             "
    };

    std::atomic<float> progress(0.0f);
    std::atomic<int> stage(0);
    std::atomic<bool> done(false);

    std::vector<std::string> stages = {
        "Initialising system...",
        "Loading configuration...",
        "Starting network services...",
        "Ready."
    };

    std::thread progressThread([&]() {
        for (int i = 0; i <= 100; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            progress = i / 100.0f;
            if (i < 25) stage = 0;
            else if (i < 50) stage = 1;
            else if (i < 75) stage = 2;
            else             stage = 3;
            screen.PostEvent(Event::Custom);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        done = true;
        screen.ExitLoopClosure()();
        });

    auto renderer = Renderer([&] {
        Elements logoElements;
        for (auto& line : asciiLogo) {
            logoElements.push_back(
                text(line) | color(Color::Cyan) | bold
            );
        }
        return vbox({
            filler(),
            vbox(logoElements) | hcenter,
            text(""),
            text("In-Memory Database")
                | color(Color::GrayLight) | hcenter,
            text(""),
            hbox({
                filler(),
                vbox({
                    gauge(progress.load())
                        | color(Color::Cyan)
                        | size(WIDTH, EQUAL, 50),
                    text(stages[stage.load()])
                        | color(Color::White) | hcenter,
                }),
                filler(),
            }),
            filler(),
            });
        });

    screen.Loop(renderer);
    progressThread.join();
}

void AdminUI::showSnapshotLoader(int keysLoaded, int totalKeys) {
    auto screen = ScreenInteractive::Fullscreen();

    std::thread closeThread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        screen.ExitLoopClosure()();
        });

    float prog = totalKeys > 0
        ? (float)keysLoaded / (float)totalKeys
        : 1.0f;

    bool hasSnapshot = totalKeys > 0;

    auto renderer = Renderer([&] {
        if (!hasSnapshot) {
            return vbox({
                filler(),
                vbox({
                    text("No snapshot found")
                        | color(Color::Yellow) | bold | hcenter,
                    text(""),
                    text("Starting with empty database")
                        | color(Color::GrayLight) | hcenter,
                }) | border | hcenter,
                filler(),
                });
        }

        return vbox({
            filler(),
            vbox({
                text("Restoring Database from Snapshot")
                    | color(Color::Cyan) | bold | hcenter,
                text(""),
                gauge(prog) | color(Color::Green),
                text(""),
                text("Loaded " + std::to_string(keysLoaded) +
                     " of " + std::to_string(totalKeys) + " keys")
                    | color(Color::White) | hcenter,
                text(""),
                text("Restore complete")
                    | color(Color::Green) | bold | hcenter,
            }) | border | hcenter,
            filler(),
            });
        });

    screen.Loop(renderer);
    closeThread.join();
}


bool AdminUI::login() {
    const std::string ADMIN_USERNAME = "admin";
    const std::string ADMIN_PASSWORD = "admin123";

    auto screen = ScreenInteractive::Fullscreen();

    std::string username;
    std::string password;
    std::string errorMessage;
    bool loginSuccess = false;

    InputOption passOption;
    passOption.password = true;

    auto usernameInput = Input(&username, "Username");
    auto passwordInput = Input(&password, "Password", passOption);

    auto loginButton = Button("  Login  ", [&] {
        if (username == ADMIN_USERNAME && password == ADMIN_PASSWORD) {
            loginSuccess = true;
            screen.ExitLoopClosure()();
        }
        else {
            errorMessage = "Invalid credentials. Please try again.";
            password = "";
        }
        });

    auto container = Container::Vertical({
        usernameInput,
        passwordInput,
        loginButton,
        });

    std::vector<std::string> smallLogo = {
        " ___ ____  ____  ",
        "|_ _|  _ \\| __ )",
        " | || | | |  _ \\",
        " | || |_| | |_) |",
        "|___|____/|____/ ",
    };

    auto renderer = Renderer(container, [&] {
        Elements logoElements;
        for (auto& line : smallLogo) {
            logoElements.push_back(
                text(line) | color(Color::Cyan) | bold
            );
        }

        Element errorEl = errorMessage.empty()
            ? text("")
            : text(errorMessage) | color(Color::Red) | hcenter;

        return vbox({
            filler(),
            vbox({
                vbox(logoElements) | hcenter,
                text("In-Memory Database Server")
                    | color(Color::GrayLight) | hcenter,
                separator(),
                text(""),
                hbox({
                    text("Username : ") | color(Color::Yellow),
                    usernameInput->Render() | flex,
                }),
                hbox({
                    text("Password : ") | color(Color::Yellow),
                    passwordInput->Render() | flex,
                }),
                text(""),
                loginButton->Render() | hcenter,
                text(""),
                errorEl,
                text(""),
            }) | border
              | size(WIDTH, EQUAL, 50)
              | hcenter,
            filler(),
            });
        });

    screen.Loop(renderer);
    return loginSuccess;
}



void AdminUI::start() {
    running = true;
    auto screen = ScreenInteractive::Fullscreen();

    std::string commandInput;
    std::string commandFeedback;

    auto input = Input(&commandInput, "Type command...");

    auto submitCommand = [&] {
        std::string cmd = commandInput;
        commandInput = "";
        for (auto& c : cmd) c = toupper(c);

        if (cmd == "SNAPSHOT") {
            snapshotManager.save();
            commandFeedback = "Snapshot saved at " +
                snapshotManager.getLastSnapshotTime();
            addLog("Admin triggered manual snapshot");
        }
        else if (cmd == "LOGOUT") {
            running = false;
            screen.ExitLoopClosure()();
        }
        else if (cmd == "QUIT" || cmd == "EXIT") {
            running = false;
            screen.ExitLoopClosure()();
        }
        else if (cmd == "HELP") {
            commandFeedback =
                "Commands: SNAPSHOT | LOGOUT | QUIT | HELP";
        }
        else if (!cmd.empty()) {
            commandFeedback = "Unknown command. Type HELP.";
        }
        };

    auto inputWithEnter = CatchEvent(input, [&](Event e) {
        if (e == Event::Return) {
            submitCommand();
            return true;
        }
        return false;
        });

    
    std::thread refreshThread([&]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            updateGraphHistory();
            screen.PostEvent(Event::Custom);
        }
        });

    auto renderer = Renderer(inputWithEnter, [&] {

        
        auto header = hbox({
            text(" InMemoryDB ") | bold | color(Color::Cyan),
            text("● ") | color(Color::Green) | bold,
            text("ONLINE ") | color(Color::Green),
            filler(),
            text("Port: 8080  ") | color(Color::GrayLight),
            text(getCurrentTime()) | color(Color::White) | bold,
            text(" "),
            }) | bgcolor(Color::Black);

        
        auto cpuCanvas = canvas([&](Canvas& c) {
            int w = c.width();
            int h = c.height();
            for (int x = 1;
                x < GRAPH_WIDTH && x < (int)cpuHistory.size(); x++) {
                int y1 = h - (int)(cpuHistory[x - 1] / 100.0 * h);
                int y2 = h - (int)(cpuHistory[x] / 100.0 * h);
                c.DrawPointLine(
                    (x - 1) * (w / GRAPH_WIDTH), y1,
                    x * (w / GRAPH_WIDTH), y2,
                    Color::Cyan
                );
            }
            }) | border;

        
        double maxMem = *std::max_element(
            memHistory.begin(), memHistory.end());
        if (maxMem < 1.0) maxMem = 1.0;

        auto memCanvas = canvas([&](Canvas& c) {
            int w = c.width();
            int h = c.height();
            for (int x = 1;
                x < GRAPH_WIDTH && x < (int)memHistory.size(); x++) {
                int y1 = h - (int)(memHistory[x - 1] / maxMem * h);
                int y2 = h - (int)(memHistory[x] / maxMem * h);
                c.DrawPointLine(
                    (x - 1) * (w / GRAPH_WIDTH), y1,
                    x * (w / GRAPH_WIDTH), y2,
                    Color::Magenta
                );
            }
            }) | border;

        double cpu = cpuHistory.back();
        double mem = memHistory.back();

        
        auto leftColumn = vbox({
            text(" CPU Usage") | color(Color::Cyan) | bold,
            text(" " + std::to_string((int)cpu) + "%")
                | color(Color::White),
            gauge((float)(cpu / 100.0)) | color(Color::Cyan),
            cpuCanvas | flex,
            separator(),
            text(" Memory Usage") | color(Color::Magenta) | bold,
            text(" " + std::to_string((int)mem) + " MB")
                | color(Color::White),
            gauge((float)(mem / maxMem)) | color(Color::Magenta),
            memCanvas | flex,
            }) | border | flex;

        
        auto statsPanel = vbox({
            text(" SERVER STATS") | color(Color::Yellow) | bold,
            separator(),
            hbox({
                text(" Uptime        ") | color(Color::GrayLight),
                text(perfMonitor.getUptime())
                    | color(Color::White) | bold,
            }),
            hbox({
                text(" Last Snapshot ") | color(Color::GrayLight),
                text(snapshotManager.getLastSnapshotTime())
                    | color(Color::White) | bold,
            }),
            hbox({
                text(" Clients       ") | color(Color::GrayLight),
                text(std::to_string(
                         perfMonitor.getConnectedUserCount()))
                    | color(Color::Green) | bold,
            }),
            hbox({
                text(" Port          ") | color(Color::GrayLight),
                text("8080") | color(Color::White) | bold,
            }),
            }) | border;

        
        auto users = perfMonitor.getConnectedUsers();

        Elements clientRows;
        clientRows.push_back(hbox({
            text(" Username")
                | color(Color::Yellow) | bold
                | size(WIDTH, EQUAL, 18),
            text("Address")
                | color(Color::Yellow) | bold
                | size(WIDTH, EQUAL, 22),
            text("Connected At")
                | color(Color::Yellow) | bold
                | size(WIDTH, EQUAL, 12),
            }));
        clientRows.push_back(separator());

        if (users.empty()) {
            clientRows.push_back(
                text(" No clients connected")
                | color(Color::GrayLight) | hcenter
            );
        }
        else {
            for (auto& u : users) {
                clientRows.push_back(hbox({
                    text(" " + u.username)
                        | color(Color::Green)
                        | size(WIDTH, EQUAL, 18),
                    text(u.address)
                        | color(Color::White)
                        | size(WIDTH, EQUAL, 22),
                    text(u.connectedAt)
                        | color(Color::GrayLight)
                        | size(WIDTH, EQUAL, 12),
                    }));
            }
        }

        auto clientsPanel = vbox({
            text(" CONNECTED CLIENTS (" +
                 std::to_string(users.size()) + ")")
                | color(Color::Yellow) | bold,
            separator(),
            vbox(clientRows),
            }) | border | flex;

        auto middleColumn = vbox({
            statsPanel,
            clientsPanel | flex,
            }) | flex;

        
        Elements logElements;
        {
            std::lock_guard<std::mutex> lock(logMutex);
            int start = (int)logMessages.size() > 20
                ? (int)logMessages.size() - 20
                : 0;
            for (int i = start; i < (int)logMessages.size(); i++) {
                logElements.push_back(
                    text(logMessages[i]) | color(Color::GrayLight)
                );
            }
        }
        if (logElements.empty()) {
            logElements.push_back(
                text(" No events yet") | color(Color::GrayDark)
            );
        }

        auto logPanel = vbox({
            text(" EVENT LOG") | color(Color::Yellow) | bold,
            separator(),
            vbox(logElements) | flex,
            }) | border | flex;

        
        auto commandPanel = vbox({
            text(" CONSOLE") | color(Color::Yellow) | bold,
            separator(),
            hbox({
                text(" > ") | color(Color::Cyan) | bold,
                inputWithEnter->Render() | flex,
            }),
            text(" " + commandFeedback) | color(Color::Green),
            text(" Commands: SNAPSHOT  LOGOUT  QUIT  HELP")
                | color(Color::GrayDark),
            }) | border;

        auto rightColumn = vbox({
            logPanel | flex,
            commandPanel,
            }) | flex;

      
        auto footer = hbox({
            filler(),
            text(" Enter: submit command ") | color(Color::GrayDark),
            }) | bgcolor(Color::Black);

      
        return vbox({
            header,
            hbox({
                leftColumn,
                middleColumn,
                rightColumn,
            }) | flex,
            footer,
            });
        });

    screen.Loop(renderer);
    refreshThread.join();
}

void AdminUI::stop() {
    running = false;
}