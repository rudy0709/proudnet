using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;

namespace PNLicenseAuthGui
{
    public partial class LicenseKeyForm : Form
    {
        MainForm m_mainForm;
        public string m_licenseInfoString;

        public void SetMainForm(MainForm form)
        {
            m_mainForm = form;
        }
        public LicenseKeyForm()
        {
            InitializeComponent();
        }

        private void CancelButton_Click(object sender, EventArgs e)
        {
            if (m_mainForm.IsCloseForm())
                this.Close();
        }

        private void OkButton_Click(object sender, EventArgs e)
        {
            if (m_mainForm.ExistLicenseAuthFile() == false)
            {
                return;
            }

            if (LicenseKeyTextBox.Text == "")
            {
                MessageBox.Show("Enter the license key.");
                return;
            }

            this.progressBar1.Visible = true;

            this.Update();

            // 임시 파일에 입력받은 라이선스 텍스트 문구를 파일로 저장한다.
            string fileName = Path.GetTempFileName();

            using (StreamWriter sw = new StreamWriter(fileName, true, Encoding.ASCII))
            {
                sw.Write(LicenseKeyTextBox.Text);
            }

            // 텍스트 파일을 인자로 라이선스키 설치앱을 실행한다.
            m_mainForm.RunNewLicense(fileName);

            OkButton.Enabled = false;
        }

        protected override void WndProc(ref Message message)
        {
            const int WM_SYSCOMMAND = 0x0112;
            const int SC_CLOSE = 0xF060;

            switch (message.Msg)
            {
                case WM_SYSCOMMAND:
                    int command = message.WParam.ToInt32();

                    //Disable the movement of the Form
                    if (command == SC_CLOSE)
                    {
                        if (!m_mainForm.IsCloseForm())
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
