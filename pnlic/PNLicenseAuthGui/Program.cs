using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Threading;
using System.Runtime.InteropServices;

namespace PNLicenseAuthGui
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]

        [DllImport("user32.dll")]
        public static extern IntPtr FindWindow(String sClassName, String sAppName);
        
        [DllImport("user32.dll")]
        static extern bool SetForegroundWindow(IntPtr hWnd);

        static void Main()
        {
            bool isMutex;

            // named mutex를 생성한다.
            Mutex hMutex = new Mutex(true, "PNLicenseAuthGui_Mutex", out isMutex);

            // 생성 잘 됐으면
            if (isMutex)
            {
                // 일반 앱 실행
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new MainForm());

                // 마무리
                hMutex.ReleaseMutex();
            }
            else
            {
                // 기존에 실행중인 앱을 포어그라운드로 내놓는다.
                IntPtr hwnd = FindWindow(null, "ProudNet License Tool");

                if (hwnd != null)
                {
                    SetForegroundWindow(hwnd);
                }
            }
        }
    }
}
