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

// OG vals: 
// echo 3000000 | sudo tee /sys/kernel/debug/sched/min_granularity_ns
// echo 4000000 | sudo tee /sys/kernel/debug/sched/wakeup_granularity_ns

// curr vals
// echo 100000 | sudo tee /sys/kernel/debug/sched/min_granularity_ns
// echo 100000 | sudo tee /sys/kernel/debug/sched/wakeup_granularity_ns

high_resolution_clock::time_point global_start;

#define NUM_5_MS_SLA 1
#define NUM_50_MS_SLA 1
#define NUM_500_MS_SLA 1
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

void printTimeTightLoop() {
    ofstream times_file;
    times_file.open("../times.txt");

    double last_time = timeDiff();
    
    while(true) {
        double curr_time = timeDiff();
        times_file  << timeDiff() << ", " << curr_time - last_time << endl;
        last_time = curr_time;
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}


int main() {

    emptyFiles();

    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(1, &mask);
    if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
        cout << "set affinity had an error" << endl;
    }

    int start_val = 15000000; // 15 ms
    int end_val = 500000; // 0.5 ms
    int decr_by = 500000; // 0.5 ms

    global_start = high_resolution_clock::now();

    thread t(printTimeTightLoop);

    for (int i = start_val; i >= end_val; i -= decr_by) {
        writeValToLatencyFile(i);
        cout << "doing val " << i << ", validated by reading: ";
        readValFromLatencyFile();
        comp_intense(i);
    }

    writeValToLatencyFile(start_val);

    terminate();
}

