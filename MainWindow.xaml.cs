using Microsoft.Web.WebView2.Core;
using System.ComponentModel;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Net.Http.Json;
using System.Windows.Input;

namespace SecureExamBrowser;

public partial class MainWindow : Window
{
    private readonly ExamConfiguration _configuration;
    private bool _authorisedExit;
    private bool _browserReady;
    private string? _sessionAllowedHost;

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
            StatusText.Text = "Getting exam URL…";
            var targetUrl = await GetExamUrlAsync();
            Browser.CoreWebView2.Navigate(targetUrl);
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

    private async Task<string> GetExamUrlAsync()
    {
        using var client = new HttpClient();
        var response = await client.GetFromJsonAsync<LaunchResponse>(_configuration.LaunchConfigUrl);
        if (response is not { Success: true } || string.IsNullOrWhiteSpace(response.Url))
            throw new InvalidOperationException("The exam URL service returned an invalid response.");
        if (!Uri.TryCreate(response.Url, UriKind.Absolute, out var uri) || uri.Scheme is not ("https" or "http"))
            throw new InvalidOperationException("The exam URL service returned an invalid URL.");
        _sessionAllowedHost = uri.Host;
        return response.Url;
    }

    private bool IsAllowedUri(string candidate)
    {
        if (!Uri.TryCreate(candidate, UriKind.Absolute, out var uri)) return false;
        if (uri.Scheme is not ("https" or "http")) return false;
        if (!string.IsNullOrEmpty(_sessionAllowedHost) && (uri.Host.Equals(_sessionAllowedHost, StringComparison.OrdinalIgnoreCase) ||
            uri.Host.EndsWith($".{_sessionAllowedHost}", StringComparison.OrdinalIgnoreCase))) return true;
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
    public string LaunchConfigUrl { get; init; } = "https://example.org";
    public List<string> AllowedHosts { get; init; } = [];
    public string ExitPasswordHash { get; init; } = "";

    public static ExamConfiguration Load()
    {
        var path = Path.Combine(AppContext.BaseDirectory, "exam-config.json");
        if (!File.Exists(path)) return new ExamConfiguration();
        return System.Text.Json.JsonSerializer.Deserialize<ExamConfiguration>(File.ReadAllText(path)) ?? new ExamConfiguration();
    }
}

internal sealed class LaunchResponse
{
    public bool Success { get; init; }
    public string Url { get; init; } = "";
}
