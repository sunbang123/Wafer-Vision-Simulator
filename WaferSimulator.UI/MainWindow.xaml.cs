using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Runtime.InteropServices;

namespace WaferSimulator.UI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        // 빌드된 DLL의 경로와 함수명 지정
        private const string DllPath = "WaferSimulator.Core.dll";
        [DllImport(DllPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Add(int a, int b);

        [DllImport(DllPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern int GetStatusCode();

        public MainWindow()
        {
            InitializeComponent();
        }
        private void BtnCalculate_Click(object sender, RoutedEventArgs e)
        {
            // 유효성 검사: 입력 값이 올바른 숫자인지 확인
            if (!int.TryParse(InputA.Text, out int numA) || !int.TryParse(InputB.Text, out int numB))
            {
                MessageBox.Show("입력창에 올바른 정수 숫자를 입력해 주세요.", "입력 오류", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            int sum = Add(numA, numB);
            int code = GetStatusCode();

            // 결과를 UI에 표시
            TxtSumResult.Text = $"연산 결과: Add({numA}, {numB}) = {sum}";
            TxtStatusResult.Text = $"엔진 상태 코드: {code}";
        }
    }
}