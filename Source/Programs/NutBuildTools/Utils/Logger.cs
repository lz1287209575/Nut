using System;

namespace NutBuildTools.Utils
{
    public static class Logger
    {
        public static void Log(string message)
        {
            Console.WriteLine($"[NutBuildTools] {message}");
        }
    }
} 