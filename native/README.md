# C++ Secure Exam Browser

This is the native C++/Win32 implementation. Open `SecureExamBrowserNative.vcxproj` in Visual Studio 2022, allow NuGet restore, choose **x64** for 64-bit Windows or **Win32** for 32-bit Windows, and build.

From the project root, build from PowerShell with:

```powershell
# x64
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe' .\native\SecureExamBrowserNative.vcxproj /p:Configuration=Release /p:Platform=x64

# x86 (use the Win32 platform name when building)
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe' .\native\SecureExamBrowserNative.vcxproj /p:Configuration=Release /p:Platform=Win32
```

The files to distribute are generated in `x64\Release\` and `Win32\Release\` respectively. Include `SecureExamBrowserNative.exe`, `WebView2Loader.dll`, and `exam-config.ini` from the same output folder.

Before distribution, edit `exam-config.ini` beside the `.exe`:

- Set the real `StartUrl` and every required `AllowedHosts` domain (comma separated).
- Replace the default password hash. In PowerShell, run:

```powershell
$password = 'your-strong-invigilator-password'
([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($password))))
```

Copy the result into `AdminPasswordSha256`. Never use the included `admin` default in an exam.

## Behaviour

- Restricted shortcut/focus-loss attempt: warning 1 and 2.
- Third attempt: browser receives a blocking **Exam paused** overlay.
- An invigilator uses `Ctrl+Shift+F10`, then the password, to resume. Resume resets the warning count to zero and adds an entry to `exam-security.log`.
- An invigilator can safely end the session with `Ctrl+Shift+F12`, then the same password. This avoids needing Task Manager or a restart during testing.

The application blocks common application-level shortcuts and browser escape routes. It cannot securely block Ctrl+Alt+Del, Windows lock/security controls, an administrator, or a person with physical control of the computer. Use a dedicated standard Windows account and kiosk/device policies for supervised production use.
