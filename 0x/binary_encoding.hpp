#pragma once

#include <stdint.h>
#include <vector>

using BinaryData = std::vector<uint8_t>;

inline void append(BinaryData& data, uint8_t i){
	data.push_back(i);
}

namespace encode{

// I don't remeber how this works anymore :/
template<typename UnsignedInt>
void uint(BinaryData& data, UnsignedInt t){
	for(int8_t s = 8*sizeof(UnsignedInt) - 8; s >= 0; s -= 8){
		append(data, static_cast<uint8_t>(t >> s));
	}
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
T uint(BinaryData& data){
	T decoded_t = 0;
	for(uint8_t i = 0; i < sizeof(T); i++){
		decoded_t |= static_cast<T>(data[i]) << (8*sizeof(T) - i*(i+1));
	}
	data.erase(data.begin(), data.begin() + sizeof(T));
	return decoded_t;
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

}
