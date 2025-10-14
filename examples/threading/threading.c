#include "threading.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    thread_data* data = (thread_data*)thread_param;

    int return_value = usleep(data->wait_to_obtain_us);
    if (return_value != 0)
    {
        ERROR_LOG("Sleep failed.");
        data->thread_complete_success = false;
        return thread_param;
    }

    return_value = pthread_mutex_lock(data->mutex);
    if (return_value != 0)
    {
        ERROR_LOG("Mutex lock failed.");
        data->thread_complete_success = false;
        return thread_param;
    }

    return_value = usleep(data->wait_to_release_us);
    if (return_value != 0)
    {
        ERROR_LOG("Sleep failed.");
        data->thread_complete_success = false;
        return thread_param;
    }

    return_value = pthread_mutex_unlock(data->mutex);
    if (return_value != 0)
    {
        ERROR_LOG("Mutex unlock failed.");
        data->thread_complete_success = false;
        return thread_param;
    }
    
    data->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    thread_data* data = (thread_data*)malloc(sizeof(thread_data));
    data->wait_to_obtain_us = wait_to_obtain_ms * 1000;
    data->wait_to_release_us = wait_to_release_ms * 1000;
    data->thread_complete_success = false;
    data->mutex = mutex;

    int return_value = pthread_create(thread, NULL, threadfunc, data);

    if (return_value != 0)
    {
        ERROR_LOG("Thread creation failed");
        return false;
    }

    return true;
}

