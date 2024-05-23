#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <numeric>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/syscall.h>
#include <sys/resource.h> 
#include <sys/wait.h> 
#include <fcntl.h>
#include <linux/sched.h>
#include <sched.h>
#include <chrono>
#include <ctime>
#include <ratio>
#include <thread>
using namespace std;
using namespace std::chrono;

// can also include #include <linux/sched/types.h>, but then we get known include errors with sched_param being double defined
struct sched_attr {
    uint32_t size;

    uint32_t sched_policy;
    uint64_t sched_flags;

    /* SCHED_NORMAL, SCHED_BATCH */
    int32_t sched_nice;

    /* SCHED_FIFO, SCHED_RR */
    uint32_t sched_priority;

    /* SCHED_DEADLINE (nsec) */
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

#define NSEC_PER_MSEC 1000000

high_resolution_clock::time_point global_start;

#define NUM_PROCS_TO_MAKE 2
#define NUM_LOW_PRIO 0
#define NUM_MIDDLE_PRIO 1

int time_since_start(high_resolution_clock::time_point start) {
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start);
    return 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
}

// 330 ms
int long_fac() {
    
    // int to_factor = 243695353;
    // std::vector<int> factors;

    // for (int i = 1; i <= to_factor; i++) {
    //     if (to_factor % i == 0) {
    //         factors.push_back(i);
    //     }
    // }

    high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    long long sum = 0;
    for (long long i = 0; i < 100000000; i++) {
        sum = 3 * i + 1;
    }

    cout << "long run: " << time_since_start(start) << "ms" << endl;

    return sum;
}

// 170 ms
int mid_fac() {
    // int to_factor = 67264565;
    // std::vector<int> factors;

    // for (int i = 1; i <= to_factor; i++) {
    //     if (to_factor % i == 0) {
    //         factors.push_back(i);
    //     }
    // }

    // return 0;

    high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    long long sum = 0;
    for (long long i = 0; i < 50000000; i++) {
        sum = 3 * i + 1;
    }

    cout << "mid run: " << time_since_start(start) << "ms" << endl;

    return sum;
}

// 50 ms
int short_fac() {
    // int to_factor = 7968253;
    // std::vector<int> factors;

    // for (int i = 1; i <= to_factor; i++) {
    //     if (to_factor % i == 0) {
    //         factors.push_back(i);
    //     }
    // }

    // return 0;
    high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    long long sum = 0;
    for (long long i = 0; i < 10000000; i++) {
        sum = 3 * i + 1;
    }

    cout << "short run: " << time_since_start(start) << "ms" << endl;

    return sum;
}


int main() {

    cout << "parent pid: " << getpid() << endl;

    global_start = std::chrono::high_resolution_clock::now();

    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
        cout << "set affinity had an error" << endl;
    }

    int rt_high_prio = 10;
    int rt_mid_prio = 80;
    int rt_low_prio = 75;

    int c_pid;
    vector<int> pids;

    for(int i=0; i<NUM_PROCS_TO_MAKE; i++) {
        int rt_to_use;
        string prio;
        if (i < NUM_LOW_PRIO) {
            rt_to_use = rt_low_prio;
            prio = "low";
        } else if (i < NUM_LOW_PRIO + NUM_MIDDLE_PRIO) {
            rt_to_use = rt_mid_prio;
            prio = "middle";
        } else {
            rt_to_use = rt_high_prio;
            prio = "high";
        }

        c_pid = fork();

        if (c_pid == -1) { 
            cout << "fork failed: " << strerror(errno) << endl;
            perror("fork"); 
            exit(EXIT_FAILURE); 
        } else if (c_pid > 0) {
            pids.push_back(c_pid);
            continue; 
        } else {

            cout << "proc w/ prio " << prio << " starting after " << time_since_start(global_start) << endl;

            struct sched_attr attr;
            int ret = syscall(SYS_sched_getattr, getpid(), &attr, sizeof(attr), 0);;
            if (ret < 0) {
                perror("ERROR: sched_getattr");
            }
            attr.sched_runtime = rt_to_use * NSEC_PER_MSEC;
            ret = syscall(SYS_sched_setattr, getpid(), &attr, 0);
            if (ret < 0) {
                perror("ERROR: sched_setattr");
            }

            // cout << "set sched_runtime to be " << rt_to_use << endl;
            // set child affinity - they will both run on cpu 0
            cpu_set_t  mask;
            CPU_ZERO(&mask);
            CPU_SET(1, &mask);
            if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
                cout << "set affinity had an error" << endl;
            }

            cout << "child w/ prio " << prio << " and pid " << getpid() << endl;

            if (prio == "low") {
                long_fac();
            } else if (prio == "middle") {
                mid_fac();
            } else {
                short_fac();
            }

            cout << "proc w/ prio " << prio << " ending after " << time_since_start(global_start) << endl;

            return 0;
            } 
    }

    for (int pid : pids) {
        struct rusage usage_stats;
        int ret = wait4(pid, NULL, 0, &usage_stats);
        if (ret < 0) {
            perror("wait4 did bad");
        }
        float runtime = (usage_stats.ru_utime.tv_sec * 1000.0 + (usage_stats.ru_utime.tv_usec/1000.0))
                                + (usage_stats.ru_stime.tv_sec * 1000.0 + (usage_stats.ru_stime.tv_usec/1000.0));
        cout << "rusage runtime for pid " << pid << " was " << runtime << endl;
    }

}

