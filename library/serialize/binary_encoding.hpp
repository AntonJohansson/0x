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

constexpr size_t npos = static_cast<size_t>(-1);
inline BinaryData subdata(BinaryData& data, size_t b, size_t e = npos){
	auto begin = data.begin() + b;
	auto end   = (e != npos) ? data.begin() + e : data.end();

	return BinaryData(begin, end);
}

template<typename Iteratable,
		typename = std::enable_if_t<!std::is_convertible<Iteratable, uint8_t>::value>>
inline void insert(BinaryData& data, size_t pos, const Iteratable& t){
		data.insert(data.begin() + pos, std::begin(t), std::end(t));
}

template<typename Iteratable,
		typename = std::enable_if_t<!std::is_convertible<Iteratable, uint8_t>::value>>
inline void append(BinaryData& data, const Iteratable& t){
		insert(data, data.size(), t);
}

inline void append(BinaryData& data, uint8_t i){
	data.push_back(i);
}

inline void append(BinaryData& data, uint8_t* buffer, size_t size){
	data.insert(data.end(), buffer, buffer + size);
}

namespace encode{

// I don't remeber how this works anymore :/
template<typename T>
inline void integer(BinaryData& data, T t){
	for(int8_t s = 8*sizeof(T) - 8; s >= 0; s -= 8){
		append(data, static_cast<uint8_t>(t >> s));
	}
}

template<typename... Args>
inline void multiple_integers(BinaryData& data, Args... args){
	(integer<Args>(data, args), ...);
}

#define ENC_INT_TYPE(name, type) \
	template<typename T> inline void name (BinaryData& data, T t) {\
		integer(data, static_cast< type >(t)); \
	}

ENC_INT_TYPE(u8,  uint8_t)
ENC_INT_TYPE(u16, uint16_t)
ENC_INT_TYPE(u32, uint32_t)
ENC_INT_TYPE(u64, uint64_t)
ENC_INT_TYPE(i8,  int8_t)
ENC_INT_TYPE(i16, int16_t)
ENC_INT_TYPE(i32, int32_t)
ENC_INT_TYPE(i64, int64_t)


inline void string(BinaryData& data, const std::string& str){
	// TODO this might not be optimal
	u32(data, str.size());
	append(data, str);
}

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
inline void integer(BinaryData& data, T& t){
	T decoded_t = 0;
	for(uint8_t i = 0; i < sizeof(T); i++){
		decoded_t |= static_cast<T>(data[i]) << (8*sizeof(T) - 8*(i+1));
	}
	data.erase(data.begin(), data.begin() + sizeof(T));
	t = decoded_t;
}

template<typename... Args>
inline void multiple_integers(BinaryData& data, Args&... args){
	(integer<Args>(data, std::forward<Args&>(args)), ...);
}

inline std::string string(BinaryData& data){
	uint32_t size;
	integer(data, size);

	auto sub = subdata(data, 0, size);
	data.erase(data.begin(), data.begin() + size);

	return std::string(sub.begin(), sub.end());
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
