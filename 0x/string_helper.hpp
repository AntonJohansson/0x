#pragma once

#include <string_view>
#include <string>
#include <charconv>

std::string_view get_next_word(std::string_view& sv){
	while(sv[0] == ' ' || sv[0] == '\n')sv.remove_prefix(1);

	auto i = sv.find_first_of(" \n");
	auto ret = sv.substr(0,i);
	sv.remove_prefix(i);

	return ret;
}

bool compare_next_word(std::string_view& sv, const std::string& word){
	while(sv[0] == ' ' || sv[0] == '\n')sv.remove_prefix(1);

	auto i = sv.find_first_of(" \n");
	if(sv.substr(0,i).compare(word) == 0){
		sv.remove_prefix(i);
		return true;
	}

	return false;
}

// Requires data(), size() member functions
// could probably add some compile time checks
template<typename DataT, typename NumberT>
bool to_number(DataT& str, NumberT& number){
	if(auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), number); ec == std::errc()){
		return true;
	}

	return false;
}


