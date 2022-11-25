/*
 * 작성자: 윤정도
 * 생성일: 11/24/2022 5:39:23 PM
 * =====================
 *
 */

#pragma once

#include <atomic>

 // [원자성 깨졌는지 테스트 용]
#define BROKEN_ATOMICITY_TEST 0

// 1. 읽기락 획득시 쓰기락이 되어있는지
inline std::atomic<int> g_broken_read_atomicity;

// 2. 쓰기락 획득시 읽기락이 되어있는지
inline std::atomic<int> g_broken_write_atomicity;

class my_rwlock
{
public:
  void read_acquire()
  {
  READ_ACQUIRE:
    // 쓰기를 수행중인 쓰레드가 없어질때까지 기다린다.
    while (write_flag.load()) {}

    while (true)
    {
      ++read_counter;

      if (write_flag)
      {
        --read_counter;

        // 디버깅용 변수
        // 여기 들어오는 경우가 있을 수 있는지 체크용
#if BROKEN_ATOMICITY_TEST
        ++g_broken_read_atomicity;
#endif
        goto READ_ACQUIRE;
      }

      break;
    }
  }

  void read_release()
  {
    if (read_counter > 0)
      --read_counter;
  }

  void write_acquire()
  {

  WRITE_ACQUIRE:

    // 먼저 읽기를 수행중인 쓰레드가 없어야한다.
    // 따라서 읽기 카운터가 0이 될때까지 스핀한다.
    while (read_counter.load() > 0) {}

    while (true)
    {
      bool write_expected = false;

      // compare_exchange_strong()
      // expected: 기대값
      // desired: 대입값
      // return: 성공적으로 대입연산이 수행되면 true를 반환,
      // 만약 false를 반환할경우 실제 저장된 값을 expected에 대입해서 반환해준다.

      // write_flag값이 false일 경우 true로 설정해줘서 쓰기 잠금을 획득해준다.
      if (write_flag.compare_exchange_strong(write_expected, true))
      {

        // 만약 성공적으로 교체된 후 read_counter가 0이 아닌 경우에는
        // 다시 read_counter가 0이 될때까지 기다린다.
        if (read_counter != 0)
        {
          write_flag = false;
#if BROKEN_ATOMICITY_TEST
          ++g_broken_write_atomicity; // 디버깅용 변수
#endif
          goto WRITE_ACQUIRE;
        }

        break;
      }
    }
  }

  void write_release()
  {
    write_flag = false;
  }

  int read_count() const
  {
    return read_counter;
  }

  // 읽기락이 되어있을 때는 무조건 쓰기락 상태가 아니어야한다.
  bool is_read_locked() const
  {
    return read_counter > 0 && write_flag == false;
  }

  // 쓰기락이 되어있을 때는 무조건 읽기락 상태가 아니어야한다.
  bool is_write_locked() const
  {
    return write_flag && read_counter == 0;
  }
private:
  std::atomic<bool> write_flag = false;
  std::atomic<int> read_counter = 0;
};




