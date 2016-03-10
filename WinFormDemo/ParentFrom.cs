using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WinFormDemo
{
    

    public partial class ParentFrom : Form
    {
        //1 : 菜鸟级别，将子窗体的属性公开给父窗体访问
        //2 ： 改成委托方式
        //3 : 改成事件方式
        //不使用委托实现观察者模式/发布订阅模式
        //定义发布信息的委托
        public Action<string> AfterMsgSend { get; set; }
        //定义消息发布的事件
        public event EventHandler AfterMsgChangeEvent;

        public ParentFrom()
        {
            InitializeComponent();
        }

        private void ParentFrom_Load(object sender, EventArgs e)
        {
            ChildFrom form = new ChildFrom();
            //子窗体订阅主窗体信息
            //AfterMsgSend += form.SetText;  委托方式

            //事件方式
            AfterMsgChangeEvent += form.AfterParentFormTextChange;
            form.Show();

        }

        private void btnSendMsg_Click(object sender, EventArgs e)
        {
            #region 委托方式
            //if (AfterMsgSend != null)
            //{
            //    AfterMsgSend(this.txtMsg.Text);
            //}
            #endregion 

            #region 事件方式
            AfterMsgChangeEvent(this, new TextBoxMsgChangeEventArg() { Text = this.txtMsg.Text }); 
            #endregion 
        }
    }
}
