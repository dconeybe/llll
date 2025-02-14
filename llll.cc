#include <atomic>
#include <thread>

struct node {
  int value;
  node* next;
};

std::atomic<node> head;

int main(int argc, char** argv) {
  return 0;
}
