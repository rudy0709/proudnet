using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;

namespace PNLicenseAuthGui
{
    public partial class MainForm : Form
    {
        public bool IsThreadRunning = false;

        LicenseKeyForm KeyForm = null;

        const string LicenseAuthFileName = "PNLicenseAuth.exe";

        // 라이선스를 실행하는 쓰레드가 처리 중일 경우 폼을 종료하지 않도록 한다.
        // 그리고 종료할 수 없다는 폼을 사용자에게 보여준다.
        public bool IsCloseForm()
        {
            bool ret = !IsThreadRunning;

            if (!ret)
            {
                MessageBox.Show("Please wait. Authentication in progress.");
            }

            return ret;
        }

        // PNLicenseInfo.exe가 있는지를 검사하고 없으면 에러를 표시한다.
        public bool ExistLicenseAuthFile()
        {
            string licPath = Application.StartupPath + "\\" + LicenseAuthFileName;

            FileInfo FI = new FileInfo(licPath);

            if (FI.Exists)
            {
                return true;
            }

            MessageBox.Show("File " + LicenseAuthFileName + " does not exist.");

            return false;
        }

        // 라이선스키 설치앱을 실행후 그 결과를 리턴한다.
        public void RunConsoleProcess(string exePath, string param = null)
        {
            IsThreadRunning = true;

            System.Threading.Thread t1 = new System.Threading.Thread
                (delegate()
                {
                    try
                    {
                        string ret, filePath = "";

                        if (param != null)
                        {
                            filePath = param;
                            param = "\"" + param + "\"";
                        }

                        Process authProcess = new Process();
                        authProcess.StartInfo.FileName = exePath;
                        authProcess.StartInfo.Arguments = param;
                        authProcess.StartInfo.UseShellExecute = false;
                        authProcess.StartInfo.CreateNoWindow = true;
                        authProcess.StartInfo.RedirectStandardOutput = true;
                        authProcess.Start();

                        // 콘솔앱의 실행 결과를 얻는다.
                        ret = authProcess.StandardOutput.ReadToEnd();

                        // 프로세스 종료 시까지 기다린다.
                        authProcess.WaitForExit();

                        if (param != null)
                        {
                            // 임시 파일 삭제.
                            File.Delete(filePath);
                        }

                        this.SetLicenseInfo(ret);
                    }
                    catch (Exception)
                    {
                        // 별도 스레드에서 던져지는 예외는 그냥 무시하도록 하자.
                    }
                    finally
                    {
                        IsThreadRunning = false;
                    }
                });

            t1.Start();
        }

        delegate void SetLicenseInfoCallback(string licenseInfo);

        // GUI에 라이선스 정보를 표시한다.
        private void SetLicenseInfo(string licenseInfo)
        {
            if (this.tbLicenseInfo.InvokeRequired)
            {
                SetLicenseInfoCallback d = new SetLicenseInfoCallback(SetLicenseInfo);
                this.Invoke(d, new object[] { licenseInfo });
            }
            else
            {
                this.tbLicenseInfo.Text = licenseInfo;
                this.tbLicenseInfo.SelectionStart = this.tbLicenseInfo.Text.Length;
                this.tbLicenseInfo.ScrollToCaret();

                if (KeyForm != null)
                {
                    KeyForm.Close();
                    KeyForm = null;
                }
            }
        }

        

        public void RunNewLicense(string licenseFile)
        {
            RunConsoleProcess(LicenseAuthFileName, licenseFile);
        }

        public MainForm()
        {
            KeyForm = null;

            InitializeComponent();

            if (ExistLicenseAuthFile() == false)
            {
                // PNLicenseInfo.exe가 없을 경우 메시지 박스 띄우고 종료.
                timer1.Enabled = true;
                return;
            }

            RunConsoleProcess(LicenseAuthFileName);
        }

        private void EnterKeyButton_Click(object sender, EventArgs e)
        {
            KeyForm = new LicenseKeyForm();
            KeyForm.SetMainForm(this);
            KeyForm.ShowDialog();

            KeyForm = null;
        }

        private void CloseButton_Click(object sender, EventArgs e)
        {
            if (IsCloseForm())
            {
                this.Close();
            }
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            // PNLicenseInfo.exe가 없을 경우 메시지 박스 띄우고 종료.
            timer1.Enabled = false;
            this.Close();
        }

        protected override void WndProc(ref Message message)
        {
            const int WM_SYSCOMMAND = 0x0112;
            const int SC_CLOSE = 0xF060;

            switch (message.Msg)
            {
                case WM_SYSCOMMAND:
                    int command = message.WParam.ToInt32();

                    // 워커 스레드가 작동중이면 창을 닫지 않게 한다. 즉 window message를 그냥 드랍한다.
                    if (command == SC_CLOSE)
                    {
                        if (!IsCloseForm())
                            return;
                    }
                    break;

                default:
                    break;
            }

            base.WndProc(ref message);
        }
    }
}
