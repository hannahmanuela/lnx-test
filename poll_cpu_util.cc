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

#define NUM_PROCS_TO_MAKE 3
#define NUM_LOW_PRIO 1
#define NUM_MIDDLE_PRIO 1

int long_fac() {
    
    int to_factor = 43695353;
    std::vector<int> factors;

    for (int i = 1; i <= to_factor; i++) {
        if (to_factor % i == 0) {
            factors.push_back(i);
        }
    }

    return 0;
}

int mid_fac() {
    int to_factor = 7264565;
    std::vector<int> factors;

    for (int i = 1; i <= to_factor; i++) {
        if (to_factor % i == 0) {
            factors.push_back(i);
        }
    }

    return 0;
}

int short_fac() {
    int to_factor = 968253;
    std::vector<int> factors;

    for (int i = 1; i <= to_factor; i++) {
        if (to_factor % i == 0) {
            factors.push_back(i);
        }
    }

    return 0;
}


int main() {

    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
        cout << "set affinity had an error" << endl;
    }

    int rt_high_prio = 3;
    int rt_mid_prio = 30;
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

        struct sched_attr attr;
        syscall(SYS_sched_getattr, getpid(), &attr, 0);
        attr.sched_runtime = rt_to_use * NSEC_PER_MSEC;
        int ret = syscall(SYS_sched_setattr, getpid(), &attr, 0);
        if (ret < 0) {
            perror("sched_setattr");
        }
        cout << "set sched_runtime to be " << rt_to_use << endl;
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
            return 0;
        } 
    }

    for (int pid : pids) {
        waitpid(pid, NULL, 0);
    }

}

