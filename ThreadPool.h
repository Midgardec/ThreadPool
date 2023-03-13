#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <set>
#include <map>
#include <stdexcept> // для std::runtime_error

class ThreadPool {
public:
    // Конструктор принимает число потоков, которые будут созданы в пуле
    explicit ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            // Создаем новый поток, который будет выполнять задачи из очереди
            threads_.emplace_back([this] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        // Ожидаем, пока в очереди появится новая задача или не придет сигнал остановки
                        condition_.wait(lock, [this] { return !tasks_.empty() || stop_; });
                        // Если пришел сигнал остановки и в очереди больше нет задач, завершаем работу потока
                        if (stop_ && tasks_.empty()) {
                            return;
                        }
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    // Выполняем задачу
                    try {
                        task();
                    } catch (const std::exception& e) {
                        std::cerr << "Exception in ThreadPool task: " << e.what() << std::endl;
                    }
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        // Добавляем id выполненной задачи в множество завершенных задач и оповещаем ожидающие потоки
                        completed_tasks_.insert(task.id);
                        condition_.notify_all();
                    }
                }
            });
        }
    }

    // Деструктор останавливает все потоки и дожидается их завершения
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (auto& thread : threads_) {
            thread.join();
        }
    }

    // Добавляет новую задачу в очередь и возвращает ее id
    uint64_t addTask(std::function<void()> func) {
        Task task(std::move(func), next_task_id_++);
        {
            std::unique_lock<std::mutex> lock(mutex_);
            tasks_.push(std::move(task));
            return task.id;
        }
    }

    // Ожидает завершения задачи с заданным id
    void wait(uint64_t task_id) {
        std::unique_lock<std::mutex> lock(mutex_);
        // Ожидаем, пока задача не будет выполнена
        condition_.wait(lock, [this, task_id] {
            return completed_tasks_.find(task_id) != completed_tasks_.end();
        });
        // Удаляем id завершенной задачи из множества завершенных задач
        completed_tasks_.erase(task_id);
    }


    void wait_all() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] {
            return tasks_.empty() && completed_tasks_.size() == next_task_id_ - 1;
        });
        //std::cout << "All tasks completed\n";
    }

private:
    class Task {
    public:
        Task() = default;

        Task(std::function<void()> f, uint64_t i) : func_(std::move(f)), id(i) {}

        void operator()() {
            func_();
        }

        uint64_t id;
    private:
        std::function<void()> func_;
    };

    std::queue<Task> tasks_;
    std::set<uint64_t> completed_tasks_;
    std::vector<std::thread> threads_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool stop_ = false;
    uint64_t next_task_id_ = 0;
};