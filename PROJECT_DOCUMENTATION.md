# Secure Exam Browser - Project Documentation

**Document version:** 1.0  
**Project type:** Windows secure exam browser  
**License:** Proprietary - All Rights Reserved  
**Primary distributable:** Native C++/Win32 build in `native`

## 1. Purpose

Secure Exam Browser is a fullscreen Windows application for delivering web-based examinations in a controlled browser window. It obtains the current exam URL from a configuration service, opens that URL in Microsoft Edge WebView2, restricts common browser escape routes, and records security violations during an exam session.

It is designed to be deployed by an exam administrator or invigilator. It is not a replacement for operating-system kiosk policy or physical supervision.

## 2. Main capabilities

- Opens as a borderless, maximized, topmost application and is hidden from the Windows taskbar.
- Shows a **Loading your exam... Please wait.** screen until the first exam page finishes loading.
- Retrieves the exam URL dynamically from a JSON launch-configuration endpoint.
- Allows navigation only to the returned exam host, its subdomains, and configured additional hosts.
- Disables WebView2 developer tools, browser context menus, zoom, status bar, browser accelerator keys, and pop-up windows.
- Detects common restricted shortcuts and losing focus from the exam window.
- Displays two security warnings; after the third violation it displays a blocking **Exam paused** overlay.
- Requires an invigilator password to resume a paused exam or end the session.
- Writes security events to `exam-security.log` next to the native executable.
- Can be built as either a 64-bit (x64) or 32-bit (x86) Windows application.

## 3. System requirements

### Build computer

- Windows 10 or Windows 11
- Visual Studio 2022 or Visual Studio Build Tools with the **Desktop development with C++** workload
- Windows 10/11 SDK
- Internet access for NuGet restore on the first build

### Exam computer

- Windows 10 or Windows 11
- Microsoft Edge WebView2 Runtime, matching the application architecture
- Internet connection when the exam URL is fetched and when the exam website is used
- A standard, non-administrator user account is recommended for actual exams

## 4. Technology stack

| Area | Technology |
|---|---|
| Primary application | C++17, Win32 API |
| Embedded browser | Microsoft Edge WebView2 SDK 1.0.3065.39 |
| Networking | Windows WinHTTP |
| Password hashing | Windows CNG / SHA-256 |
| Installer (recommended) | Inno Setup |
| Alternative implementation | .NET 8 WPF with WebView2 |

## 5. Project layout

| Path | Purpose |
|---|---|
| `native/main.cpp` | Native browser behavior, loading screen, security checks, navigation rules, password validation, and logging |
| `native/SecureExamBrowserNative.vcxproj` | Native Visual Studio project; contains x64 and Win32/x86 build configurations |
| `native/exam-config.ini` | Runtime configuration for the native application |
| `native/x64/Release/` | Generated 64-bit distribution files |
| `native/x86/Release/` | Generated 32-bit distribution files |
| `MainWindow.xaml` / `MainWindow.xaml.cs` | Alternative .NET/WPF implementation |
| `exam-config.json` | Runtime configuration for the WPF implementation |
| `LICENSE` / `LICENSE.txt` | Proprietary licence text |
| `INSTALLATION_INFORMATION.txt` | Text shown before installation in Inno Setup |
| `POST_INSTALLATION_INFORMATION.txt` | Text shown after installation in Inno Setup |
| `SecureExamBrowser.reg` | Optional current-user registry metadata file |
| `README.md` | Short build and setup reference |

## 6. How startup works

1. The native executable starts in fullscreen/topmost mode.
2. The loading screen is shown immediately.
3. WebView2 initializes.
4. The app calls the URL in `LaunchConfigUrl`.
5. The service must return a valid JSON response containing an exam URL.
6. The returned URL's host becomes the current session's primary allowed host.
7. The browser navigates to the exam URL.
8. When that navigation completes, the loading screen disappears and the exam is visible.

If WebView2 is unavailable, the launch service cannot be reached, or the response is invalid, the app displays an error message instead of opening the exam.

## 7. Launch service response contract

The `LaunchConfigUrl` endpoint must return JSON in this shape:

```json
{
  "success": true,
  "url": "https://exam.example.com/start"
}
```

- `success` must be `true`.
- `url` must be an absolute `https://` or `http://` URL.
- The application blocks navigation to other schemes, such as `file:`, `mailto:`, or custom protocols.

## 8. Native configuration

Edit `native/exam-config.ini` **before distribution**:

```ini
[Exam]
LaunchConfigUrl=https://your-service.example.com/launch
AllowedHosts=exam.example.com,cdn.example.com
AdminPasswordSha256=YOUR_UPPERCASE_SHA256_HASH
```

| Setting | Meaning |
|---|---|
| `LaunchConfigUrl` | The endpoint that supplies the current exam URL. |
| `AllowedHosts` | Extra comma-separated domains needed by the exam, such as a CDN or identity provider. The returned exam host is automatically allowed. |
| `AdminPasswordSha256` | Uppercase SHA-256 hash of the invigilator password. |

### Create the password hash

Run this in PowerShell and copy the full output into `AdminPasswordSha256`:

```powershell
$password = 'replace-with-a-strong-password'
[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($password)))
```

**Important:** The sample configuration contains a known default password hash. Replace it before releasing the software for any real exam.

## 9. Build instructions

Run these commands from the repository root in PowerShell. The MSBuild path may differ when using a different Visual Studio edition or version.

```powershell
# Build 64-bit application
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe' .\native\SecureExamBrowserNative.vcxproj /p:Configuration=Release /p:Platform=x64

# Build 32-bit application (the Visual Studio platform name is Win32)
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe' .\native\SecureExamBrowserNative.vcxproj /p:Configuration=Release /p:Platform=Win32
```

Successful outputs:

| Architecture | Output folder |
|---|---|
| x64 | `native\x64\Release\` |
| x86 | `native\x86\Release\` |

The x86 build runs on both 32-bit and 64-bit Windows. The x64 build runs only on 64-bit Windows and is the preferred option for modern computers.

## 10. Files to distribute

For each architecture, distribute these files from the **same** output folder:

```text
SecureExamBrowserNative.exe
WebView2Loader.dll
exam-config.ini
```

Do not mix x64 and x86 files. Use the x64 set on 64-bit client computers; use the x86 set only where a 32-bit build is required. The target computer must also have the matching Microsoft Edge WebView2 Runtime installed.

## 11. Inno Setup installer guidance

Use the x64 or x86 output folder as the installer source. Include the three files listed in the previous section.

For the **Application Documentation** page in the Inno Setup Script Wizard, use:

| Wizard field | File |
|---|---|
| License file | `LICENSE.txt` |
| Information file shown before installation | `INSTALLATION_INFORMATION.txt` |
| Information file shown after installation | `POST_INSTALLATION_INFORMATION.txt` |

`LICENSE.txt` is provided specifically so the wizard can display the licence reliably; `LICENSE` contains the same proprietary terms without a file extension.

The optional `SecureExamBrowser.reg` adds only basic application metadata under `HKEY_CURRENT_USER\Software\SecureExamBrowser`. It is not required for the browser to run and should be included only if your installer needs it.

## 12. Security behavior and invigilator controls

The native browser blocks or reports common shortcuts including Windows keys, Alt+Tab, Alt+Esc, Alt+F4, Ctrl+Esc, Ctrl+L, Ctrl+T, Ctrl+N, Ctrl+W, Ctrl+P, Ctrl+S, F11, and F12.

| Event | Result |
|---|---|
| First restricted shortcut or focus loss | Security warning 1 of 3 |
| Second restricted shortcut or focus loss | Security warning 2 of 3 |
| Third restricted shortcut or focus loss | Exam is paused behind a blocking overlay |
| `Ctrl+Shift+F10` while paused | Invigilator password prompt; correct password resumes the exam and resets warnings |
| `Ctrl+Shift+F12` | Invigilator password prompt; correct password safely closes the exam session |

Each warning, pause, resume, and authorized end action is written to `exam-security.log` beside the executable.

## 13. Security limitations

This software improves browser-level control, but no ordinary Windows desktop application can fully secure a device by itself. In particular, it cannot reliably block:

- Ctrl+Alt+Del and the Windows secure attention screen
- Windows lock, power, accessibility, and security controls
- An administrator account
- Process termination by a user with sufficient permissions
- Physical access to the computer, external devices, or another computer/camera

For real supervised exams, use a dedicated standard Windows account together with Windows Assigned Access, Shell Launcher, Group Policy/device-management restrictions, controlled removable media, and appropriate invigilation. Test the complete exam setup on a non-production device before use.

## 14. Troubleshooting

| Problem | Check or fix |
|---|---|
| Black/blank browser while starting | The current native build displays a loading screen until the first page completes. If it never finishes, check the internet connection and launch service. |
| “WebView2 Runtime is required” | Install Microsoft Edge WebView2 Runtime matching the selected architecture. |
| “Could not retrieve the exam URL” | Verify internet access and the `LaunchConfigUrl` in `exam-config.ini`. |
| “Invalid URL” or invalid response | Confirm the launch endpoint returns `success: true` and a valid `http`/`https` URL. |
| Required exam resources are blocked | Add their domains to `AllowedHosts`, separated by commas. |
| Invigilator cannot resume or close | Verify `AdminPasswordSha256` contains the correct uppercase SHA-256 value and recheck the typed password. |
| EXE starts but immediately fails after installer deployment | Confirm `WebView2Loader.dll` and `exam-config.ini` are in the same folder as the EXE. |
| Wrong application architecture | Install and distribute matching x64 or x86 files; do not mix the folders. |

## 15. Alternative WPF implementation

The repository also includes a .NET 8 WPF implementation at the repository root. It uses `exam-config.json` rather than `native/exam-config.ini` and has similar WebView2 navigation controls. The native C++ project is the primary implementation documented for x64/x86 installer distribution.

To publish the WPF implementation separately, install the .NET 8 SDK and use a runtime identifier such as `win-x64` or `win-x86`:

```powershell
dotnet publish -c Release -r win-x64 --self-contained false
dotnet publish -c Release -r win-x86 --self-contained false
```

Do not combine WPF configuration files with the native executable, or native configuration files with the WPF executable.

## 16. Licence and ownership

This project is proprietary and is covered by the terms in `LICENSE.txt`. All rights are reserved. Copying, modifying, distributing, publishing, reverse engineering, sublicensing, selling, or using the software without the copyright owner's written permission is prohibited.

## 17. Release checklist

Before distributing a new exam-browser release:

1. Build the required x64 and/or x86 Release version.
2. Set the production `LaunchConfigUrl`.
3. Set every required `AllowedHosts` domain.
4. Generate and set a strong invigilator password hash.
5. Confirm the launch service returns a valid response.
6. Test navigation to all required exam, login, and resource domains.
7. Test warning, pause, resume, and authorized exit behavior.
8. Confirm the target device has WebView2 Runtime installed.
9. Package only matching architecture files in the installer.
10. Test the complete installer and exam flow on a non-production computer.
