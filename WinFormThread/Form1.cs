using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WinFormThread
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void btnLoop_Click(object sender, EventArgs e)
        {
            Thread thread = new Thread(() => {
                while (true)
                {
                    if(btnLoop.InvokeRequired)//判断是否是其他线程创建的控件 
                    {
                        //寻找创建该控件的线程，执行委托实例
                        btnLoop.Invoke(new Action<string>(s => { this.btnLoop.Text = s; }), DateTime.Now.ToString());
                    }
                    else
                    {
                        this.btnLoop.Text = DateTime.Now.ToString();
                    }
                    Console.WriteLine(DateTime.Now.ToString());
                }
            });

            thread.IsBackground = true;
            thread.Start();

            
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }
    }
}
