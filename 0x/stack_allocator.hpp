#pragma once

//#include <utility>
#include <queue>
#include <vector>
#include <cstdlib>

#include <new>

#include "number_types.hpp"

struct MemoryBlock{
	u8* block_ptr;
	u8* last_allocated_ptr;
	size_t top_of_stack;
	size_t size;

	template<typename T, typename... Args>
	inline T* alloc(Args&&... args){
		T* ptr = reinterpret_cast<T*> (block_ptr + size);
		last_allocated_ptr = block_ptr + size;

    ptr = new (ptr) T(std::forward<Args>(args)...);

		size += sizeof(T);

		return ptr;
	}

	inline size_t free_memory(){
		return size - top_of_stack;
	}
};

class Allocator{};

class StackAllocator : public Allocator{
public:
  explicit StackAllocator(u64 block_size = 16)
    :_block_size(block_size) {
    allocate_block();
  }

  ~StackAllocator(){
    for(auto& block : _memory_blocks){
      delete[] block.block_ptr;
    }
  }

  template<typename T, typename... Args>
  T* alloc(Args&&... args){
		size_t size = sizeof(T);

		assert(_block_size >= size);

		MemoryBlock* memory_block = nullptr;
		for(auto& block : _memory_blocks){
			if(block.free_memory() >= size){
				memory_block = &block;
			}
		}

		if(!memory_block){
			memory_block = allocate_block();
		}

		T* ptr = memory_block->alloc<T>(std::forward<Args>(args)...);

    return ptr;
  }

  //void dealloc(T* t){
  //  // Call destructor
  //  t->~T();

  //  // "free" memory
  //  _free_list.push(t);
  //  _size--;

  //  // @TODO should i dealloc a block if it becomes emtpy?
  //}

  //void reset(){
  //  _size = 0;

  //  // Free all pointers, but do not dealloc blocks
  //  for(u8* block : _memory_blocks){
  //    add_block_to_free_list(block);
  //  }
  //}

  //inline size_t size()			{return _size;}
  //inline size_t block_size(){return _block_size;}
  //inline size_t blocks()		{return _memory_blocks.size();}
  //inline size_t capacity()	{return _block_size*_memory_blocks.size();}
  //inline size_t free()			{return _free_list.size();}

  //float load_factor(){return _size/capacity();}

private:
  MemoryBlock* allocate_block(){
    u8* block = new u8[_block_size];
    _memory_blocks.push_back({block, 0, _block_size});
		return &*_memory_blocks.end();
  }

  //void add_block_to_free_list(u8* block){
  //  // cast to T => ptr++ moves to next T*
  //  T* ptr = reinterpret_cast<T*>(block);

  //  // Add evert T* in the block to the free list
  //  for(u64 i = 0; i < _block_size; i++){
  //    _free_list.push(ptr);
  //    ptr++;
  //  }
  //}

  const size_t _block_size;

  std::vector<MemoryBlock> _memory_blocks;
};
