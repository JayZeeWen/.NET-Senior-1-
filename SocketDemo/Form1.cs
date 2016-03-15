using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace SocketDemo
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void btnStart_Click(object sender, EventArgs e)
        {
            //Socket服务器端的逻辑
            //1 创建socket对象   第一个参数：网络的寻址协议   第二个参数：数据传输方式  三：通信协议
            Socket serverSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

            //2 绑定ip端口
            IPAddress ip = IPAddress.Parse(txtIPAddress.Text);
            //IPEndPoint ipEndPoint = new IPEndPoint();
            //serverSocket.Bind()
        }
    }
}
