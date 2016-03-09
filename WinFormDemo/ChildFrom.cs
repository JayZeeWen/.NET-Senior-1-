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
    public partial class ChildFrom : Form
    {
        public ChildFrom()
        {
            InitializeComponent();
        }

        public void SetText(string msg)
        {
            this.txtMsg.Text = msg;
        }

        public void AfterParentFormTextChange (object sender ,EventArgs e)
        {
            //拿到父窗体传递的值
            TextBoxMsgChangeEventArg arg = e as TextBoxMsgChangeEventArg;
            this.txtMsg.Text = arg.Text;
        }
    }
}
