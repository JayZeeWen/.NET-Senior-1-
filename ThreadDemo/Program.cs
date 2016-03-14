using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace ThreadDemo
{
    class Program
    {
        //Main:程序入口，clr一开始会创建一个默认的主线程（前台），指向Main方法。
        static void Main(string[] args)
        {
            #region 线程
            //Thread thread = new Thread(delegate() { 
            //    while(true)
            //    {
            //        Console.WriteLine(DateTime.Now);
            //        Thread.Sleep(500);
            //    }
            //});
            ////默认是前台线程   当所有的前台线程都结束之后进程才回退出；后台线程不会阻塞进程的退出。
            //thread.IsBackground = true;
            //thread.Start();

            ////主线程可以做其他的事情
            //while(true)
            //{
            //    Console.WriteLine("I am in Main Thread");
            //    Thread.Sleep(500);
            //}

            #endregion

            #region 生产者消费者问题

            //create a pool 
            MyConncetoin[] connectionArray = new MyConncetoin[100];
            //index
            int index = 0;
            //create 10 customer
            for (int i = 0; i < 10; i++)
            {
                // one customer
                Thread thread = new Thread(() =>
                {
                    while (true)
                    {
                        if (index >= 0)
                        {
                            connectionArray[index - 1] = null;
                            Console.WriteLine("create Customer");
                            index--;
                        }
                        Thread.Sleep(200);
                    }
                });
                thread.IsBackground = true;
                thread.Start();
            }

            //create 5 producer
            for (int i = 0; i < 5; i++)
            {
                Thread thread = new Thread(() =>
                {
                    while (true)
                    {
                        if (index < 99)
                        {
                            connectionArray[index] = new MyConncetoin();
                            Console.WriteLine("create Producer");
                            index++;
                        }
                        Thread.Sleep(500);
                    }
                });
                thread.IsBackground = true;
                thread.Start();
            }

            #endregion
            Console.ReadKey();
        }

        public class MyConncetoin
        {

        }

    }
}
