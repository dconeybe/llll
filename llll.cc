#include <atomic>
#include <iostream>
#include <latch>
#include <thread>
#include <vector>

struct node {
  int value;
  std::atomic<node*> next;
};

std::ptrdiff_t num_threads = std::thread::hardware_concurrency();
std::latch latch(num_threads);
constinit node head = { .next = nullptr };

void list_populate(int start);
void list_print();

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  std::vector<std::thread> threads;
  int next_start = 100;
  for (auto i = num_threads; i > 0; --i) {
    threads.emplace_back(list_populate, next_start);
    next_start += 100;
  }

  for (auto& thread : threads) {
    thread.join();
  }

  list_print();

  return 0;
}

void list_append(int value) {
  node* const new_tail = new node{ .value = value, .next = nullptr };
  node* cur = &head;
  node* next = cur->next.load(std::memory_order_acquire);

  while (true) {
    if (next) {
      cur = next;
      next = cur->next.load(std::memory_order_acquire);
    } else if (cur->next.compare_exchange_weak(next, new_tail, std::memory_order_release, std::memory_order_acquire)) {
      break;
    }
  }
}

void list_populate(int start) {
  latch.arrive_and_wait();
  for (int i=0; i<50000; ++i) {
    list_append(start + i);
  }
}

void list_print() {
  unsigned count = 0;
  const node* cur = &head;
  while (cur->next) {
    cur = cur->next;
    if (count != 0) {
      //std::cout << ", ";
    }
    ++count;
    //std::cout << cur->value;
  }
  std::cout << std::endl << "size=" << count << std::endl;
}
