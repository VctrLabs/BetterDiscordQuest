#include <windows.h>
#include <winhttp.h>
#include <tlhelp32.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>
#include <atomic>
#include <ctime>
#include "json.hpp"

#pragma comment(lib, "winhttp.lib")

namespace fs = std::filesystem;
using json = nlohmann::json;

std::atomic<bool> g_running{true};

struct GameTarget {
    std::string id;
    std::string name;
    std::string exe_rel_path;
};

struct QueueItem {
    GameTarget game;
    int target_minutes;
};

// Hvatanje Ctrl+C ili zatvaranja konzole za cisto gasenje
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT || signal == CTRL_BREAK_EVENT) {
        g_running = false;
        std::cout << "\n[!] Prekidanje programa i ciscenje procesa...\n";
        return TRUE;
    }
    return FALSE;
}

std::wstring ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wide(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], size);
    return wide;
}

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

int RandInt(std::mt19937& rng, int min_val, int max_val) {
    if (min_val >= max_val) return min_val;
    std::uniform_int_distribution<int> dist(min_val, max_val);
    return dist(rng);
}

// Prekidivo cekanje u sekundama sa opcionim ispisom tajmera
bool InterruptibleSleep(int total_seconds, const std::string& prefix_label = "") {
    for (int elapsed = 0; elapsed < total_seconds; ++elapsed) {
        if (!g_running) return false;

        if (!prefix_label.empty()) {
            int rem = total_seconds - elapsed;
            int mins = rem / 60;
            int secs = rem % 60;
            std::cout << "\r" << prefix_label << " | Preostalo: "
                      << std::setw(2) << std::setfill('0') << mins << "m "
                      << std::setw(2) << std::setfill('0') << secs << "s   " << std::flush;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    if (!prefix_label.empty()) std::cout << "\n";
    return g_running;
}

// Provjera da li je prava igra vec pokrenuta na sistemu
bool IsProcessAlreadyRunning(const std::wstring& exe_name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe = { sizeof(pe) };
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exe_name.c_str()) == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

// Preuzimanje baze igara sa Discorda
bool FetchDetectableGames(const fs::path& cache_path) {
    HINTERNET hSession = WinHttpOpen(L"VctrLabDev-Sim/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, L"discord.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/api/v9/applications/detectable",
                                            nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    bool success = false;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr)) {

        std::ofstream out(cache_path, std::ios::binary);
        DWORD bytesAvailable = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
            std::vector<char> buffer(bytesAvailable);
            DWORD bytesRead = 0;
            if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                out.write(buffer.data(), bytesRead);
            }
        }
        success = true;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return success;
}

// Pretraga do 15 igara koje imaju Windows .exe putanju
std::vector<GameTarget> SearchGames(const json& detectable_data, const std::string& query) {
    std::vector<GameTarget> matches;
    std::string q_lower = ToLower(query);

    for (const auto& item : detectable_data) {
        if (!item.contains("executables") || !item["executables"].is_array()) continue;

        std::string id = item.value("id", "");
        std::string name = item.value("name", "");

        if (id == query || ToLower(name).find(q_lower) != std::string::npos) {
            for (const auto& exe : item["executables"]) {
                if (exe.value("os", "") == "win32" && !exe.value("is_launcher", false)) {
                    std::string rel = exe.value("name", "");
                    // Discord API ponekad stavlja '>' na pocetak naziva fajla
                    if (!rel.empty() && rel.front() == '>') {
                        rel.erase(0, 1);
                    }
                    if (!rel.empty()) {
                        matches.push_back({ id, name, rel });
                        break;
                    }
                }
            }
        }
        if (matches.size() >= 15) break;
    }
    return matches;
}

std::string FormatFutureClockTime(int add_seconds) {
    auto future = std::chrono::system_clock::now() + std::chrono::seconds(add_seconds);
    std::time_t tt = std::chrono::system_clock::to_time_t(future);
    std::tm local_tm{};
    localtime_s(&local_tm, &tt);
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%H:%M");
    return oss.str();
}

bool RunSimulatedSession(const fs::path& stub_path, const QueueItem& item, int index, int total, std::mt19937& rng) {
    fs::path rel_p(item.game.exe_rel_path);
    std::wstring exe_filename = rel_p.filename().wstring();

    if (IsProcessAlreadyRunning(exe_filename)) {
        std::cout << "[!] Preskacem '" << item.game.name << "' — igra je vec stvarno pokrenuta na PC-u.\n";
        return false;
    }

    fs::path temp_root = fs::temp_directory_path() / "VctrLabDev" / item.game.id;
    fs::path target_exe = temp_root / rel_p;

    std::error_code ec;
    fs::create_directories(target_exe.parent_path(), ec);
    fs::copy_file(stub_path, target_exe, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        std::cerr << "[!] Greska pri kopiranju stub.exe: " << ec.message() << "\n";
        return false;
    }

    HANDLE hJob = CreateJobObjectW(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    std::wstring cmd = L"\"" + target_exe.wstring() + L"\"";
    std::wstring work_dir = target_exe.parent_path().wstring();

    if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE,
                        CREATE_SUSPENDED | CREATE_NO_WINDOW, nullptr, work_dir.c_str(), &si, &pi)) {
        std::cerr << "[!] Neuspjesno pokretanje procesa. Kod: " << GetLastError() << "\n";
        CloseHandle(hJob);
        fs::remove_all(temp_root, ec);
        return false;
    }

    AssignProcessToJobObject(hJob, pi.hProcess);
    ResumeThread(pi.hThread);

    // Bazno vrijeme + humanizovani lingering (35 - 110 sekundi)
    int linger_sec = RandInt(rng, 35, 110);
    int total_sec = (item.target_minutes * 60) + linger_sec;

    std::cout << "\n[" << index << "/" << total << "] Pokrenuta igra: " << item.game.name
              << " (" << item.game.exe_rel_path << ")\n";
    std::cout << "[+] Planirano trajanje: " << item.target_minutes << "m + " << linger_sec << "s lingering\n";

    std::string status_label = "[*] Aktivno: " + item.game.name;
    InterruptibleSleep(total_sec, status_label);

    TerminateProcess(pi.hProcess, 0);
    WaitForSingleObject(pi.hProcess, 3000);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(hJob);

    fs::remove_all(temp_root, ec);
    if (g_running) {
        std::cout << "[✓] Sesija zavrsena i privremeni fajlovi obrisani: " << item.game.name << "\n";
    }
    return true;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    std::random_device rd;
    std::mt19937 rng(rd());

    fs::path stub_path = fs::current_path() / "stub.exe";
    fs::path cache_path = fs::current_path() / "detectable_cache.json";

    if (!fs::exists(stub_path)) {
        std::cerr << "[!] Nedostaje stub.exe u folderu programa.\n";
        return 1;
    }

    if (!fs::exists(cache_path)) {
        std::cout << "[*] Preuzimanje baze igara sa Discord API-ja...\n";
        if (!FetchDetectableGames(cache_path)) {
            std::cerr << "[!] Neuspjesno preuzimanje liste igara.\n";
            return 1;
        }
    }

    std::ifstream in(cache_path);
    json detectable_data = json::parse(in, nullptr, false);
    if (detectable_data.is_discarded() || !detectable_data.is_array()) {
        std::cerr << "[!] Ostecen cache fajl. Obrisi detectable_cache.json i pokreni ponovo.\n";
        return 1;
    }

    std::cout << "=== VctrLabDev | Native Game Presence Scheduler ===\n";
    std::vector<QueueItem> queue;

    while (true) {
        std::cout << "\nPretrazi igru po nazivu ili ID-u (ili upisi 'start' za pokretanje reda [" << queue.size() << "]): ";
        std::string input;
        std::getline(std::cin, input);

        if (ToLower(input) == "start") {
            if (queue.empty()) {
                std::cout << "[!] Red cekanja je prazan. Dodaj bar jednu igru.\n";
                continue;
            }
            break;
        }

        if (input.empty()) continue;

        auto matches = SearchGames(detectable_data, input);
        if (matches.empty()) {
            std::cout << "[!] Nema rezultata za '" << input << "'.\n";
            continue;
        }

        for (size_t i = 0; i < matches.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << matches[i].name
                      << " [" << matches[i].exe_rel_path << "]\n";
        }

        std::cout << "Odaberi broj igre (0 za odustajanje): ";
        std::string choice_str;
        std::getline(std::cin, choice_str);
        int choice = 0;
        try { choice = std::stoi(choice_str); } catch (...) { choice = 0; }

        if (choice < 1 || choice > static_cast<int>(matches.size())) continue;

        std::cout << "Trajanje u minutama (Enter za default 15 min): ";
        std::string min_str;
        std::getline(std::cin, min_str);
        int mins = 15;
        if (!min_str.empty()) {
            try { mins = std::max(1, std::stoi(min_str)); } catch (...) { mins = 15; }
        }

        queue.push_back({ matches[choice - 1], mins });
        std::cout << "[+] Dodano u red: " << matches[choice - 1].name << " (" << mins << " min)\n";
    }

    int min_cooldown_min = 10;
    int max_cooldown_min = 120;

    if (queue.size() > 1) {
        std::cout << "\nMinimalna pauza izmedju igara u minutama (Enter za 10): ";
        std::string s_min;
        std::getline(std::cin, s_min);
        if (!s_min.empty()) {
            try { min_cooldown_min = std::max(0, std::stoi(s_min)); } catch (...) {}
        }

        std::cout << "Maksimalna pauza izmedju igara u minutama (Enter za 120): ";
        std::string s_max;
        std::getline(std::cin, s_max);
        if (!s_max.empty()) {
            try { max_cooldown_min = std::max(min_cooldown_min, std::stoi(s_max)); } catch (...) {}
        }
    }

    int total_tasks = static_cast<int>(queue.size());
    int completed_count = 0;
    bool is_first = true;

    while (!queue.empty() && g_running) {
        if (is_first) {
            is_first = false;
            int initial_delay = RandInt(rng, 5, 15);
            std::cout << "\n[*] Pronadjeno " << total_tasks << " igara u redu. Prva krece za " << initial_delay << "s...\n";
            if (!InterruptibleSleep(initial_delay)) break;
        } else {
            int wait_sec = RandInt(rng, min_cooldown_min * 60, max_cooldown_min * 60);
            std::string next_clock = FormatFutureClockTime(wait_sec);
            std::cout << "\n[*] Pauza prije sljedece igre: ~" << (wait_sec / 60)
                      << " min (krece oko " << next_clock << "). Ostalo jos: " << queue.size() << "\n";
            if (!InterruptibleSleep(wait_sec, "[*] Pauza u toku")) break;
        }

        if (!g_running) break;

        // Nasumican izbor sljedece igre iz reda (kao u JS skripti)
        int rand_idx = RandInt(rng, 0, static_cast<int>(queue.size()) - 1);
        QueueItem current = queue[rand_idx];
        queue.erase(queue.begin() + rand_idx);

        completed_count++;
        RunSimulatedSession(stub_path, current, completed_count, total_tasks, rng);
    }

    // Zavrsetak i brisanje korijenskog temp foldera ako je prazan
    std::error_code ec;
    fs::remove(fs::temp_directory_path() / "VctrLabDev", ec);

    if (g_running) {
        std::cout << "\n[✓] Svi zadaci iz reda su zavrseni!\n";
    }
    return 0;
}