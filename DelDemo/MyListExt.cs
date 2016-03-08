using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DelDemo
{
    public static class MyListExt
    {
        public static List<string> MyWhere(this List<string> list,Func<string,bool> FuncWhere)
        {
            List<string> result = new List<string> ();
            foreach(var item in list)
            {
                if(FuncWhere(item))
                {
                    result.Add(item);
                }
            }
            return result;
        }
    }
}
