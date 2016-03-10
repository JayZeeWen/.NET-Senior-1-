using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WinFormNoDelegateDemo
{
    public partial class ChildFrom : Form, IChildForm
    {
        public ChildFrom()
        {
            InitializeComponent();
        }

        public void SetText(string msg)
        {
            this.txtMsg.Text = msg;
        }
    }
}
