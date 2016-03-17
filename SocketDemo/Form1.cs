using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace SocketDemo
{
    public partial class Form1 : Form
    {
        List<Socket> ClientProxSocket = new List<Socket>();

        public Form1()
        {
            InitializeComponent();
            Control.CheckForIllegalCrossThreadCalls = false;
        }

        private void btnStart_Click(object sender, EventArgs e)
        {
            //Socket服务器端的逻辑
            //1 创建socket对象   第一个参数：网络的寻址协议   第二个参数：数据传输方式  三：通信协议
            Socket serverSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            this.txtLog.Text = "创建服务器端SOCKET对象\r\n" + this.txtLog.Text;

            //2 绑定ip端口
            IPAddress ip = IPAddress.Parse(txtIPAddress.Text);
            IPEndPoint ipEndPoint = new IPEndPoint(ip, int.Parse(txtPort.Text));
            serverSocket.Bind(ipEndPoint);

            //3开启侦听
            serverSocket.Listen(10);

            //4开始接受客户端连接   会阻塞当前线程，直到客户端连接上
            this.txtLog.Text = "开始接受\r\n" + this.txtLog.Text;
            ThreadPool.QueueUserWorkItem(new WaitCallback(StartAcceptCient), serverSocket);


        }        

        private void btnSend_Click(object sender, EventArgs e)
        {
            foreach (Socket client in ClientProxSocket)
            {
                if (client.Connected)
                {
                    string str = this.txtInfo.Text;
                    byte[] data = Encoding.Default.GetBytes(str);
                    client.Send(data, 0, data.Length, SocketFlags.None);
                }
            }
        }

        public void StartAcceptCient(object state)
        {
            var serverSocket = (Socket)state;
            while (true)
            {
                Socket proxSocket = serverSocket.Accept();
                this.txtLog.Text = string.Format("一个客户端{0}连接上\r\n", proxSocket.RemoteEndPoint.ToString()) + this.txtLog.Text;
                ClientProxSocket.Add(proxSocket);

                //服务器端接受客户端消息
                ThreadPool.QueueUserWorkItem(new WaitCallback(RecieveData), proxSocket);
            }
        }

        private void RecieveData(object state)
        {
            var proxSocket = (Socket)state;
            byte[] data = new byte[1024 * 1024];
            while (true)
            {
                //实际接受的数据长度
                int realLen = proxSocket.Receive(data, 0, data.Length, SocketFlags.None);
                if (realLen == 0)
                {
                    //客户端终止连接
                    this.txtLog.Text = string.Format("接受到客户端{0}  终止连接 \r\n", proxSocket.RemoteEndPoint.ToString()) + this.txtLog.Text;
                    proxSocket.Shutdown(SocketShutdown.Both);
                    proxSocket.Close();
                    ClientProxSocket.Remove(proxSocket);
                    return;                    
                }
                string fromClientMsg = Encoding.Default.GetString(data, 0, realLen);

                this.txtLog.Text = string.Format("接受到客户端{0}  的消息 {1} \r\n", proxSocket.RemoteEndPoint.ToString(), fromClientMsg) + this.txtLog.Text;

            }
        }
    }
}
