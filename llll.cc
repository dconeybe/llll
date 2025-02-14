#include <atomic>
#include <cinttypes>
#include <thread>

struct node {
  int value;
  node* next;
};

std::atomic<node> head;

void ll_append(int value) {
  const node head_copy = head.load();
  const node * cur = &head_copy;
  while (cur->next) {
    cur = cur->next;
  }
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  head.store({ .value = 0, .next = nullptr });

  return 0;
}
