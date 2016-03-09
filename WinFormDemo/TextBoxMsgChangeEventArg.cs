using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WinFormDemo
{
    class TextBoxMsgChangeEventArg :EventArgs
    {
        public string Text { get; set; }
    }
}
