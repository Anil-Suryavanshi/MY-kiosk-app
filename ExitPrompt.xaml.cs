using System.Security.Cryptography;
using System.Text;
using System.Windows.Input;

namespace SecureExamBrowser;

public partial class ExitPrompt : Window
{
    private readonly string _passwordHash;

    public ExitPrompt(string passwordHash)
    {
        InitializeComponent();
        _passwordHash = passwordHash;
        Loaded += (_, _) => Password.Focus();
    }

    private void Exit_Click(object sender, RoutedEventArgs e) => TryExit();
    private void Password_KeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key == Key.Enter) TryExit();
    }

    private void TryExit()
    {
        var candidate = Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(Password.Password)));
        if (!string.IsNullOrWhiteSpace(_passwordHash) && CryptographicOperations.FixedTimeEquals(
                Encoding.ASCII.GetBytes(candidate), Encoding.ASCII.GetBytes(_passwordHash.ToUpperInvariant())))
        {
            DialogResult = true;
            return;
        }
        MessageBox.Show(this, "Incorrect password.", "Secure Exam Browser", MessageBoxButton.OK, MessageBoxImage.Warning);
        Password.Clear();
    }
}
