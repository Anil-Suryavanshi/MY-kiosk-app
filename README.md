# Secure Exam Browser (Windows)

WPF + WebView2 kiosk browser starter. It launches borderless, maximized, topmost, and does not appear in the Windows taskbar. Browser developer tools, browser accelerators, context menus, zoom, pop-ups, and non-allowlisted navigation are disabled.

## Configure

1. Install the .NET 8 SDK and Microsoft Edge WebView2 Runtime on each client.
2. Edit `exam-config.json`: set the start URL, permitted portal hosts, and an administrator exit-password SHA-256 hash (uppercase hex).
3. Publish with `dotnet publish -c Release -r win-x64 --self-contained false`.

Generate a hash in PowerShell:

```powershell
$text = 'choose-a-strong-admin-password'
([System.BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash([Text.Encoding]::UTF8.GetBytes($text)))).Replace('-', '')
```

Administrator exit is `Ctrl+Shift+F10`, then the configured password.

## Important security boundary

An ordinary Windows desktop app cannot securely suppress OS-reserved actions such as Ctrl+Alt+Del, Win+L, secure attention screen, or prevent a user from terminating its process. For real supervised exams, deploy this application with a dedicated standard Windows account and Windows **Assigned Access / kiosk mode** or **Shell Launcher**, plus organization device policies (disable Task Manager, command prompt, removable media, etc.). Test the complete policy on a non-production machine before use.
