using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DelDemo
{
    class Program
    {
        public delegate int AddDel(int a, int b);
        static void Main(string[] args)
        {
            #region 委托 基本复习
            //AddDel del = new AddDel(Add);
            //Program p = new Program();
            //del += p.AddInstancedFuc;
            //int result = del(3, 5);
            //Console.WriteLine(result);
            #endregion 

            #region 泛型委托

            //Func<int, int> FucnDemo = new Func<int, int>(Double);
            //int result = FucnDemo(5);
            //Console.WriteLine(result);
            //#endregion 

            //#region 匿名方法
            //Func<int, int> FDemo = delegate(int a) { return a * 2; };
            //int r = FDemo(3);
            //Console.WriteLine(r);

            //Func<int, int> Func = (int a) => { return a * a; };
            //int f = Func(3);
            //Console.WriteLine(f);

            //Func<int, int> FuncD = (int a) => a * 10 ;
            //int r1 = FuncD(3);
            //Console.WriteLine(r1);

            #endregion

            #region 案例

            List<string> strList = new List<string>() { "3","6","9","11"};

            //取出小于"6"的然后打印
            //var temp = strList.Where(delegate(string a) { return a.CompareTo("6") < 0; });
            var temp = strList.MyWhere(delegate(string a) { return a.CompareTo("6") < 0; });
            foreach(var item  in  temp)
            {
                Console.WriteLine(item);
            }


            #endregion 
            Console.ReadKey();
        }

        public static int Add(int a, int b)
        {
            return a + b;
        }

        public int AddInstancedFuc(int a ,  int b)
        {
            return a + b + 2;
        }

        static int Double(int a)
        {
            return a * 2;
        }
    
    }
}
