using Microsoft.Web.WebView2.Core;
using System.ComponentModel;
using System.Security.Cryptography;
using System.Text;
using System.Windows.Input;

namespace SecureExamBrowser;

public partial class MainWindow : Window
{
    private readonly ExamConfiguration _configuration;
    private bool _authorisedExit;
    private bool _browserReady;

    public MainWindow()
    {
        InitializeComponent();
        _configuration = ExamConfiguration.Load();
        Loaded += MainWindow_Loaded;
    }

    private async void MainWindow_Loaded(object sender, RoutedEventArgs e)
    {
        try
        {
            await Browser.EnsureCoreWebView2Async();
            ConfigureBrowser(Browser.CoreWebView2);
            Browser.CoreWebView2.Navigate(_configuration.StartUrl);
            _browserReady = true;
            LoadingOverlay.Visibility = Visibility.Collapsed;
        }
        catch (Exception exception)
        {
            StatusText.Text = $"Browser could not start: {exception.Message}";
        }
    }

    private void ConfigureBrowser(CoreWebView2 webView)
    {
        webView.Settings.AreBrowserAcceleratorKeysEnabled = false;
        webView.Settings.AreDefaultContextMenusEnabled = false;
        webView.Settings.AreDevToolsEnabled = false;
        webView.Settings.IsStatusBarEnabled = false;
        webView.Settings.IsZoomControlEnabled = false;
        webView.NewWindowRequested += (_, args) => args.Handled = true;
        webView.NavigationStarting += (_, args) =>
        {
            if (!IsAllowedUri(args.Uri)) args.Cancel = true;
        };
    }

    private bool IsAllowedUri(string candidate)
    {
        if (!Uri.TryCreate(candidate, UriKind.Absolute, out var uri)) return false;
        if (uri.Scheme is not ("https" or "http")) return false;
        return _configuration.AllowedHosts.Count == 0 || _configuration.AllowedHosts.Any(host =>
            uri.Host.Equals(host, StringComparison.OrdinalIgnoreCase) ||
            uri.Host.EndsWith($".{host}", StringComparison.OrdinalIgnoreCase));
    }

    private void Window_PreviewKeyDown(object sender, KeyEventArgs e)
    {
        // Blocks shortcuts delivered to this window. Ctrl+Alt+Del cannot be blocked by any user-mode app.
        if (e.Key == Key.F11 || e.Key == Key.F12 || e.Key == Key.Escape ||
            (Keyboard.Modifiers & (ModifierKeys.Control | ModifierKeys.Alt | ModifierKeys.Windows)) != ModifierKeys.None ||
            (e.SystemKey == Key.F4 && Keyboard.Modifiers.HasFlag(ModifierKeys.Alt)))
        {
            e.Handled = true;
        }

        if (e.Key == Key.F10 && Keyboard.Modifiers == (ModifierKeys.Control | ModifierKeys.Shift))
        {
            e.Handled = true;
            RequestExit();
        }
    }

    private void Window_PreviewKeyUp(object sender, KeyEventArgs e) => e.Handled = true;

    private void Window_Deactivated(object? sender, EventArgs e)
    {
        if (_browserReady) Activate();
    }

    private void Window_Closing(object? sender, CancelEventArgs e)
    {
        if (!_authorisedExit)
        {
            e.Cancel = true;
            RequestExit();
        }
    }

    private void RequestExit()
    {
        var prompt = new ExitPrompt(_configuration.ExitPasswordHash) { Owner = this };
        if (prompt.ShowDialog() == true)
        {
            _authorisedExit = true;
            Close();
        }
    }
}

internal sealed class ExamConfiguration
{
    public string StartUrl { get; init; } = "https://example.org";
    public List<string> AllowedHosts { get; init; } = [];
    public string ExitPasswordHash { get; init; } = "";

    public static ExamConfiguration Load()
    {
        var path = Path.Combine(AppContext.BaseDirectory, "exam-config.json");
        if (!File.Exists(path)) return new ExamConfiguration();
        return System.Text.Json.JsonSerializer.Deserialize<ExamConfiguration>(File.ReadAllText(path)) ?? new ExamConfiguration();
    }
}
