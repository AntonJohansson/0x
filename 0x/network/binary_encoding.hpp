#pragma once

#include <stdint.h>
#include <vector>
#include <tuple>
#include <type_traits>

#define CONCAT_IMPL(A,B,C) A ## B ## C
#define CONCAT(A,B,C) CONCAT_IMPL(A,B,C)

#define SIGNED_INT_TYPE(size)		CONCAT(int, size,_t)
#define UNSIGNED_INT_TYPE(size) CONCAT(uint,size,_t)

using BinaryData = std::vector<uint8_t>;

inline void append(BinaryData& data, uint8_t i){
	data.push_back(i);
}

namespace encode{

// I don't remeber how this works anymore :/
template<typename T>
void integer(BinaryData& data, T t){
	for(int8_t s = 8*sizeof(T) - 8; s >= 0; s -= 8){
		append(data, static_cast<uint8_t>(t >> s));
	}
}

template<typename... Args>
void multiple_integers(BinaryData& data, Args... args){
	(integer<Args>(data, args), ...);
}

template<typename T> inline void u8 (BinaryData& data, T t){uint(data, static_cast<uint8_t> (t));}
template<typename T> inline void u16(BinaryData& data, T t){uint(data, static_cast<uint16_t>(t));}
template<typename T> inline void u32(BinaryData& data, T t){uint(data, static_cast<uint32_t>(t));}
template<typename T> inline void u64(BinaryData& data, T t){uint(data, static_cast<uint64_t>(t));}

template<typename T> inline void i8 (BinaryData& data, T t){uint(data, static_cast<int8_t> (t));}
template<typename T> inline void i16(BinaryData& data, T t){uint(data, static_cast<int16_t>(t));}
template<typename T> inline void i32(BinaryData& data, T t){uint(data, static_cast<int32_t>(t));}
template<typename T> inline void i64(BinaryData& data, T t){uint(data, static_cast<int64_t>(t));}

}

namespace decode{

//template<typename T>
//	T decode_uint(){
//		auto ti = decode_type_info(*this);
//		T decoded_t = 0;
//		for(u8 i = 0; i < sizeof(T); i++){
//			decoded_t |= static_cast<T>(_data[i]) << (8*sizeof(T) - 8*(i+1));
//		}
//		_data.erase(_data.begin(), _data.begin() + ti.size);
//		return decoded_t;
//}

template<typename T>
void integer(BinaryData& data, T& t){
	T decoded_t = 0;
	for(uint8_t i = 0; i < sizeof(T); i++){
		decoded_t |= static_cast<T>(data[i]) << (8*sizeof(T) - 8*(i+1));
	}
	data.erase(data.begin(), data.begin() + sizeof(T));
	t = decoded_t;
}

template<typename... Args>
void multiple_integers(BinaryData& data, Args&... args){
	(integer<Args>(data, std::forward<Args&>(args)), ...);
}

//template<typename SignedInt, typename UnsignedInt>
//SignedInt decode_signed_int(){
//	auto unsigned_value = decode_uint<UnsignedInt>();
//	if(unsigned_value <= std::numeric_limits<SignedInt>::max()){
//		return unsigned_value;
//	}else{
//		return -1 - (std::numeric_limits<UnsignedInt>::max() - unsigned_value);
//	}
//}

//template<typename SignedInt, typename UnsignedInt>
//SignedInt _int(BinaryData& data){
//	auto unsigned_value = uint<UnsignedInt>();
//	if(unsigned_value <= std::numeric_limits<SignedInt>::max()){
//		return unsigned_value;
//	}else{
//		return -1 - (std::numeric_limits<UnsignedInt>::max() - unsigned_value);
//	}
//}

}
