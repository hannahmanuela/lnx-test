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

high_resolution_clock::time_point global_start;

#define NUM_PROCS_TO_MAKE 2
#define NUM_LOW_PRIO 1
#define TO_FACTOR_BIIG 9369535334532
#define TO_FACTOR_500 93695353
#define TO_FACTOR_50  9768565
#define TO_FACTOR_5   964653

double timeDiff() {
    duration<double> time_span = duration_cast<duration<double>>(high_resolution_clock::now() - global_start);
    return 1000000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
}

void readValFromLatencyFile() {
    ifstream file("/sys/kernel/debug/sched/latency_ns");
    if (file.is_open())
        std::cout << file.rdbuf();
}

void writeValToLatencyFile(int val) {
    int fd = open("/sys/kernel/debug/sched/latency_ns", O_RDWR);
    if(fd == -1) {
        cout << "open failed: " << strerror(errno) << endl;
    }
    string to_write = to_string(val);
    size_t nb = write(fd, to_write.c_str(), to_write.size());
    if (nb == -1) {
        perror("Error writing");
    }
}

// The function we want to execute on each thread.
void comp_intense(int curr_latency) {

    ofstream file;
    file.open("../worker_out.txt", ios::app);
    // file << timeDiff() << ", " << curr_latency << ", 1" << endl;
    ofstream trash_file;
    trash_file.open("../trash.txt", ios::app);

    high_resolution_clock::time_point beg = high_resolution_clock::now();
 
    // ============================
    // do the thing
    // ============================
    for (int i = 1; i <= TO_FACTOR_5; i++) 
        if (TO_FACTOR_5 % i == 0) 
            trash_file << " " << i << endl; 

    high_resolution_clock::time_point end = high_resolution_clock::now();

    duration<double> time_span = duration_cast<duration<double>>(end - beg);

    file << timeDiff() << ", " <<  curr_latency  << ", " 
        << 1000 * time_span.count() << endl; // 1000 => shows millisec; 1000000 => shows microsec
    file.close();
}

void emptyFiles() {

    // empty files we write to
    ofstream polling_file;
    polling_file.open("../polling_info.txt");
    polling_file << "";
    polling_file.close();

    ofstream worker_file;
    worker_file.open("../worker_out.txt");
    worker_file << "";
    worker_file.close();

    ofstream proc_file;
    proc_file.open("../proc_files.txt");
    proc_file << "";
    proc_file.close();

    ofstream trash_file;
    trash_file.open("../trash.txt");
    trash_file << "";
    trash_file.close();

    ofstream times_file;
    times_file.open("../times.txt");
    times_file << "";
    times_file.close();

}

void tightLoop() {
    // vector<double> factors;
    
    // for (int i = 1; i <= TO_FACTOR_BIIG; i++) 
    //     if (TO_FACTOR_BIIG % i == 0) 
    //         factors.push_back(i);
    while(true) {}
}

void tightLoopYield() {
    // vector<double> factors;
    
    // for (int i = 1; i <= TO_FACTOR_BIIG; i++) 
    //     if (TO_FACTOR_BIIG % i == 0) 
    //         factors.push_back(i);
    while(true) {sched_yield();}
}


int main() {

    emptyFiles();

    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
        cout << "set affinity had an error" << endl;
    }

    int fd_high_prio = open("/sys/fs/cgroup/one-digit-ms", O_RDONLY);
    if(fd_high_prio == -1) {
        cout << "open failed: " << strerror(errno) << endl;
    }
    int fd_low_prio = open("/sys/fs/cgroup/three-digit-ms", O_RDONLY);
    if(fd_low_prio == -1) {
        cout << "open failed: " << strerror(errno) << endl;
    }

    int c_pid;

    for(int i=0; i<NUM_PROCS_TO_MAKE; i++) {
        int fd_to_use;
        int prio;
        if (i < NUM_LOW_PRIO) {
            fd_to_use = fd_low_prio;
            prio = 0;
        } else {
            fd_to_use = fd_high_prio;
            prio = 1;
        }

        struct clone_args args = {
            .flags = CLONE_INTO_CGROUP,
            .exit_signal = SIGCHLD,
            .cgroup = fd_to_use,
            };
        c_pid = syscall(SYS_clone3, &args, sizeof(struct clone_args));

        if (c_pid == -1) { 
            cout << "clone3 failed: " << strerror(errno) << endl;
            perror("clone3"); 
            exit(EXIT_FAILURE); 
        } else if (c_pid > 0) {
            continue; 
        } else {
            // set child affinity - they will both run on cpu 0
            cpu_set_t  mask;
            CPU_ZERO(&mask);
            CPU_SET(1, &mask);
            if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
                cout << "set affinity had an error" << endl;
            }
            cout << "child w/ prio " << prio << " and pid " << getpid() << " on cpu " << sched_getcpu() << endl;
            if (prio == 0) {
                // low priority
                tightLoop();
            } else {
                tightLoopYield();
            }
            
        } 
    }

    waitpid(c_pid, NULL, 0);
}

