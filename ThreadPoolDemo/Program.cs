using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.Remoting.Messaging;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace ThreadPoolDemo
{
    class Program
    {

        static void Main(string[] args)
        {
            //线程池的线程都是后台线程，优势：线程可以进行重用，任务执行完成后并不会马上释放对象
            //启动一个线程：开辟一个内存空间，1M内存。
            //在线程数量非常多的时候，操作系统在切换线程上会耗费较多的计算。
            //同时，线程的创建同样消耗资源
            //ThreadPool.QueueUserWorkItem(s => 
            //{
            //    Console.WriteLine(s);
            //},"ssssssssss");
            #region 线程池优势演示
            //Stopwatch sw = new Stopwatch();
            //sw.Start();
            //for (var i = 0; i < 100; i++ )
            //{
            //    new Thread(() => 
            //    {
            //        int i1 = 0;
            //        //Console.WriteLine(++i1);
            //        i1++;
            //    }).Start();

            //}
            //sw.Stop();
            //Console.WriteLine(sw.Elapsed.TotalSeconds);
            //sw.Restart();
            //for (var i = 0; i < 100; i++ )
            //{
            //    ThreadPool.QueueUserWorkItem(s =>
            //    {
            //        int i1= 0 ;
            //        Console.WriteLine(Thread.CurrentThread.ManagedThreadId);
            //        //Console.WriteLine(++i1);
            //        i1++;
            //    });
            //}
            //sw.Stop();
            //Console.WriteLine(sw.Elapsed.TotalSeconds);
            #endregion

            #region 异步委托

            //Console.WriteLine("Main Thread:" + Thread.CurrentThread.ManagedThreadId);

            //Func<int, int, string> delFunc = (a, b) => 
            //{
            //    Console.WriteLine("Delegate Thread :" + Thread.CurrentThread.ManagedThreadId);
            //    return (a + b).ToString(); 
            //};
            ////string str = delFunc(3, 4);

            ////异步调用委托
            ////delFunc.BeginInvoke(3, 4, null, null);
            ////Console.WriteLine(str);
            ////得到异步委托的返回值
            ////EndInvoke会阻塞当前执行线程
            //IAsyncResult result = delFunc.BeginInvoke(3, 4, null, null);
            //string str = delFunc.EndInvoke(result);
            //Console.WriteLine(str);

            #endregion

            #region 有回调函数的异步委托

            Func<int, int, string> delFunc = (a, b) =>
            {
                Console.WriteLine("Delegate Thread :" + Thread.CurrentThread.ManagedThreadId);
                return (a + b).ToString();
            };

            delFunc.BeginInvoke(6, 6, MyAsycCallBack, "123");

            #endregion
            Console.ReadKey();
        }

        //回调函数在异步委托方法执行完成后执行
        static void MyAsycCallBack(IAsyncResult ar)
        {
            //拿到异步委托的执行结果
            AsyncResult result = (AsyncResult)ar;

            var del = (Func<int, int, string>)result.AsyncDelegate;
            string value =  del.EndInvoke(result);
            Console.WriteLine(value);

            //拿到传给回调函数的参数
            Console.WriteLine(result.AsyncState);

            Console.WriteLine("回调函数的线程id" + Thread.CurrentThread.ManagedThreadId);
        }
    }
}
