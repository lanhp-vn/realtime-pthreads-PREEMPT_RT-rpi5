#include <pthread.h>
#include <sys/mman.h>  // necessary for mlockall
#include <sys/time.h>
#include <cstring>
#include <stdexcept>
#include <string>
#include "p3_util.h"

#define SET_CPU false

void LockMemory() {
    int ret = mlockall(MCL_CURRENT | MCL_FUTURE);
    if (ret) {
        throw std::runtime_error{std::string("mlockall failed: ") + std::strerror(errno)};
    }
}

void setCPU(int cpu_id = 1) {
    // Set CPU affinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

class ThreadRT {
    int priority_;
    int policy_;
    pthread_t thread_;
    struct timeval start_time;

    static void* RunThreadRT(void* data) {
        #if SET_CPU
        setCPU(1);  // Ensure thread is bound to CPU 1
    #endif
    
        // Print which CPU the RT thread is running on
        printf("[RT thread #%lu] running on CPU #%d\n", pthread_self(), sched_getcpu());
    
        // Get the thread's scheduling parameters (policy and priority)
        sched_param param;
        int policy;
        int ret = pthread_getschedparam(pthread_self(), &policy, &param);
        if (ret == 0) {
            printf("[RT thread #%lu] Scheduling policy: ");
            if (policy == SCHED_FIFO) {
                printf("SCHED_FIFO ");
            } else if (policy == SCHED_RR) {
                printf("SCHED_RR ");
            } else {
                printf("Other policy ");
            }
            printf("with priority %d\n", param.sched_priority);
        } else {
            printf("[RT thread #%lu] Failed to get scheduling parameters\n", pthread_self());
        }
    
        // Run the thread's workload
        ThreadRT* thread = static_cast<ThreadRT*>(data);
        thread->Run();
        return NULL;
    }

public:
    int app_id_;

    ThreadRT(int app_id, int priority, int policy) : app_id_(app_id), priority_(priority), policy_(policy) {}

    void Start() {
        // Initialize pthread attributes
        pthread_attr_t thread_attr;
        pthread_attr_init(&thread_attr);

        // Set the scheduling policy (e.g., SCHED_FIFO or SCHED_RR)
        pthread_attr_setschedpolicy(&thread_attr, policy_);
        
        // Set the thread's priority
        sched_param param;
        param.sched_priority = priority_;
        pthread_attr_setschedparam(&thread_attr, &param);

        // Set the thread's stack size (optional, but often necessary for RT threads)
        pthread_attr_setstacksize(&thread_attr, 1024 * 1024);  // 1MB stack size

        // Ensure the thread does not inherit attributes from the parent thread
        pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);

        // Start the timer
        gettimeofday(&start_time, NULL);

        // Create the RT thread
        int ret = pthread_create(&thread_, &thread_attr, &ThreadRT::RunThreadRT, this);
        if (ret) {
            throw std::runtime_error{std::string("pthread_create failed: ") + std::strerror(ret)};
        }

        // Destroy the thread attribute object after the thread has been created
        pthread_attr_destroy(&thread_attr);
    }

    void Join() {
        // Wait for the thread to finish
        int ret = pthread_join(thread_, NULL);
        if (ret) {
            throw std::runtime_error{std::string("pthread_join failed: ") + std::strerror(ret)};
        }

        // End the timer and calculate elapsed time
        struct timeval end_time;
        gettimeofday(&end_time, NULL);

        long seconds = end_time.tv_sec - start_time.tv_sec;
        long micros = (end_time.tv_usec - start_time.tv_usec);
        double elapsed_time = seconds + micros * 1e-6;
        printf("App #%d runtime: %f seconds\n", app_id_, elapsed_time);

        printf("[RT thread #%lu] App #%d Ends\n", thread_, app_id_);
    }

    virtual void Run() = 0;
};

class ThreadNRT {
    pthread_t thread_;
    struct timeval start_time;

    static void* RunThreadNRT(void* data) {
#if SET_CPU
        setCPU(1);
#endif
        printf("[NRT thread #%lu] running on CPU #%d\n", pthread_self(), sched_getcpu());
        ThreadNRT* thread = static_cast<ThreadNRT*>(data);
        thread->Run();
        return NULL;
    }

public:
    int app_id_;

    ThreadNRT(int app_id) : app_id_(app_id) {}

    void Start() {
        // Start the timer
        gettimeofday(&start_time, NULL);

        // Create the pthread
        int ret = pthread_create(&thread_, NULL, &ThreadNRT::RunThreadNRT, this);
        if (ret) {
            throw std::runtime_error{std::string("pthread_create failed: ") + std::strerror(ret)};
        }
    }

    void Join() {
        // Wait for the thread to finish
        pthread_join(thread_, NULL);

        // End the timer and calculate elapsed time
        struct timeval end_time;
        gettimeofday(&end_time, NULL);

        long seconds = end_time.tv_sec - start_time.tv_sec;
        long micros = (end_time.tv_usec - start_time.tv_usec);
        double elapsed_time = seconds + micros * 1e-6;
        printf("App #%d runtime: %f seconds\n", app_id_, elapsed_time);

        printf("[NRT thread #%lu] App #%d Ends\n", thread_, app_id_);
    }

    virtual void Run() = 0;
};

class AppTypeX : public ThreadRT {
public:
    AppTypeX(int app_id, int priority, int policy) : ThreadRT(app_id, priority, policy) {}

    void Run() {
        printf("Running App #%d...\n", app_id_);
        // Simulate compute-intensive task
        BusyCal();
    }
};

class AppTypeY : public ThreadNRT {
public:
    AppTypeY(int app_id) : ThreadNRT(app_id) {}

    void Run() {
        printf("Running App #%d...\n", app_id_);
        // Simulate compute-intensive task
        BusyCal();
    }
};

int main(int argc, char** argv) {
    int exp_id = 4;
    if (argc < 2) {
        fprintf(stderr, "WARNING: default exp_id=0\n");
    } else {
        exp_id = atoi(argv[1]);
    }

    LockMemory();

    if (exp_id == 0) {
        printf("Experiment 1: One CannyP3 APP (RT) and Two any-type APPs (NRT), All running on CPU=1\n");
        // One CannyP3 APP (RT) and Two any-type APPs (NRT), All running on CPU=1
        AppTypeX app1(1, 80, SCHED_FIFO);
        AppTypeY app2(2);
        AppTypeY app3(3);

        app1.Start();
        app2.Start();
        app3.Start();

        app1.Join();
        app2.Join();
        app3.Join();
    }
    else if (exp_id == 1) {
        printf("Experiment 2: Same workload as 1, but freely run on available CPUs\n");
        // Same workload as 1, but freely run on available CPUs
        AppTypeX app1(1, 80, SCHED_FIFO);
        AppTypeY app2(2);
        AppTypeY app3(3);

        app1.Start();
        app2.Start();
        app3.Start();

        app1.Join();
        app2.Join();
        app3.Join();
    }
    else if (exp_id == 2) {
        printf("Experiment 3: Two any-type APPs (same priority, SCHED_FIFO) in RT and One any-type APP (NRT), All running on CPU=1\n");
        // Two any-type APPs (same priority, SCHED_FIFO) in RT and One any-type APP (NRT), All running on CPU=1
        AppTypeX app1(1, 80, SCHED_FIFO);
        AppTypeX app2(2, 80, SCHED_FIFO);
        AppTypeY app3(3);

        app1.Start();
        app2.Start();
        app3.Start();

        app1.Join();
        app2.Join();
        app3.Join();
    }
    else if (exp_id == 3) {
        printf("Experiment 4: Two any-type APPs (same priority, SCHED_RR) in RT and One any-type APP (NRT), All running on CPU=1\n");
        // Two any-type APPs (same priority, SCHED_RR) in RT and One any-type APP (NRT), All running on CPU=1
        AppTypeX app1(1, 80, SCHED_RR);
        AppTypeX app2(2, 80, SCHED_RR);
        AppTypeY app3(3);

        app1.Start();
        app2.Start();
        app3.Start();

        app1.Join();
        app2.Join();
        app3.Join();
    }
    else if (exp_id == 4) {
        printf("Experiment 5: Same workload as 3, but freely run on available CPUs\n");
        // Same workload as 3, but freely run on available CPUs
        AppTypeX app1(1, 80, SCHED_FIFO);
        AppTypeX app2(2, 80, SCHED_FIFO);
        AppTypeY app3(3);

        app1.Start();
        app2.Start();
        app3.Start();

        app1.Join();
        app2.Join();
        app3.Join();
    }
    else {
        printf("ERROR: exp_id NOT FOUND\n");
    }

    return 0;
}
