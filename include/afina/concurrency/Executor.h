#ifndef AFINA_CONCURRENCY_EXECUTOR_H
#define AFINA_CONCURRENCY_EXECUTOR_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <algorithm>

namespace Afina {
namespace Concurrency {

/**
 * # Thread pool
 */
class Executor {
    enum class State {
        // Threadpool is fully operational, tasks could be added and get executed
        kRun,

        // Threadpool is on the way to be shutdown, no ned task could be added, but existing will be
        // completed as requested
        kStopping,

        // Threadppol is stopped
        kStopped
    };

    Executor(std::string name, size_t low_watermark,
            size_t hight_watermark, size_t max_queue_size, size_t idle_time) {
        this->low_watermark = low_watermark;
        this->hight_watermark = hight_watermark;
        this->max_queue_size = max_queue_size;
        this->idle_time = idle_time;
    }

    ~Executor();

    //When you run Executor create threads
    void Run() {
        std::unique_lock<std::mutex> _lock(mutex);
        if(state == State::kRun) {
            throw std::runtime_error("Executor already run!\n");
        }
        for (size_t i = 0; i < low_watermark; ++i){
            threads.emplace_back(std::thread([this] {
                return perform(this);
            }));
        }
        cur_thread_count = low_watermark;
        state = State::kRun;
    }

    /**
     * Signal thread pool to stop, it will stop accepting new jobs and close threads just after each become
     * free. All enqueued jobs will be complete.
     *
     * In case if await flag is true, call won't return until all background jobs are done and all threads are stopped
     */
    void Stop(bool await = false) {
        std::unique_lock<std::mutex> _lock(mutex);
        if (state == State::kStopped) { return; }
        state = State::kStopping;
        if (await) {
            while (!threads.empty()){
                stop_condition.wait(_lock);
            }
        }

        if (!cur_thread_count && tasks.empty()) {
            state = State::kStopped;
        }
    }

    /**
     * Add function to be executed on the threadpool. Method returns true in case if task has been placed
     * onto execution queue, i.e scheduled for execution and false otherwise.
     *
     * That function doesn't wait for function result. Function could always be written in a way to notify caller about
     * execution finished by itself
     */
    template <typename F, typename... Types> bool Execute(F &&func, Types... args) {
        // Prepare "task"
        auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);

        std::unique_lock<std::mutex> lock(this->mutex);
        if (state != State::kRun || tasks.size() == max_queue_size) {
            return false;
        }
        
        if (tasks.size() > threads.size() && cur_thread_count <= hight_watermark) {
            tasks.push_back(exec);
            
            threads.emplace_back(std::thread([this] {
                return perform(this);
            }));
            ++cur_thread_count;
        } 
        tasks.push_back(exec);
        empty_condition.notify_one();
        return true;
    }

private:
    // No copy/move/assign allowed
    Executor(const Executor &);            // = delete;
    Executor(Executor &&);                 // = delete;
    Executor &operator=(const Executor &); // = delete;
    Executor &operator=(Executor &&);      // = delete;

    /**
     * Main function that all pool threads are running. It polls internal task queue and execute tasks
     */
    friend void perform(Executor *executor) {
        while(true){
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(executor->mutex);
                

                executor->empty_condition.wait_for(lock, std::chrono::milliseconds(executor->idle_time),
                        [executor]{ return executor->tasks.empty(); });

                if (executor->tasks.empty() && executor->cur_thread_count > executor->low_watermark){    
                    auto t_id = std::this_thread::get_id();
                    auto it = std::find(executor->threads.begin(), executor->threads.end(), [t_id](std::thread &t){
                            return t.get_id() == t_id;
                            });
                    (*it).detach();

                    --executor->cur_thread_count;
                    if(executor->cur_thread_count == 0 && executor->state == Executor::State::kStopping){
                        executor->stop_condition.notify_all();
                        executor->state = State::kStopped;
                    }
                    break;
                }

                task = executor->tasks.front();
                executor->tasks.pop_front();
            }
            try {
            	task();
            } catch(...) {
                fprintf(stderr, "Error in task\n");
            }
        }
    }
   

    /**
     * Mutex to protect state below from concurrent modification
     */
    std::mutex mutex;

    /**
     * Conditional variable to await new data in case of empty queue
     */
    std::condition_variable empty_condition;

    /**
     * Vector of actual threads that perorm execution
     */
    std::vector<std::thread> threads;

    /**
     * Task queue
     */
    std::deque<std::function<void()>> tasks;

    /**
     * Flag to stop bg threads
     */
    State state;

    //Home work
    size_t low_watermark;

    size_t hight_watermark;

    size_t max_queue_size;

    size_t idle_time;

    size_t cur_thread_count = 0;

    std::condition_variable stop_condition;

    size_t active_threads = 0;
};

} // namespace Concurrency
} // namespace Afina

#endif // AFINA_CONCURRENCY_EXECUTOR_H
