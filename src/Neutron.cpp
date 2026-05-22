/*
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

namespace neutron
{
    struct Message
    {
        std::string sender;
        std::string content;
        bool isSelf;
    };

    class CryptographyEngine
    {
    private:
        std::string m_secretKey;

    public:
        CryptographyEngine() : m_secretKey("HYPERNOVA_NEUTRON_2026_SECURE_MATRIX")
        {
        }

        void SetCustomKey(const std::string& key)
        {
            m_secretKey = key;
        }

        std::string ProcessData(const std::string& input)
        {
            std::string output = input;
            size_t keyLength = m_secretKey.length();

            for (size_t i = 0; i < input.length(); ++i)
            {
                output[i] = input[i] ^ m_secretKey[i % keyLength];
                output[i] = output[i] ^ static_cast<char>(i % 255);
            }

            return output;
        }
    };

    class NetworkNode
    {
    private:
        SOCKET m_socket;
        SOCKET m_clientSocket;
        bool m_isRunning;
        bool m_isConnected;
        std::vector<Message> m_chatHistory;
        std::mutex m_historyMutex;
        CryptographyEngine m_crypto;

    public:
        NetworkNode() : m_socket(INVALID_SOCKET), m_clientSocket(INVALID_SOCKET), m_isRunning(false), m_isConnected(false)
        {
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
        }

        ~NetworkNode()
        {
            m_isRunning = false;
            if (m_socket != INVALID_SOCKET)
            {
                closesocket(m_socket);
            }
            if (m_clientSocket != INVALID_SOCKET)
            {
                closesocket(m_clientSocket);
            }
            WSACleanup();
        }

        void ListenLAN(int port)
        {
            m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(port);
            serverAddr.sin_addr.s_addr = INADDR_ANY;

            bind(m_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
            listen(m_socket, 1);

            m_isRunning = true;
            std::thread(&NetworkNode::AcceptLoop, this).detach();
        }

        void ConnectWAN(const std::string& ip, int port)
        {
            m_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            sockaddr_in clientAddr;
            clientAddr.sin_family = AF_INET;
            clientAddr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &clientAddr.sin_addr);

            if (connect(m_clientSocket, (SOCKADDR*)&clientAddr, sizeof(clientAddr)) != SOCKET_ERROR)
            {
                m_isConnected = true;
                m_isRunning = true;
                std::thread(&NetworkNode::ReceiveLoop, this, m_clientSocket).detach();
            }
        }

        void AcceptLoop()
        {
            sockaddr_in clientAddr;
            int clientAddrSize = sizeof(clientAddr);
            m_clientSocket = accept(m_socket, (SOCKADDR*)&clientAddr, &clientAddrSize);

            if (m_clientSocket != INVALID_SOCKET)
            {
                m_isConnected = true;
                std::thread(&NetworkNode::ReceiveLoop, this, m_clientSocket).detach();
            }
        }

        void ReceiveLoop(SOCKET currentSocket)
        {
            char buffer[4096];
            while (m_isRunning)
            {
                int bytesReceived = recv(currentSocket, buffer, 4096, 0);
                if (bytesReceived > 0)
                {
                    std::string encryptedData(buffer, bytesReceived);
                    std::string decryptedText = m_crypto.ProcessData(encryptedData);

                    std::lock_guard<std::mutex> lock(m_historyMutex);
                    m_chatHistory.push_back({"Peer", decryptedText, false});
                }
                else
                {
                    m_isConnected = false;
                    break;
                }
            }
        }

        void SendMessageToPeer(const std::string& text, const std::string& myName)
        {
            if (m_isConnected)
            {
                std::string encryptedText = m_crypto.ProcessData(text);
                send(m_clientSocket, encryptedText.c_str(), encryptedText.length(), 0);

                std::lock_guard<std::mutex> lock(m_historyMutex);
                m_chatHistory.push_back({myName, text, true});
            }
        }

        std::vector<Message> GetHistory()
        {
            std::lock_guard<std::mutex> lock(m_historyMutex);
            return m_chatHistory;
        }

        bool IsConnected() const
        {
            return m_isConnected;
        }
    };

    class ApplicationUI
    {
    private:
        NetworkNode m_network;
        std::string m_profileName;
        std::string m_targetIP;
        int m_targetPort;
        char m_inputBuffer[1024];
        bool m_showAbout;
        bool m_showSettings;

    public:
        ApplicationUI() : m_profileName("hypernova-developer"), m_targetIP("127.0.0.1"), m_targetPort(8080), m_showAbout(false), m_showSettings(false)
        {
            memset(m_inputBuffer, 0, sizeof(m_inputBuffer));
        }

        void SetupPurpleDarkTheme()
        {
            ImGuiStyle& style = ImGui::GetStyle();
            ImVec4* colors = style.Colors;

            colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.05f, 0.11f, 1.00f);
            colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.08f, 0.16f, 1.00f);
            colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.05f, 0.11f, 1.00f);
            colors[ImGuiCol_Border] = ImVec4(0.30f, 0.15f, 0.45f, 1.00f);
            colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.10f, 0.30f, 1.00f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.35f, 0.20f, 0.50f, 1.00f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.45f, 0.25f, 0.65f, 1.00f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.08f, 0.22f, 1.00f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.12f, 0.38f, 1.00f);
            colors[ImGuiCol_Button] = ImVec4(0.30f, 0.15f, 0.45f, 1.00f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.20f, 0.60f, 1.00f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.25f, 0.75f, 1.00f);
            colors[ImGuiCol_Header] = ImVec4(0.30f, 0.15f, 0.45f, 1.00f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.40f, 0.20f, 0.60f, 1.00f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.25f, 0.75f, 1.00f);

            style.FrameRounding = 6.0f;
            style.WindowRounding = 8.0f;
            style.ChildRounding = 6.0f;
        }

        void DrawAvatar(const std::string& name, ImVec2 pos, float radius)
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddCircleFilled(ImVec2(pos.x + radius, pos.y + radius), radius, IM_COL32(100, 50, 150, 255));
            
            std::string initial = name.empty() ? "?" : name.substr(0, 1);
            ImVec2 textSize = ImGui::CalcTextSize(initial.c_str());
            drawList->AddText(ImVec2(pos.x + radius - (textSize.x / 2), pos.y + radius - (textSize.y / 2)), IM_COL32(255, 255, 255, 255), initial.c_str());
        }

        void RenderContactsPanel()
        {
            ImGui::BeginChild("ContactsPanel", ImVec2(250, 0), true);
            
            if (ImGui::Button("Settings", ImVec2(100, 30)))
            {
                m_showSettings = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("About", ImVec2(100, 30)))
            {
                m_showAbout = true;
            }

            ImGui::Separator();

            ImGui::Text("Network Node");
            if (!m_network.IsConnected())
            {
                if (ImGui::Button("Listen (LAN)", ImVec2(220, 30)))
                {
                    m_network.ListenLAN(m_targetPort);
                }
                
                char ipBuf[64];
                strcpy(ipBuf, m_targetIP.c_str());
                if (ImGui::InputText("IP", ipBuf, 64))
                {
                    m_targetIP = ipBuf;
                }
                
                if (ImGui::Button("Connect (WAN/LAN)", ImVec2(220, 30)))
                {
                    m_network.ConnectWAN(m_targetIP, m_targetPort);
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "E2E Encrypted Session Active");
            }

            ImGui::Separator();
            ImGui::Text("Recent Chats");
            
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            DrawAvatar("Peer", cursorPos, 15.0f);
            ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + 40, cursorPos.y + 5));
            ImGui::Text("Anonymous Node");
            ImGui::Dummy(ImVec2(0, 20));

            ImGui::EndChild();
        }

        void RenderChatPanel()
        {
            ImGui::BeginChild("ChatPanel", ImVec2(0, 0), true);

            ImVec2 headerPos = ImGui::GetCursorScreenPos();
            DrawAvatar(m_network.IsConnected() ? "Peer" : "?", headerPos, 20.0f);
            ImGui::SetCursorScreenPos(ImVec2(headerPos.x + 50, headerPos.y + 10));
            ImGui::TextDisabled(m_network.IsConnected() ? "Secure Connection Established" : "Waiting for connection...");
            
            ImGui::Dummy(ImVec2(0, 30));
            ImGui::Separator();

            ImGui::BeginChild("Messages", ImVec2(0, -50), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            std::vector<Message> history = m_network.GetHistory();
            for (const auto& msg : history)
            {
                if (msg.isSelf)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.5f, 1.0f, 1.0f));
                    ImGui::TextWrapped("[You]: %s", msg.content.c_str());
                    ImGui::PopStyleColor();
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    ImGui::TextWrapped("[Peer]: %s", msg.content.c_str());
                    ImGui::PopStyleColor();
                }
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            {
                ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::PushItemWidth(-80);
            if (ImGui::InputText("##MessageInput", m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                std::string text(m_inputBuffer);
                if (!text.empty() && m_network.IsConnected())
                {
                    m_network.SendMessageToPeer(text, m_profileName);
                    memset(m_inputBuffer, 0, sizeof(m_inputBuffer));
                }
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("Send", ImVec2(70, 0)))
            {
                std::string text(m_inputBuffer);
                if (!text.empty() && m_network.IsConnected())
                {
                    m_network.SendMessageToPeer(text, m_profileName);
                    memset(m_inputBuffer, 0, sizeof(m_inputBuffer));
                }
            }

            ImGui::EndChild();
        }

        void RenderModals()
        {
            if (m_showAbout)
            {
                ImGui::OpenPopup("About Neutron");
            }
            if (ImGui::BeginPopupModal("About Neutron", &m_showAbout, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextColored(ImVec4(0.8f, 0.2f, 1.0f, 1.0f), "Neutron");
                ImGui::Text("hypernova-developer");
                ImGui::Text("Version: 1.0.0");
                ImGui::Text("Year: 2026");
                ImGui::Separator();
                if (ImGui::Button("Close", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                    m_showAbout = false;
                }
                ImGui::EndPopup();
            }

            if (m_showSettings)
            {
                ImGui::OpenPopup("Profile Settings");
            }
            if (ImGui::BeginPopupModal("Profile Settings", &m_showSettings, ImGuiWindowFlags_AlwaysAutoResize))
            {
                char nameBuf[64];
                strcpy(nameBuf, m_profileName.c_str());
                if (ImGui::InputText("Display Name", nameBuf, 64))
                {
                    m_profileName = nameBuf;
                }
                
                ImGui::TextDisabled("Profile pictures are rendered dynamically via initials to keep binaries minimal.");
                
                if (ImGui::Button("Save & Close", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                    m_showSettings = false;
                }
                ImGui::EndPopup();
            }
        }

        void Run()
        {
            glfwInit();
            GLFWwindow* window = glfwCreateWindow(1000, 700, "Neutron E2EE Terminal", nullptr, nullptr);
            glfwMakeContextCurrent(window);
            glfwSwapInterval(1);

            ImGui::CreateContext();
            SetupPurpleDarkTheme();
            
            ImGui_ImplGlfw_InitForOpenGL(window, true);
            ImGui_ImplOpenGL3_Init("#version 130");

            while (!glfwWindowShouldClose(window))
            {
                glfwPollEvents();
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
                ImGui::Begin("MainLayout", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

                RenderContactsPanel();
                ImGui::SameLine();
                RenderChatPanel();
                RenderModals();

                ImGui::End();

                ImGui::Render();
                int display_w, display_h;
                glfwGetFramebufferSize(window, &display_w, &display_h);
                glViewport(0, 0, display_w, display_h);
                glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                glfwSwapBuffers(window);
            }

            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    };

    void ExecuteCoreEngine()
    {
        neutron::ApplicationUI ui;
        ui.Run();
    }
}

int main()
{
    neutron::ExecuteCoreEngine();
    return 0;
}
*/

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace neutron
{
    class NetworkEngine
    {
    private:
        std::string m_nickname;
        SOCKET m_connectionSocket;
        bool m_isServerActive;
        bool m_isHostMode;
        std::mutex m_displayMutex;

        void EraseAndOverwriteUserLine(const std::string& message)
        {
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hConsole, &csbi);
            
            csbi.dwCursorPosition.Y -= 1;
            csbi.dwCursorPosition.X = 0;
            SetConsoleCursorPosition(hConsole, csbi.dwCursorPosition);
            
            DWORD written;
            FillConsoleOutputCharacterA(hConsole, ' ', csbi.dwSize.X, csbi.dwCursorPosition, &written);
            
            std::cout << m_nickname << " (You): " << message << "\n";
        }

        void PrintTunnelHeader()
        {
            std::cout << "[SYSTEM] End-to-End Tunnel Established!" << std::endl;
            std::cout << "=============================================" << std::endl;
        }

        void PrintHelpMenu()
        {
            std::cout << "\n--- NEUTRON CORE COMMANDS ---" << std::endl;
            std::cout << "/help  : Display available system commands." << std::endl;
            std::cout << "/clear : Clear the console screen buffer." << std::endl;
            std::cout << "/quit  : Terminate the secure session." << std::endl;
            std::cout << "-----------------------------\n" << std::endl;
        }

    public:
        NetworkEngine() : m_connectionSocket(INVALID_SOCKET), m_isServerActive(true), m_isHostMode(false)
        {
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
            std::cout << "[NEUTRON] Enter your Nickname: ";
            std::getline(std::cin, m_nickname);
            system("CLS");
            SetConsoleTitleA("Neutron System Core 2026");
            system("COLOR 0D");
        }

        ~NetworkEngine()
        {
            m_isServerActive = false;
            if (m_connectionSocket != INVALID_SOCKET)
            {
                closesocket(m_connectionSocket);
            }
            WSACleanup();
        }

        void ExecuteServerMode()
        {
            m_isHostMode = true;
            SOCKET listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            sockaddr_in serverAddress = {};
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_port = htons(8080);
            serverAddress.sin_addr.s_addr = INADDR_ANY;

            bind(listeningSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
            listen(listeningSocket, 1);

            while (true)
            {
                m_isServerActive = true;
                std::cout << "[SYSTEM] Operational. Waiting for peer on port 8080..." << std::endl;
                m_connectionSocket = accept(listeningSocket, NULL, NULL);

                if (m_connectionSocket != INVALID_SOCKET)
                {
                    InitializeCommunication();
                }
                
                closesocket(m_connectionSocket);
                m_connectionSocket = INVALID_SOCKET;
                
                std::cout << "\n[SYSTEM] Re-initializing server loop..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            closesocket(listeningSocket);
        }

        void ExecuteClientMode(const std::string& targetIp)
        {
            m_isHostMode = false;
            m_connectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            sockaddr_in remoteAddress = {};
            remoteAddress.sin_family = AF_INET;
            remoteAddress.sin_port = htons(8080);
            inet_pton(AF_INET, targetIp.c_str(), &remoteAddress.sin_addr);

            std::cout << "[SYSTEM] Attempting connection to " << targetIp << "..." << std::endl;
            if (connect(m_connectionSocket, (sockaddr*)&remoteAddress, sizeof(remoteAddress)) == 0)
            {
                InitializeCommunication();
            }
            else
            {
                std::cout << "[ERROR] Connection establishment failed!" << std::endl;
            }
        }

    private:
        void InitializeCommunication()
        {
            system("CLS");
            PrintTunnelHeader();

            std::thread receiverThread(&NetworkEngine::IncomingTrafficHandler, this);
            receiverThread.detach();

            std::string userMessage;
            while (m_isServerActive)
            {
                std::getline(std::cin, userMessage);
                if (!m_isServerActive)
                {
                    break;
                }
                if (userMessage == "/quit")
                {
                    m_isServerActive = false;
                    break;
                }
                if (userMessage == "/clear")
                {
                    std::lock_guard<std::mutex> lock(m_displayMutex);
                    system("CLS");
                    PrintTunnelHeader();
                    continue;
                }
                if (userMessage == "/help")
                {
                    std::lock_guard<std::mutex> lock(m_displayMutex);
                    PrintHelpMenu();
                    continue;
                }
                if (!userMessage.empty())
                {
                    std::string payload = m_nickname + ": " + userMessage;
                    send(m_connectionSocket, payload.c_str(), static_cast<int>(payload.length()), 0);
                    
                    std::lock_guard<std::mutex> lock(m_displayMutex);
                    EraseAndOverwriteUserLine(userMessage);
                }
            }
        }

        void IncomingTrafficHandler()
        {
            char networkBuffer[2048];
            while (m_isServerActive)
            {
                int bytesRead = recv(m_connectionSocket, networkBuffer, 2048, 0);
                if (bytesRead > 0)
                {
                    std::lock_guard<std::mutex> lock(m_displayMutex);
                    std::string receivedData(networkBuffer, bytesRead);
                    Beep(800, 150);
                    std::cout << receivedData << "\n";
                }
                else
                {
                    std::lock_guard<std::mutex> lock(m_displayMutex);
                    std::cout << "\n[SYSTEM] Peer disconnected from session." << std::endl;
                    m_isServerActive = false;
                    
                    if (!m_isHostMode)
                    {
                        exit(0);
                    }
                    
                    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
                    INPUT_RECORD ir;
                    DWORD written;
                    ir.EventType = KEY_EVENT;
                    ir.Event.KeyEvent.bKeyDown = TRUE;
                    ir.Event.KeyEvent.wRepeatCount = 1;
                    ir.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
                    ir.Event.KeyEvent.wVirtualScanCode = static_cast<WORD>(MapVirtualKeyA(VK_RETURN, MAPVK_VK_TO_VSC));
                    ir.Event.KeyEvent.uChar.AsciiChar = '\r';
                    ir.Event.KeyEvent.dwControlKeyState = 0;
                    WriteConsoleInputA(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &written);
                    break;
                }
            }
        }
    };
}

int main()
{
    neutron::NetworkEngine appCore;
    std::string selection;
    std::cout << "=== NEUTRON SUBSYSTEM ===" << std::endl;
    std::cout << "1. Act as Host (Listen)\n2. Act as Peer (Connect)\nChoice: ";
    std::cin >> selection;

    if (selection == "1")
    {
        appCore.ExecuteServerMode();
    }
    else
    {
        std::string remoteIp;
        std::cout << "Enter target IP: ";
        std::cin >> remoteIp;
        appCore.ExecuteClientMode(remoteIp);
    }
    return 0;
}
