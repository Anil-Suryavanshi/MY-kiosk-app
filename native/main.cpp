#include <windows.h>
#include <shellapi.h>
#include <bcrypt.h>
#include <wrl.h>
#include "WebView2.h"
#include "resource.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "urlmon.lib")
using Microsoft::WRL::Callback;

constexpr UINT WM_VIOLATION = WM_APP + 1;
constexpr UINT WM_ADMIN_UNLOCK = WM_APP + 2;
constexpr UINT WM_ADMIN_EXIT = WM_APP + 3;

struct Config {
    std::wstring startUrl;
    std::vector<std::wstring> allowedHosts;
    std::wstring passwordHash;
};

class ExamApp {
public:
    HWND window{};
    HWND freezeOverlay{};
    ICoreWebView2Controller* controller{};
    ICoreWebView2* webview{};
    Config config;
    int violations{};
    bool frozen{};
    bool unlockDialogOpen{};
    bool warningDialogOpen{};

    void Start(HWND hwnd) {
        window = hwnd;
        config = LoadConfig();
        CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [this](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                    if (FAILED(result)) { MessageBox(window, L"WebView2 Runtime is required.", L"Secure Exam Browser", MB_ICONERROR); return result; }
                    return environment->CreateCoreWebView2Controller(window,
                        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [this](HRESULT result, ICoreWebView2Controller* created) -> HRESULT {
                                if (FAILED(result)) return result;
                                controller = created;
                                controller->AddRef();
                                controller->get_CoreWebView2(&webview);
                                ConfigureWebView();
                                ResizeBrowser();
                                webview->Navigate(config.startUrl.c_str());
                                return S_OK;
                            }).Get());
                }).Get());
    }

    void ResizeBrowser() {
        if (!controller) return;
        RECT rect{}; GetClientRect(window, &rect);
        controller->put_Bounds(rect);
        if (freezeOverlay) MoveWindow(freezeOverlay, 0, 0, rect.right, rect.bottom, TRUE);
    }

    void ReportViolation(const wchar_t* reason) {
        if (frozen || unlockDialogOpen) return;
        ++violations;
        WriteLog(reason);
        if (violations >= 3) {
            frozen = true;
            ShowWindow(freezeOverlay, SW_SHOW);
            SetWindowPos(freezeOverlay, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            MessageBox(window, L"Exam has been paused after 3 security warnings. Call the invigilator.", L"Exam frozen", MB_ICONSTOP | MB_OK);
            return;
        }
        std::wstring message = L"Security warning " + std::to_wstring(violations) + L" of 3.\n\n" + reason +
            L"\n\nDo not leave the exam window or use restricted shortcuts.";
        warningDialogOpen = true;
        MessageBox(window, message.c_str(), L"Secure Exam Browser", MB_ICONWARNING | MB_OK);
        warningDialogOpen = false;
    }

    void RequestUnlock() {
        if (!frozen || unlockDialogOpen) return;
        unlockDialogOpen = true;
        const auto answer = PromptForAdminPassword();
        unlockDialogOpen = false;
        if (answer == IDOK) {
            frozen = false;
            violations = 0;
            ShowWindow(freezeOverlay, SW_HIDE);
            WriteLog(L"Invigilator resumed exam");
            SetForegroundWindow(window);
        }
    }

    void RequestAdminExit() {
        if (unlockDialogOpen) return;
        unlockDialogOpen = true;
        const auto answer = PromptForAdminPassword();
        unlockDialogOpen = false;
        if (answer == IDOK) {
            WriteLog(L"Invigilator ended exam session");
            DestroyWindow(window);
        }
    }

    bool IsAllowed(const wchar_t* uri) const {
        Microsoft::WRL::ComPtr<IUri> parsed;
        if (FAILED(CreateUri(uri, Uri_CREATE_ALLOW_RELATIVE, 0, &parsed))) return false;
        BSTR scheme{}; BSTR host{};
        parsed->GetSchemeName(&scheme); parsed->GetHost(&host);
        std::wstring currentScheme = scheme ? scheme : L"";
        std::wstring currentHost = host ? host : L"";
        SysFreeString(scheme); SysFreeString(host);
        if (_wcsicmp(currentScheme.c_str(), L"https") != 0 && _wcsicmp(currentScheme.c_str(), L"http") != 0) return false;
        for (const auto& allowed : config.allowedHosts) {
            if (_wcsicmp(currentHost.c_str(), allowed.c_str()) == 0) return true;
            const auto suffix = L"." + allowed;
            if (currentHost.size() > suffix.size() && _wcsicmp(currentHost.c_str() + currentHost.size() - suffix.size(), suffix.c_str()) == 0) return true;
        }
        return false;
    }

    static INT_PTR CALLBACK UnlockDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
        auto* app = reinterpret_cast<ExamApp*>(GetWindowLongPtr(dialog, GWLP_USERDATA));
        if (message == WM_INITDIALOG) { SetWindowLongPtr(dialog, GWLP_USERDATA, lParam); SetFocus(GetDlgItem(dialog, IDC_PASSWORD)); return FALSE; }
        if (message == WM_COMMAND && LOWORD(wParam) == IDC_UNLOCK_BUTTON) {
            wchar_t password[256]{}; GetDlgItemText(dialog, IDC_PASSWORD, password, 256);
            if (app && Sha256(password) == app->config.passwordHash) { EndDialog(dialog, IDOK); }
            else { MessageBox(dialog, L"Incorrect password.", L"Secure Exam Browser", MB_ICONWARNING); SetDlgItemText(dialog, IDC_PASSWORD, L""); }
            return TRUE;
        }
        if (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL) { EndDialog(dialog, IDCANCEL); return TRUE; }
        return FALSE;
    }

    INT_PTR PromptForAdminPassword() {
        return DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_UNLOCK), window, UnlockDialogProc, reinterpret_cast<LPARAM>(this));
    }

    static std::wstring Sha256(const std::wstring& text) {
        const int bytes = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(bytes, '\0'); WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, utf8.data(), bytes, nullptr, nullptr); utf8.resize(bytes - 1);
        BCRYPT_ALG_HANDLE alg{}; BCRYPT_HASH_HANDLE hash{}; DWORD objectSize{}, received{}; std::vector<BYTE> object; BYTE output[32]{};
        BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
        BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &received, 0);
        object.resize(objectSize); BCryptCreateHash(alg, &hash, object.data(), objectSize, nullptr, 0, 0);
        BCryptHashData(hash, reinterpret_cast<PUCHAR>(utf8.data()), static_cast<ULONG>(utf8.size()), 0); BCryptFinishHash(hash, output, sizeof(output), 0);
        BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(alg, 0);
        const wchar_t* hex = L"0123456789ABCDEF"; std::wstring result; result.reserve(64);
        for (BYTE value : output) { result += hex[value >> 4]; result += hex[value & 15]; } return result;
    }

private:
    Config LoadConfig() {
        wchar_t location[MAX_PATH]{}; GetModuleFileName(nullptr, location, MAX_PATH);
        std::wstring path(location); path = path.substr(0, path.find_last_of(L"\\/")) + L"\\exam-config.ini";
        wchar_t value[4096]{};
        GetPrivateProfileString(L"Exam", L"StartUrl", L"https://your-exam-portal.example/start", value, 4096, path.c_str()); Config result{ value, {}, L"" };
        GetPrivateProfileString(L"Exam", L"AllowedHosts", L"your-exam-portal.example", value, 4096, path.c_str()); std::wstringstream hosts(value); std::wstring host;
        while (std::getline(hosts, host, L',')) { host.erase(0, host.find_first_not_of(L" \t")); host.erase(host.find_last_not_of(L" \t") + 1); if (!host.empty()) result.allowedHosts.push_back(host); }
        GetPrivateProfileString(L"Exam", L"AdminPasswordSha256", L"", value, 4096, path.c_str()); result.passwordHash = value;
        return result;
    }
    void ConfigureWebView() {
        ICoreWebView2Settings* settings{}; webview->get_Settings(&settings);
        settings->put_AreDefaultContextMenusEnabled(FALSE); settings->put_AreDevToolsEnabled(FALSE); settings->put_IsStatusBarEnabled(FALSE); settings->put_IsZoomControlEnabled(FALSE);
        ICoreWebView2Settings3* settings3{};
        if (SUCCEEDED(settings->QueryInterface(IID_PPV_ARGS(&settings3)))) { settings3->put_AreBrowserAcceleratorKeysEnabled(FALSE); settings3->Release(); }
        settings->Release();
        EventRegistrationToken ignored{};
        webview->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>([](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) { args->put_Handled(TRUE); return S_OK; }).Get(), &ignored);
        webview->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>([this](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) { LPWSTR uri{}; args->get_Uri(&uri); const bool allowed = IsAllowed(uri); CoTaskMemFree(uri); if (!allowed) { args->put_Cancel(TRUE); ReportViolation(L"An unapproved website was requested."); } return S_OK; }).Get(), &ignored);
    }
    void WriteLog(const wchar_t* event) {
        wchar_t location[MAX_PATH]{}; GetModuleFileName(nullptr, location, MAX_PATH); std::wstring path(location); path = path.substr(0, path.find_last_of(L"\\/")) + L"\\exam-security.log";
        SYSTEMTIME now{}; GetLocalTime(&now); std::wofstream log(path, std::ios::app); log << now.wYear << L"-" << now.wMonth << L"-" << now.wDay << L" " << now.wHour << L":" << now.wMinute << L":" << now.wSecond << L" | " << event << L"\n";
    }
};

ExamApp* gApp{};
HHOOK gKeyboardHook{};

LRESULT CALLBACK KeyboardHook(int code, WPARAM keyMessage, LPARAM data) {
    if (code != HC_ACTION || !gApp || gApp->unlockDialogOpen) return CallNextHookEx(gKeyboardHook, code, keyMessage, data);
    const auto* key = reinterpret_cast<KBDLLHOOKSTRUCT*>(data);
    if (keyMessage != WM_KEYDOWN && keyMessage != WM_SYSKEYDOWN) return CallNextHookEx(gKeyboardHook, code, keyMessage, data);
    const bool ctrl = GetAsyncKeyState(VK_CONTROL) & 0x8000, alt = GetAsyncKeyState(VK_MENU) & 0x8000, shift = GetAsyncKeyState(VK_SHIFT) & 0x8000;
    if (ctrl && shift && key->vkCode == VK_F12) { PostMessage(gApp->window, WM_ADMIN_EXIT, 0, 0); return 1; }
    if (gApp->frozen && ctrl && shift && key->vkCode == VK_F10) { PostMessage(gApp->window, WM_ADMIN_UNLOCK, 0, 0); return 1; }
    const bool restricted = key->vkCode == VK_LWIN || key->vkCode == VK_RWIN || (alt && (key->vkCode == VK_TAB || key->vkCode == VK_ESCAPE || key->vkCode == VK_F4)) || (ctrl && (key->vkCode == VK_ESCAPE || key->vkCode == 'L' || key->vkCode == 'T' || key->vkCode == 'N' || key->vkCode == 'W' || key->vkCode == 'P' || key->vkCode == 'S')) || key->vkCode == VK_F11 || key->vkCode == VK_F12;
    if (restricted) { PostMessage(gApp->window, WM_VIOLATION, 0, 0); return 1; }
    return CallNextHookEx(gKeyboardHook, code, keyMessage, data);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: gApp->freezeOverlay = CreateWindowEx(WS_EX_TOPMOST, L"STATIC", L"EXAM PAUSED\n\n3 security warnings detected.\nCall the invigilator.\n\nInvigilator: press Ctrl+Shift+F10 to unlock.", WS_CHILD | SS_CENTER | SS_CENTERIMAGE, 0, 0, 0, 0, hwnd, nullptr, GetModuleHandle(nullptr), nullptr); SendMessage(gApp->freezeOverlay, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE); return 0;
    case WM_SIZE: gApp->ResizeBrowser(); return 0;
    case WM_ACTIVATE: if (LOWORD(wParam) == WA_INACTIVE && !gApp->frozen && !gApp->unlockDialogOpen && !gApp->warningDialogOpen) PostMessage(hwnd, WM_VIOLATION, 0, 0); return 0;
    case WM_VIOLATION: gApp->ReportViolation(L"Restricted shortcut or attempt to leave the exam window detected."); return 0;
    case WM_ADMIN_UNLOCK: gApp->RequestUnlock(); return 0;
    case WM_ADMIN_EXIT: gApp->RequestAdminExit(); return 0;
    case WM_CTLCOLORSTATIC:
        if (reinterpret_cast<HWND>(lParam) == gApp->freezeOverlay) {
            SetTextColor(reinterpret_cast<HDC>(wParam), RGB(255, 255, 255));
            SetBkColor(reinterpret_cast<HDC>(wParam), RGB(0, 0, 0));
            return reinterpret_cast<LRESULT>(GetStockObject(BLACK_BRUSH));
        }
        break;
    case WM_CLOSE: return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ExamApp app; gApp = &app;
    WNDCLASS wc{}; wc.hInstance = instance; wc.lpszClassName = L"SecureExamBrowserWindow"; wc.lpfnWndProc = WindowProc; wc.hCursor = LoadCursor(nullptr, IDC_ARROW); wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)); RegisterClass(&wc);
    const int width = GetSystemMetrics(SM_CXSCREEN), height = GetSystemMetrics(SM_CYSCREEN);
    HWND window = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, wc.lpszClassName, L"", WS_POPUP | WS_VISIBLE, 0, 0, width, height, nullptr, nullptr, instance, nullptr);
    SetWindowPos(window, HWND_TOPMOST, 0, 0, width, height, SWP_SHOWWINDOW); app.Start(window);
    gKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, instance, 0);
    MSG message{}; while (GetMessage(&message, nullptr, 0, 0)) { TranslateMessage(&message); DispatchMessage(&message); }
    if (gKeyboardHook) UnhookWindowsHookEx(gKeyboardHook); if (app.webview) app.webview->Release(); if (app.controller) app.controller->Release(); CoUninitialize(); return 0;
}
