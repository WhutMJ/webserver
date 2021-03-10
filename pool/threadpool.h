/*
 * @Author        :mj
 * @Date          :2021-03-10
 * @copyright
 */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

class Threadpool{
  public:
    explicit Threadpool(size_t thread_num = 8) : pool_(std::make_shared<Taskpool>()) {
      assert(thread_num > 0);
      for (int i=0; i<thread_num; ++i) {
        std::thread([pool_ = pool]{
          std::unique_lock<std::mutex> locker(pool->task_mutex);
          while(true) {
            //handle the task
            if (!pool->tasks.empty()) {
              auto task = std::move(pool->tasks.front());
              pool->tasks.pop();
              locker.unlock();
              task();
              locker.lock();
            }
            else if (pool->isClosed) break;
            else pool->task_variable.wait(lock);
          }
        }).detach();
      }
    }
    Threadpool() = default;
    Threadpool(Threadpool&&) = default;

    ~Threadpool(){
      if(static_cast<bool>(pool_)){
        std::lock_guard<std::mutex> locker(pool_->task_mutex);
        pool_->isClosed = true;
      }
      pool_->condition_variable.notify_all();
    }

    template<class F>
    void addTask(F&& func){
      {
        std::lock_guard<std::mutex> locker(pool_->task_mutex);
        pool_->tasks.emplace(std::forward<F>(func));
      }
      pool->task_variable.notify_one();
    }
  private:
    typedef struct Taskpool{
      std::mutex task_mutex;
      std::condition_variable task_variable;
      bool isClosed = false;
      std::queue<std::functional<void()> > tasks;
    }Taskpool;
    std::shared_ptr<Taskpool> pool_;
};

#endif //THREADPOOL_H