#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "../comm/log.hpp"
#include "../comm/tool.hpp"

namespace ns_runner
{
    using namespace ns_tool;
    using namespace ns_log;

    class Runner
    {
    public:
        Runner() {}
        ~Runner() {}

    public:
        //提供设置进程占用资源大小的接口
        static void SetProcLimit(int _cpu_limit, int _mem_limit)
        {
            // 设置CPU时长
            struct rlimit cpu_rlimit;
            cpu_rlimit.rlim_max = RLIM_INFINITY;
            cpu_rlimit.rlim_cur = _cpu_limit;
            setrlimit(RLIMIT_CPU, &cpu_rlimit);

            // 设置内存大小
            struct rlimit mem_rlimit;
            mem_rlimit.rlim_max = RLIM_INFINITY;
            mem_rlimit.rlim_cur = _mem_limit * 1024; //转化成为KB
            setrlimit(RLIMIT_AS, &mem_rlimit);
        }
        static int Run(const std::string &file_name, int cpu_limit, int mem_limit)
        {
            std::string _execute = PathUtil::Exe(file_name);
            std::string _stdin   = PathUtil::Stdin(file_name);
            std::string _stdout  = PathUtil::Stdout(file_name);
            std::string _stderr  = PathUtil::Stderr(file_name);

            umask(0);
            int _stdin_fd = open(_stdin.c_str(), O_CREAT|O_RDONLY, 0644);
            int _stdout_fd = open(_stdout.c_str(), O_CREAT|O_WRONLY, 0644);
            int _stderr_fd = open(_stderr.c_str(), O_CREAT|O_WRONLY, 0644);

            if(_stdin_fd < 0 || _stdout_fd < 0 || _stderr_fd < 0){
                LOG(ERROR) << "运行时打开标准文件失败" << "\n";
                return -1; //代表打开文件失败
            }            

            pid_t pid = fork();
            if (pid < 0)
            {
                LOG(ERROR) << "运行时创建子进程失败" << "\n";
                close(_stdin_fd);
                close(_stdout_fd);
                close(_stderr_fd);
                return -2; //代表创建子进程失败
            }
            else if (pid == 0)
            {
                dup2(_stdin_fd, 0);
                dup2(_stdout_fd, 1);
                dup2(_stderr_fd, 2);

                SetProcLimit(cpu_limit, mem_limit);
                
                execl(_execute.c_str()/*我要执行谁*/, _execute.c_str()/*我想在命令行上如何执行该程序*/, nullptr);
                exit(1);
            }
            else
            {
                close(_stdin_fd);
                close(_stdout_fd);
                close(_stderr_fd);
                int status = 0;
                waitpid(pid, &status, 0);
                // 程序运行异常，一定是因为因为收到了信号！
                LOG(INFO) << "运行完毕, info: " << (status & 0x7F) << "\n"; 
                return status & 0x7F;
            }
        }
    };
}