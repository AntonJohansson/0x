#include "external/catch.hpp"
#include "0x/stack_allocator.hpp"
#include <iostream>

TEST_CASE("", "[stack_alloc]"){
	StackAllocator stack(sizeof(int)*10);

	int* ptrs[10];	
	for(int i = 0; i < 10; i++){
		ptrs[i] = stack.alloc<int>(i);
	}

	SECTION("alloc data corruption check"){
		for(int i = 0; i < 10; i++){
			REQUIRE(*ptrs[i] == i);
		}
	}

	SECTION("dealloc correct order"){

	}

}
