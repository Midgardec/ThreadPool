#include <iostream>
#include <vector>
#include <chrono>
#include "ThreadPool.h"

void task1() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Task 1 executed\n";
}

void task2(int x) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Task 2 executed with argument " << x << "\n";
}

int main() {
    ThreadPool pool(4); // создаем пул из 4 потоков

    std::vector<uint64_t> task_ids;

    // добавляем задачи в пул и сохраняем их идентификаторы
    task_ids.push_back(pool.addTask(task1));
    task_ids.push_back(pool.addTask([&] { task2(42); }));
    task_ids.push_back(pool.addTask([] {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "Task 3 executed\n";
    }));

    // ждем выполнения задачи с id = task_ids[1]

    pool.wait(task_ids[3]);
    /*pool.wait(task_ids[2]);*/
    // ждем выполнения всех задач в пуле
    pool.wait_all();

    std::cout << "All Tasks completed" << std::endl;
    return 0;
}