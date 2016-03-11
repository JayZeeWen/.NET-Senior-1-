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
    public partial class LuckDraw : Form
    {
        private List<Label> lbList = new List<Label>();

        private Thread threadStard;
        private bool isRunning = false;

        public LuckDraw()
        {
            InitializeComponent();
        }

        private void LuckDraw_Load(object sender, EventArgs e)
        {
            for(int i = 0 ;i< 6;i++)
            {
                Label lb = new Label();
                lb.Text = "0";
                lb.AutoSize = true;
                lb.Location = new Point((i + 1) * 50, 100);
                this.Controls.Add(lb);
                lbList.Add(lb);
            }
        }

        private void btnStart_Click(object sender, EventArgs e)
        {
            Thread thread = new Thread(new ThreadStart(() =>
            {
                Random r = new Random ();
                isRunning = true;
                //Change the number of label 
                while(isRunning)
                {
                    foreach(Label item in lbList)
                    {
                        string str = r.Next(0, 10).ToString();
                        if(item.InvokeRequired)
                        {
                            item.Invoke(new Action<string>(s => {
                                item.Text = str;

                            }),str);
                        }
                        else
                        {
                            item.Text = str;
                        }
                    }
                    Thread.Sleep(200);
                }
            }));

            thread.IsBackground = true;
            thread.Start();
            threadStard = thread;
        }

        private void btnStop_Click(object sender, EventArgs e)
        {
            isRunning = false;
            //if(threadStard == null || !threadStard.IsAlive)
            //{
            //    return;
            //}
            //threadStard.Abort();

        }
    }
}
