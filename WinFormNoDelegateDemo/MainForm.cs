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
    public partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            ParentForm p = new ParentForm();
            ChildFrom c = new ChildFrom();
            p.ChildFormList = new List<IChildForm>();
            p.ChildFormList.Add(c);
            p.Show();            
            c.Show();
        }
    }
}
