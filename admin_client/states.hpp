#pragma once

#include <string>
#include <stdio.h>
#include <memory/hash_map.hpp>
#include <serialize/murmur_hash3.hpp>

namespace state{

using EnterCallback 	= void(*)(void);
using EventCallback 	= void(*)(void*);
using UpdateCallback 	= void(*)(void*);
using LeaveCallback 	= void(*)(void);

struct State{
	EnterCallback enter;
	EventCallback event;
	UpdateCallback update;
	EnterCallback leave;
};

static HashMap<uint32_t, State> states;
State* current_state = nullptr;

inline uint32_t hash(const std::string& name){
	return murmur::MurmurHash3_x86_32(name.data(), name.size(), 123);
}

inline void add(const std::string& name, State state){
	states.insert({hash(name), state});
}

inline void change(const std::string& name){
	if(auto found = states.at(hash(name))){
		if(current_state && current_state->leave)
			current_state->leave();

		current_state = &(*found);

		if(current_state->enter)
			current_state->enter();
	}else{
		printf("State %s not found\n", name.c_str());
	}
}

inline void event(void* data){
	if(current_state && current_state->event)
		current_state->event(data);
}

inline void update(void* data){
	if(current_state && current_state->update)
		current_state->update(data);
}
 
}
