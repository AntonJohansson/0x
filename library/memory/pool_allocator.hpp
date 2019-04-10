#pragma once

#include <other/number_types.hpp>
//#include <utility>
#include <queue>
#include <vector>
#include <cstdlib>

#include <new>

enum Mode{
  RESIZE,
  NO_RESIZE,
};

class Allocator{};

template<typename T>
class PoolAllocator : public Allocator{
public:
  explicit PoolAllocator(u64 block_size = 16, Mode m = RESIZE)
    : _size(0),
      _block_size(block_size),
      _mode(m){
    allocate_block();
  }

  ~PoolAllocator(){
    for(u8* block : _memory_blocks){
      delete[] block;
    }
  }

  template<typename... Args>
  T* alloc(Args&&... args){
    T* ptr = nullptr;

    // Check out of memory
    if(_free_list.size() == 0){
      if(_mode == RESIZE)allocate_block();
      else throw std::bad_alloc();
    }

    // Get ptr from free list
    ptr = _free_list.front();
    _free_list.pop();

    // Placement new
    ptr = new (ptr) T(std::forward<Args>(args)...);

    _size++;

    return ptr;
  }

  void free(T* t){
    // Call destructor
    t->~T();

    // "free" memory
    _free_list.push(t);
    _size--;

    // @TODO should i dealloc a block if it becomes emtpy?
  }

  void reset(){
    _size = 0;

    // Free all pointers, but do not dealloc blocks
    for(u8* block : _memory_blocks){
      add_block_to_free_list(block);
    }
  }

  u64 size(){return _size;}
  u64 block_size(){return _block_size;}
  u64 blocks(){return _memory_blocks.size();}
  u64 capacity(){return _block_size*_memory_blocks.size();}
  u64 free(){return _free_list.size();}

  float load_factor(){return _size/capacity();}

private:
  void allocate_block(){
    u8* block = new u8[sizeof(T)*_block_size];
    _memory_blocks.push_back(block);

    add_block_to_free_list(block);
  }

  void add_block_to_free_list(u8* block){
    // cast to T => ptr++ moves to next T*
    T* ptr = reinterpret_cast<T*>(block);

    // Add evert T* in the block to the free list
    for(u64 i = 0; i < _block_size; i++){
      _free_list.push(ptr);
      ptr++;
    }
  }

  u64 _size;
  const u64 _block_size;

  Mode _mode;

  std::queue<T*> _free_list;
  std::vector<u8*> _memory_blocks;
};
