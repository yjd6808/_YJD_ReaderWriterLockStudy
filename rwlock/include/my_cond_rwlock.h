/*
 * 작성자: 윤정도
 * 생성일: 11/25/2022 11:32:24 PM
 * =====================
 *
 */

#pragma once

#include <condition_variable>
#include <mutex>

class my_cond_rwlock
{
public:
  void write_acquire()
  {
    std::unique_lock lg(mtx);
    while (write_flag || read_count)
      condvar.wait(lg);
    write_flag = true;
  }

  void write_release()
  {
    std::unique_lock lg(mtx);
    write_flag = false;
    condvar.notify_all();
  }

  void read_acquire()
  {
    std::unique_lock lg(mtx);
    while (write_flag)
      condvar.wait(lg);
    ++read_count;
  }

  void read_release()
  {
    std::unique_lock lg(mtx);
    if (read_count > 0)
      --read_count;
    condvar.notify_all();
  }

private:
  std::condition_variable condvar;
  std::mutex mtx;

  int read_count;
  bool write_flag;
};

