#include <atomic>
#include <iostream>
#include <latch>
#include <mutex>
#include <thread>
#include <vector>

// A node in the linked list.
struct node {
  unsigned value;
  std::atomic<node*> next;
};

// The head of the linked list.
constinit node head = { .next = nullptr };

// Append the given value to the linked list.
void list_append(unsigned value) {
  node* const new_tail = new node{ .value = value, .next = nullptr };
  node* cur = &head;
  node* next = cur->next.load(std::memory_order_acquire);

  // This is a compare-and-swap (a.k.a. "CAS") loop, which is very common
  // in lock-free programming.
  while (true) {
    if (next) {
      cur = next;
      next = cur->next.load(std::memory_order_acquire);
    } else if (
        cur->next.compare_exchange_weak(
          next,
          new_tail,
          std::memory_order_release,
          std::memory_order_acquire
        )
    ) {
      break;
    }
  }
}

// Appends `count` values to the list, starting at `start`.
// Blocks on latch.arrive_and_wait().
void list_populate(
    std::latch& latch,
    std::atomic<unsigned>& live_thread_count,
    const unsigned start,
    const unsigned count
);

// Prints information about the list.
void list_print();

// Lock to serialize writes to stdout.
std::mutex cout_mutex;

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  const auto num_threads = std::thread::hardware_concurrency();
  std::cout << "Starting " << num_threads << " threads." << std::endl;
  std::atomic<unsigned> live_thread_count(num_threads);
  const unsigned count = 200000 / num_threads;
  std::latch latch(num_threads);
  std::vector<std::thread> threads;
  unsigned next_start = 0;
  for (auto i = num_threads; i > 0; --i) {
    threads.emplace_back([&latch, &live_thread_count, next_start, count] {
        list_populate(latch, live_thread_count, next_start, count);
    });
    next_start += count;
  }

  for (auto& thread : threads) {
    thread.join();
  }
  std::cout << "All " << num_threads << " threads have completed." << std::endl;

  list_print();

  return 0;
}

void list_populate(
    std::latch& latch,
    std::atomic<unsigned>& live_thread_count,
    const unsigned start,
    const unsigned count
) {
  const auto thread_id = std::this_thread::get_id();
  {
    std::lock_guard lock(cout_mutex);
    std::cout << "Thread " << thread_id
      << " started: start=" << start
      << " count=" << count << std::endl;
  }

  latch.arrive_and_wait();
  for (unsigned i=0; i<count; ++i) {
    list_append(start + i);
  }

  {
    std::lock_guard lock(cout_mutex);
    const auto num_threads_remaining =
      live_thread_count.fetch_sub(1, std::memory_order_relaxed) - 1;
    std::cout << "Thread " << thread_id
      << " done (" << num_threads_remaining
      << " threads remaining): start=" << start
      << " count=" << count << std::endl;
  }
}

void list_print() {
  unsigned count = 0;
  const node* cur = &head;
  while (cur->next) {
    cur = cur->next;
    ++count;
  }

  std::cout << std::endl << "DONE: list size: " << count << std::endl;
}
