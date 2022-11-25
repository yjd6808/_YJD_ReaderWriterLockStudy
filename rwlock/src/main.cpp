/*
 * 작성자: 윤정도
 * =====================
 *
 */

#pragma warning(push)
  #pragma warning(disable: 26495) // variable is uninitialized. Always initialize a member variable
  #include <benchmark/benchmark.h>
#pragma warning(pop)

#include <functional>
#include <iostream>
#include <vector>
#include <shared_mutex>
#include <mutex>
#include <chrono>

#include "my_atomic_rwlock.h"
#include "my_cond_rwlock.h"

using namespace std::chrono_literals;

#define MY_RWTEST     1     // 내가 만든 rw락으로 테스트
#define STD_RWTEST    2     // 표준 rw락으로 테스트
#define STD_MTXTEST   3     // 표준 뮤텍스로 테스트

#define TESTCASE      STD_RWTEST

// 1또는 0으로 바꿔서 테스트하면 됨
#if 1
  my_cond_rwlock my_rwlock;
#else
  my_atomic_rwlock my_rwlock;
#endif

std::shared_mutex std_rwlock;
std::mutex std_lock;

volatile int write_value = 0;
std::vector<int> possible_observable_values;
int read_values[8];

static void my_writer(int64_t size, int /* thread_index */, int /* sleepms */)
{
  my_rwlock.write_acquire();
  for (int64_t i = 0; i < size; i++)
    write_value += 1;
  my_rwlock.write_release();
}

static void my_reader(int64_t /*size */, int thread_index, int sleepms)
{
  my_rwlock.read_acquire();
  read_values[thread_index] = write_value;
  std::this_thread::sleep_for(std::chrono::milliseconds(sleepms));
  my_rwlock.read_release();
}

static void std_writer(int64_t size, int /* thread_index */, int /* sleepms */ )
{
  std::unique_lock lk(std_rwlock);
  for (int64_t i = 0; i < size; i++)
    write_value += 1;
}

static void std_reader(int64_t /*size */, int thread_index, int sleepms)
{
  std::shared_lock lk(std_rwlock);
  read_values[thread_index] = write_value;
  std::this_thread::sleep_for(std::chrono::milliseconds(sleepms));
}

static void std_mtx_writer(int64_t size, int /* thread_index */, int /* sleepms */ )
{
  std::unique_lock lk(std_lock);
  for (int64_t i = 0; i < size; i++)
    write_value += 1;
}

static void std_mtx_reader(int64_t /*size */, int thread_index, int sleepms)
{
  std::unique_lock lk(std_lock);
  read_values[thread_index] = write_value;
  std::this_thread::sleep_for(std::chrono::milliseconds(sleepms));
}


static void rwlock_test_setup(const benchmark::State& s)
{
  const int write_count = static_cast<int>(s.range(0));
  const int writer_thread_count = static_cast<int>(s.range(1));

  // 먼저 관측가능한 값들을 세팅해준다.
  for (int i = 0; i <= writer_thread_count; ++i)
    possible_observable_values.push_back(i * write_count);
}

static void rwlock_test_teardown(const benchmark::State& s)
{
  // 읽기 쓰레드에서 관측한 값이 관측가능한 값 목록에 없는 값인 경우 테스트 실패 (내가 만든 rw락을 테스트하기 위함)
  for (int r : read_values)
    if (std::ranges::find(possible_observable_values, r) == possible_observable_values.end())
      assert(false);

  /*
  중간에 읽기가 어떻게 수행되는지 확인용 (관측가능한 값들 중 하나)
  const int writer_thread_count = static_cast<int>(s.range(1));
  for (int i = writer_thread_count; i < 8; i++)
    std::cout << read_values[i] << "\n";
  std::cout << "===================\n";
  */

  // 초기화
  for (int i = 0; i < 8; i++)
    read_values[i] = 0;
  write_value = 0;
  possible_observable_values.clear();
}

static void my_rwlock_test(benchmark::State& state)
{
  const int64_t write_count = state.range(0);
  const int64_t writer_thread_count = state.range(1);
  const int64_t sleepms = state.range(2);
  const bool is_writer = state.thread_index() < writer_thread_count;
  const std::function method = is_writer ? my_writer : my_reader;
  const int thread_index = state.thread_index();

  for (auto _ : state)
    method(write_count, thread_index, static_cast<int>(sleepms));
}

static void std_rwlock_test(benchmark::State& state)
{
  const int64_t write_count = state.range(0);
  const int64_t writer_thread_count = state.range(1);
  const int64_t sleepms = state.range(2);
  const bool is_writer = state.thread_index() < writer_thread_count;
  const std::function method = is_writer ? std_writer : std_reader;
  const int thread_index = state.thread_index();

  for (auto _ : state)
    method(write_count, thread_index, static_cast<int>(sleepms));
}

static void std_mutex_test(benchmark::State& state)
{
  const int64_t write_count = state.range(0);
  const int64_t writer_thread_count = state.range(1);
  const int64_t sleepms = state.range(2);
  const bool is_writer = state.thread_index() < writer_thread_count;
  const std::function method = is_writer ? std_mtx_writer : std_mtx_reader;
  const int thread_index = state.thread_index();

  for (auto _ : state)
    method(write_count, thread_index, static_cast<int>(sleepms));
}


#if TESTCASE == MY_RWTEST
  BENCHMARK(my_rwlock_test)
#elif TESTCASE == STD_RWTEST
  BENCHMARK(std_rwlock_test)
#else
  BENCHMARK(std_mutex_test)
#endif
->Setup(rwlock_test_setup)
->Teardown(rwlock_test_teardown)
->Iterations(1)
->Repetitions(100)
->DisplayAggregatesOnly()
->Unit(benchmark::kMicrosecond)
->ThreadRange(8, 8)  // 쓰레드수는 8개
->ArgsProduct(
{
  {   10'000'000},                                                   
  {            1,  2, 4, 6},     
  {           10}                                                    
});


  // 첫번째 인자: write_value에 몇번 1을 더할지 (내가 구현한 rwlock이 제대로 동작하는지 테스트하기 위함)
  // 두번째 인자: 8개의 쓰레드 중 writer 쓰레드 수(나머지는 읽기쓰레드)
  // 세번째 인자: 읽기 작업시간 (잠금획득 해당 ms시간만큼 sleep)