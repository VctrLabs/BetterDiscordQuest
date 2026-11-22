#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    // Minimalna petlja koja drži proces aktivnim bez trošenja CPU resursa
    while (true) {
        Sleep(1000);
    }
    return 0;
}