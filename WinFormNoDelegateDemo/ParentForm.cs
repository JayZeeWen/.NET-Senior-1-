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
    public partial class ParentForm : Form
    {
        public List<IChildForm> ChildFormList
        {
            get;
            set;
        }

        public ParentForm()
        {
            InitializeComponent();
        }

        private void btnSendMsg_Click(object sender, EventArgs e)
        {
            //遍历所有关注消息变化的子窗体集合
            //调用集合中每个元素的方法
            

            //if (ChildFormList != null)
            //{
            //    foreach (IChildForm f in ChildFormList)
            //    {
            //        f.SetText(this.txtMsg.Text);
            //    }
            //}

            //减少层次的写法
            if (ChildFormList == null)
            {
                return;
            }
            foreach (IChildForm f in ChildFormList)
            {
                f.SetText(this.txtMsg.Text);
            }

            

        }

        private void ParentForm_Load(object sender, EventArgs e)
        {
            //ChildFrom frm = new ChildFrom();
            //this.ChildFormList = new List<IChildForm>();
            //this.ChildFormList.Add(frm);
            //frm.Show();
        }
    }
}
