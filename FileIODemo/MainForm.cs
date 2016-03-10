using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace FileIODemo
{
    public partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();
        }
        

        private void btnOpenFile_Click(object sender, EventArgs e)
        {
            using(OpenFileDialog ofd = new OpenFileDialog ())
            {
                if(ofd.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                {
                    return;
                }

                //读取文件
                using(FileStream fs = new FileStream (ofd.FileName,FileMode.Open,FileAccess.Read))
                {
                    using(StreamReader sr = new StreamReader (fs))
                    {
                        while (!sr.EndOfStream)
                        {
                            string line = sr.ReadLine();
                            this.txtFileContent.Text += line;
                        }   
                    }
                }
            }
        }

        private void btnSaveFile_Click(object sender, EventArgs e)
        {
            using(SaveFileDialog sfd = new SaveFileDialog ())
            {
                if(sfd.ShowDialog() != DialogResult.OK)
                {
                    return;
                }
                //File.WriteAllText(sfd.FileName, txtFileContent.Text, Encoding.Default);
                //using(StreamWriter sw = new StreamWriter (sfd.FileName,false,Encoding.Default,1024*1024))
                //{
                //    sw.Write(txtFileContent.Text);
                //    sw.Flush();
                //}

                using (FileStream fs = new FileStream (sfd.FileName,FileMode.OpenOrCreate,FileAccess.ReadWrite))
                {
                    string str = txtFileContent.Text;
                    byte[] data = Encoding.Default.GetBytes(str);
                    fs.Write(data, 0, data.Length);
                }
            }
        }
    }
}
