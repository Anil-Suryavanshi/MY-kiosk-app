# Secure Exam Browser - Complete Documentation

## Table of Contents
1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [Features](#features)
4. [Security Considerations](#security-considerations)
5. [Installation & Setup](#installation--setup)
6. [Configuration](#configuration)
7. [Usage](#usage)
8. [API & Components](#api--components)
9. [Deployment](#deployment)
10. [Troubleshooting](#troubleshooting)
11. [FAQ](#faq)

---

## Project Overview

**Secure Exam Browser** is a specialized Windows desktop application designed to create a secure, controlled environment for online exam administration. It's available in two implementations:

- **WPF + WebView2** (Modern .NET 8 version)
- **C++/Win32** (Native implementation)

Both versions provide a kiosk-mode browser that prevents exam cheating through advanced lockdown features and restricted navigation capabilities.

### Key Goals
- Prevent unauthorized access to external resources during exams
- Restrict system shortcuts and escape routes
- Enable administrator control and monitoring
- Maintain a distraction-free exam environment

---

## Architecture

### WPF Version (Modern)
```
SecureExamBrowser (WPF)
├── MainWindow.xaml / MainWindow.xaml.cs
│   ├── Browser Control (WebView2)
│   ├── Loading Overlay
│   └── Status Bar
├── ExamConfiguration (Config Loader)
├── ExitPrompt (Password Dialog)
└── exam-config.json (Configuration File)
```

### C++ Version (Native)
```
SecureExamBrowserNative (Win32)
├── Main Application Window
├── WebView2 Host
├── Configuration Manager
├── Security Logger
└── exam-config.ini (Configuration File)
```

### Core Dependencies
- **Microsoft Edge WebView2 Runtime**: Browser engine and rendering
- **.NET 8 SDK** (WPF version): Application framework
- **Windows 10/11**: Operating system requirement
- **Visual Studio 2022** (C++ version): Development and compilation

---

## Features

### Browser Lockdown
| Feature | Status | Description |
|---------|--------|-------------|
| **Borderless Window** | ✓ | No window frame or title bar |
| **Maximized Display** | ✓ | Full-screen exclusive mode |
| **Topmost Window** | ✓ | Always on top, cannot be covered |
| **Hidden Taskbar** | ✓ | Does not appear in Windows taskbar |
| **Developer Tools Disabled** | ✓ | F12, Dev Tools blocked |
| **Context Menus Disabled** | ✓ | Right-click menu suppressed |
| **Zoom Control Disabled** | ✓ | Ctrl+/-, Ctrl+Wheel blocked |
| **Browser Shortcuts Disabled** | ✓ | Ctrl+L, Ctrl+N, etc. blocked |

### Navigation Control
- **URL Whitelist**: Only permitted hosts can be accessed
- **HTTPS Enforcement**: HTTP requests blocked (supports HTTPS only)
- **Popup Blocking**: New window requests rejected
- **Subdomain Support**: Wildcards for allowed hosts (e.g., `*.exam.edu`)

### Administrator Controls
| Shortcut | Action | Requirement |
|----------|--------|-------------|
| `Ctrl+Shift+F10` | Request Exit | Admin password required |
| `Ctrl+Shift+F12` | Force Session End | Admin password required (C++ version) |

### Session Management (C++ Version)
- **Warning System**: 
  - First two lockdown attempt triggers warnings
  - Third attempt triggers exam pause overlay
- **Resume Capability**: Invigilator can resume with password
- **Logging**: All admin actions logged to `exam-security.log`
- **Graceful Shutdown**: Safe exit without requiring Task Manager

---

## Security Considerations

### ⚠️ Security Boundaries
The application operates **within user-mode limitations**. The following **cannot be blocked** by any user-mode application:

- `Ctrl+Alt+Del` (Secure Attention Sequence)
- `Win+L` (Windows Lock)
- `Win+X` (Power Menu)
- `Alt+Tab` (Task Switcher)
- Physical process termination via Task Manager
- Administrator override capabilities
- Hardware-level interrupts

### Recommended Deployment Practices
1. **Run as Standard User**: Never run as Administrator
2. **Physical Supervision**: Require exam invigilator presence
3. **Hardware Controls**: Disable USB ports, disable physical media
4. **Network Isolation**: Consider segmented network for exams
5. **Strong Passwords**: Use cryptographically strong admin passwords
6. **Regular Updates**: Keep WebView2 Runtime and Windows updated
7. **Backup Exit Method**: Have failsafe shutdown procedures

### Not Suitable For
- High-stakes, high-security exams without additional controls
- Unsupervised, unattended testing environments
- Environments where administrator access cannot be restricted

---

## Installation & Setup

### System Requirements
- **OS**: Windows 10 (build 1809+) or Windows 11
- **RAM**: 2 GB minimum
- **Storage**: 500 MB available space
- **Network**: Stable internet connection required

### WPF Version Setup

#### Step 1: Install Dependencies
```powershell
# Install .NET 8 SDK (if not already installed)
# Download from https://dotnet.microsoft.com/download

# Install WebView2 Runtime
# Download from https://developer.microsoft.com/microsoft-edge/webview2/
```

#### Step 2: Prepare Deployment Files
```bash
# Clone repository
git clone https://github.com/Anil-Suryavanshi/MY-kiosk-app.git
cd MY-kiosk-app
```

#### Step 3: Publish Application
```powershell
# Publish as self-contained executable
dotnet publish -c Release -r win-x64 --self-contained false

# Output directory: bin\Release\net8.0-windows\win-x64\publish
```

#### Step 4: Deploy to Target Machines
- Copy entire `publish` folder to client machine
- Place `exam-config.json` in same directory as executable
- No installer or registry modifications required

### C++ Version Setup

#### Step 1: Open Project
- Launch **Visual Studio 2022**
- Open `SecureExamBrowserNative.vcxproj`
- Allow NuGet package restore (automatic)

#### Step 2: Build Configuration
- Select **x64** platform
- Choose **Release** configuration
- Enable **WebView2 NuGet support**

#### Step 3: Build Application
```
Build → Build Solution
```

#### Step 4: Prepare for Distribution
- Locate compiled `.exe` in `x64\Release\`
- Ensure `exam-config.ini` is in same directory
- Copy both to distribution location

---

## Configuration

### WPF Version: exam-config.json

```json
{
  "StartUrl": "https://your-exam-portal.example/start",
  "AllowedHosts": [
    "your-exam-portal.example",
    "api.exam-provider.com",
    "cdn.example.com"
  ],
  "ExitPasswordHash": "REPLACE_WITH_SHA256_UPPERCASE_HEX"
}
```

#### Configuration Properties

| Property | Type | Description | Example |
|----------|------|-------------|---------|
| `StartUrl` | String | Initial page loaded on startup | `https://exam.edu/login` |
| `AllowedHosts` | Array | List of permitted domains | `["exam.edu", "cdn.example.com"]` |
| `ExitPasswordHash` | String | SHA-256 hash of admin password (uppercase hex) | `A1B2C3D4...` |

#### Subdomain Matching
- `exam.edu` matches `exam.edu` and `*.exam.edu`
- `example.com` matches `example.com` and `test.example.com`
- No wildcards needed; automatic subdomain support

### C++ Version: exam-config.ini

```ini
[Settings]
StartUrl=https://your-exam-portal.example/start
AllowedHosts=your-exam-portal.example,api.exam-provider.com,cdn.example.com
AdminPasswordSha256=REPLACE_WITH_SHA256_HEX
```

### Generating Password Hash

#### PowerShell (WPF Version)
```powershell
$text = 'your-strong-invigilator-password'
([System.BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash([Text.Encoding]::UTF8.GetBytes($text)))).Replace('-', '')
```

#### PowerShell (C++ Version)
```powershell
$password = 'your-strong-invigilator-password'
([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($password))))
```

#### Python Alternative
```python
import hashlib
password = 'your-strong-invigilator-password'
hash_result = hashlib.sha256(password.encode()).hexdigest().upper()
print(hash_result)
```

### Password Requirements
- ✓ **Minimum 12 characters** (recommended)
- ✓ **Mix of uppercase, lowercase, numbers, and symbols**
- ✓ **No common words or dictionary attacks**
- ✗ **Never use default password** ("admin") in production
- ✗ **Never share password with exam candidates**

---

## Usage

### Starting an Exam Session

1. **Launch Application**
   - Double-click executable
   - Application loads configured `StartUrl`
   - Loading overlay displays until browser ready

2. **Perform Login** (if required by your exam portal)
   - Enter candidate credentials
   - Confirm identity/biometric verification if applicable

3. **Begin Exam**
   - Start answering questions
   - All navigation restricted to allowed hosts
   - All system shortcuts blocked

### During Exam Session

#### For Candidates
| Action | Result |
|--------|--------|
| `Alt+Tab` | **Blocked** - Focus returns to application |
| `Win+L` | **Not Blocked** - Window loses focus (system control) |
| `Ctrl+L` | **Blocked** - Cannot change URL |
| `F11`, `F12`, `Escape` | **Blocked** - Cannot escape fullscreen |
| Minimize/Close | **Blocked** - Window cannot be minimized or closed |
| Navigate to blocked URL | **Blocked** - Navigation cancelled, stays on current page |

#### For Invigilators (Administrators)
- **Monitor Screen**: Observe exam progress and student activity
- **Pause Exam** (3+ lockdown attempts): Manually intervene if needed
  - Press `Ctrl+Shift+F10`
  - Enter admin password
  - Application pauses exam and displays overlay (C++ version)
- **Resume Exam**: Re-enter password to continue
- **End Session**: Press `Ctrl+Shift+F12` (C++ version) to safely terminate

### Exiting Exam

#### Normal Exit (End of Exam)
1. Candidate or invigilator triggers exit:
   - Press `Ctrl+Shift+F10`
2. Password dialog appears
3. Enter admin password
4. Click "Exit Exam"
5. Application closes cleanly

#### Emergency Exit
- **Last Resort**: `Ctrl+Shift+F12` (C++ version only)
- Logs event and terminates session
- Prevents need for Task Manager or system restart

---

## API & Components

### Core Classes (WPF Version)

#### MainWindow.xaml.cs
```csharp
public partial class MainWindow : Window
{
    private readonly ExamConfiguration _configuration;
    private bool _authorisedExit;
    private bool _browserReady;

    public MainWindow()          // Initialize window and load config
    public void ConfigureBrowser(CoreWebView2 webView)  // Disable browser features
    private bool IsAllowedUri(string candidate)         // Validate navigation URLs
    private void Window_PreviewKeyDown(KeyEventArgs e)  // Block restricted shortcuts
    private void RequestExit()   // Show password prompt for exit
}
```

#### ExamConfiguration
```csharp
public sealed class ExamConfiguration
{
    public string StartUrl { get; init; }           // Initial URL to load
    public List<string> AllowedHosts { get; init; } // Whitelist of allowed domains
    public string ExitPasswordHash { get; init; }   // SHA-256 hash of admin password

    public static ExamConfiguration Load()  // Load from exam-config.json
}
```

#### Key Methods Explained

**ConfigureBrowser(CoreWebView2)**
- Disables DevTools (`AreDevToolsEnabled = false`)
- Disables context menus (`AreDefaultContextMenusEnabled = false`)
- Disables keyboard shortcuts (`AreBrowserAcceleratorKeysEnabled = false`)
- Disables zoom controls (`IsZoomControlEnabled = false`)
- Blocks popup windows (`NewWindowRequested` event handled)

**IsAllowedUri(string)**
- Validates URI scheme (HTTP/HTTPS only)
- Checks against whitelist of allowed hosts
- Supports subdomain matching automatically
- Returns `false` if navigation should be blocked

**Window_PreviewKeyDown()**
- Catches all keyboard events before they reach child controls
- Blocks F11, F12, Escape
- Blocks all Ctrl+*, Alt+*, Win+* combinations (except authorized ones)
- Allows only `Ctrl+Shift+F10` for admin exit

### Event Flow

```
Application Start
  ↓
Load exam-config.json
  ↓
Initialize MainWindow
  ↓
Window Loaded event
  ↓
Initialize WebView2 (EnsureCoreWebView2Async)
  ↓
Configure browser restrictions (ConfigureBrowser)
  ↓
Navigate to StartUrl
  ↓
Display full-screen kiosk
  ↓
Monitor keyboard/navigation events
  ↓
On Ctrl+Shift+F10 → Show password dialog
  ↓
If password correct → Exit; else → Continue exam
```

---

## Deployment

### Production Deployment Checklist

#### Pre-Deployment
- [ ] Generate strong admin password and hash
- [ ] Update `StartUrl` to real exam portal
- [ ] Update `AllowedHosts` with all required domains
- [ ] Test all exam portal features on all allowed hosts
- [ ] Test exit procedure with admin password
- [ ] Verify WebView2 Runtime is installed on target machine
- [ ] Test on target operating system (Windows 10/11)
- [ ] Create backup of configuration files

#### Deployment
- [ ] Deploy application folder to client machines
- [ ] Deploy `exam-config.json` alongside executable
- [ ] Verify file permissions are set correctly
- [ ] Test application starts and loads StartUrl
- [ ] Confirm password works for exit

#### Post-Deployment
- [ ] Run test exam with sample candidate
- [ ] Verify all lockdown features working
- [ ] Confirm exam data submission successful
- [ ] Test invigilator password and exit procedure
- [ ] Document any environmental issues
- [ ] Create runsheet for invigilators

### Distribution Package Contents

For each client machine:
```
exam-browser/
├── SecureExamBrowser.exe          (WPF executable)
├── exam-config.json               (Configuration - KEEP SECURE)
├── webview2/                       (WebView2 files)
├── runtimes/                       (.NET runtime files)
└── README.txt                      (Quick start guide)
```

### Automated Deployment (Group Policy / MDM)
```powershell
# Deploy via PowerShell script
$appFolder = "C:\Program Files\SecureExamBrowser"
Copy-Item -Path "\\deployment-share\SecureExamBrowser" -Destination $appFolder -Recurse
Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\Run" -Name "ExamBrowser" -Value "$appFolder\SecureExamBrowser.exe"
```

---

## Troubleshooting

### Common Issues & Solutions

#### Issue: Application Won't Start
**Symptom**: Executable launches but immediately closes

**Solutions**:
1. Verify WebView2 Runtime installed:
   ```powershell
   # Check registry
   Get-ItemProperty "HKLM:\SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
   ```
2. Reinstall WebView2 Runtime
3. Check `exam-config.json` is valid JSON:
   ```powershell
   Get-Content "exam-config.json" | ConvertFrom-Json
   ```

#### Issue: Configuration Not Loading
**Symptom**: Application uses default settings instead of exam-config.json

**Solutions**:
1. Verify `exam-config.json` is in same directory as .exe
2. Verify JSON syntax is valid (use online JSON validator)
3. Check file permissions allow read access
4. Ensure JSON uses UTF-8 encoding (no BOM)

#### Issue: StartUrl Doesn't Load
**Symptom**: Browser stays on blank/loading screen

**Solutions**:
1. Verify URL is correct and accessible from test machine
2. Check firewall/proxy allows access to that domain
3. Verify domain is in `AllowedHosts` whitelist
4. Test URL in standard browser first
5. Check for HTTPS requirement (non-HTTPS URLs blocked)

#### Issue: Cannot Exit Exam
**Symptom**: Ctrl+Shift+F10 doesn't open password dialog

**Solutions**:
1. Verify keyboard event is reaching application (try Alt+Tab instead)
2. Ensure NumLock is off (can interfere with F10)
3. Verify admin password hash is correct
4. Try alternative keyboard layout if using non-US keyboard
5. Restart application if frozen

#### Issue: Password Not Accepted
**Symptom**: Correct password rejected by exit dialog

**Solutions**:
1. Verify password hash is correct format (uppercase hex, no dashes)
2. Regenerate hash for the exact password being used
3. Check for extra whitespace in config file
4. Verify hash wasn't truncated or corrupted

#### Issue: Navigation to Blocked Domain Fails Silently
**Symptom**: Link click or navigation attempt does nothing

**Solutions**:
1. Verify domain is in `AllowedHosts` list
2. Check if domain is subdomain; parent domain may need addition
3. Confirm URL uses HTTP or HTTPS (not FTP, file://, etc.)
4. Test URL directly in address bar
5. Check browser console (if accessible) for error messages

### Debug Mode (Development)
To enable debug logging:
1. Rebuild with debug configuration: `dotnet publish -c Debug`
2. Check Output window in Visual Studio for messages
3. Examine WebView2 process in Process Explorer
4. Enable WebView2 logging via environment variable:
   ```powershell
   $env:WEBVIEW2_USER_DATA_FOLDER = "C:\temp\webview2debug"
   ```

### Logs & Monitoring

#### WPF Version
- No built-in logging (stateless application)
- Implement custom logging if audit trail required:
  ```csharp
  File.AppendAllText("exam-log.txt", $"[{DateTime.Now}] Event: {details}");
  ```

#### C++ Version
- Automatic logging to `exam-security.log` alongside executable
- Contains timestamps and all admin actions
- Review after exam for compliance/incidents

---

## FAQ

### General Questions

**Q: Can I use this for high-stakes, high-security exams?**
A: This application provides application-level lockdown only. For high-stakes exams, combine with: physical supervision, network isolation, hardware controls, and consider additional browser-based security features provided by the exam platform itself.

**Q: Is this compatible with macOS or Linux?**
A: No. WebView2 and the lockdown features are Windows-specific. Consider alternative solutions like Respondus LockDown Browser or web-based exam proctoring for cross-platform support.

**Q: Can candidates see the password or hack it?**
A: No. The password is never displayed, only its SHA-256 hash is stored. Candidates see only a password input dialog (masked input). However, if someone has physical access to the config file, they could theoretically regenerate hashes; restrict physical access accordingly.

**Q: What if a candidate closes the browser using Alt+F4?**
A: Alt+F4 is blocked. The close button is disabled. Close requests go through `Window_Closing()` which requires `_authorisedExit` flag set by successful password entry.

### Technical Questions

**Q: Can I customize the UI (colors, logo, text)?**
A: Yes. For WPF version:
- Edit `MainWindow.xaml` for layout/colors
- Modify `StatusText.Text` for custom messages
- Add company branding in XAML
For C++ version, modify Win32 resource files.

**Q: Can I add support for third-party authentication (SSO, SAML)?**
A: Yes. The exam portal (StartUrl) handles authentication. The browser simply displays whatever HTML is served. If your exam platform supports SSO, it will work transparently.

**Q: What's the performance impact on exam systems?**
A: Minimal. WebView2 is lightweight (~50-100 MB RAM). Application overhead is <20 MB. Suitable for older machines with 2 GB RAM minimum.

**Q: Can I monitor exam activity remotely?**
A: Not built-in. Implement by: (1) Having exam platform log all activity server-side, (2) Using remote desktop/supervision software (with care not to expose passwords), or (3) Recording screen on invigilator device.

### Security Questions

**Q: What if I forget the admin password?**
A: You would need to modify the config file (requires physical access) or redeploy with a new hash. No built-in password recovery. **Recommendation**: Store password securely with IT department or test with temporary password first.

**Q: Can I use the same password for all exam sessions?**
A: Yes, it's common. However, for added security, rotate passwords between exam periods or use different passwords for different exam levels.

**Q: Is Windows Sandbox / Virtual Machine supported?**
A: Yes. Application runs in VM environments. Note: VM guest/host clipboard sharing could be a security concern; disable in VM settings.

**Q: What happens if the network disconnects during exam?**
A: The browser remains open on last loaded page. Candidate can attempt to reconnect when network resumes. This is determined by exam portal design, not the browser application.

---

## Support & Contact

For issues or feature requests:
- **Repository**: https://github.com/Anil-Suryavanshi/MY-kiosk-app
- **Documentation**: See included docs and README.md
- **Issue Reporting**: Create issue on GitHub with:
  - Windows version
  - .NET version (WPF) or Visual Studio version (C++)
  - Detailed error message or screenshot
  - Steps to reproduce

---

## Version History

| Version | Release Date | Changes |
|---------|--------------|---------|
| 1.0 | Initial | WPF + WebView2 implementation |
| 1.1 | - | C++ native implementation added |

---

## License

See LICENSE file in repository for terms and conditions.

---

**Last Updated**: August 2026
**Maintained By**: Anil Suryavanshi
