#pragma once

#include <vector>
#include <cassert>
#include <optional>
#include <functional>
#include <type_traits>

// Since Optional references are not in the language yet
// I resorted to this.
template<typename T>
struct Optional{
	T* t = nullptr;

	Optional(){}
	Optional(T& _t) : t(&_t) {}

	operator bool(){return t != nullptr;}
	T& operator*(){return *t;}
};

// Open address hash table with linear probing.
//

template<typename Key, typename Value>
class HashMap{
	// The occupied flag will most likely not affect the size of the pair struct
	// since struct will probably be padded. Variant could be used instead.
	struct Pair{
		Key key;
		Value value;
		//bool occupied;
	};
public:
	HashMap(size_t size = 16, Key empty_key = {}) : _size{0}, _empty_key(empty_key) {
		// get power of two
		size_t size_pow2 = 1;
		while(size_pow2 < size)size_pow2 <<= 1;

		_pairs.resize(size_pow2, Pair{_empty_key, Value{}});
	}

	// Data manipulation
	//
	
	Pair& insert(Pair& p){
		assert(p.key != _empty_key && "Empty key should not be used");
		resize_if_necessary();

		for(auto i = key_to_index(p.key); ; i = probe_next(i)){
			//std::cout << "index: " << i << std::endl;
			auto& pair = _pairs[i];
			if(pair.key == _empty_key){
				pair.key = p.key;
				pair.value = p.value;
				_size++;
				return pair;
			}else if(pair.key == p.key){
				pair.value = p.value;
				return pair;
			}
		}
	}

	Pair& insert(Pair&& p){
		return insert(p);
	}

	template<typename... Args>
	Pair& emplace(const Key& k, Args&&... args){
		assert(k != _empty_key && "Empty key should not be used");
		resize_if_necessary();

		for(auto i = key_to_index(k); ; i = probe_next(i)){
			//std::cout << "index: " << i << std::endl;
			auto& pair = _pairs[i];
			if(pair.key == _empty_key){
				pair.key = k;
				pair.value = Value(std::forward<Args>(args)...);
				_size++;
				return pair;
			}else if(pair.key == k){
				return pair;
			}
		}
	}

	template<typename... Args>
	Pair& emplace(Key&& k, Args&&... args){
		return emplace(k, std::forward<Args>(args)...);
	}

	bool erase(Key& k){
		auto index_found = find(k);
		if(!index_found)return false;
		
		auto index_to_remove = *index_found;

		// we have to handle items that were placed via collision hashes before we remove.
		for(auto i = probe_next(index_to_remove); ; i = probe_next(i)){
			auto& pair = _pairs[i];
			if(pair.key == _empty_key){
				_pairs[index_to_remove].key = _empty_key;
				_size--;
				return true;
			}else if(size_t ideal = key_to_index(pair.key);
						 	 diff(index_to_remove, ideal) < diff(i, ideal)){
				// this branch is taken if the element we're looking at is not in its ideal position,
				// and the position of the element to remove happens to be close to that position.
				// This is done to avoid putting holes in the the table between items with colliding
				// hashes.
				_pairs[index_to_remove] = pair;
				index_to_remove = i;
			}
		}

		return false;
	}

	bool erase(Key&& k){
		return erase(k);
	}

	void clear(){
		clear_and_resize(_pairs.size());
	}

	// Access
	// 
	
	//std::optional<size_t> find(const Key& k){
	//	assert(k != _empty_key && "empty key should not be used");
	//	for(auto i = key_to_index(k); ; i = probe_next(i)){
	//		auto& pair = _pairs[i];
	//		if(pair.key == k){
	//			return i;
	//		}else if(pair.key == _empty_key){
	//			return {};
	//		}
	//	}
	//}

	//std::optional<size_t> find(Key&& k){
	//	return find(k);
	//}
	//
	//std::optional<Value&> at(const Key& k){
	//	if(auto i = find(k)){
	//		return _pairs[*i].value;
	//	}
	//	return {};
	//}

	//std::optional<Value&> at(const Key&& k){
	//	return at(k);
	//}
	
	std::optional<size_t> find(const Key& k){
		assert(k != _empty_key && "empty key should not be used");
		for(auto i = key_to_index(k); ; i = probe_next(i)){
			auto& pair = _pairs[i];
			if(pair.key == k){
				return i;
			}else if(pair.key == _empty_key){
				return {};
			}
		}
	}

	std::optional<size_t> find(Key&& k){
		return find(k);
	}
	
	Optional<Value> at(const Key& k){
		if(auto i = find(k)){
			return _pairs[*i].value;
		}
		return {};
	}

	Optional<Value> at(const Key&& k){
		return at(k);
	}
	
	Value& operator[] (Key& k){
		return emplace(k).value;
	}

	Value& operator[] (const Key& k) {
		return emplace(k).value;
	}

	Value& operator[] (Key&& k){
		return operator[](k);
	}

	Value& operator[] (const Key&& k) {
		return operator[](k);
	}

	// Size and table information
	//
	
	const size_t capacity() const {
		return _pairs.size();
	}

	const size_t size() const {
		return _size;
	}

	void print_table(){
		for(int i = 0; i < _pairs.size(); i++){
			auto& pair = _pairs[i];
			//std::cout << "{" << pair.key << ", " << pair.value << "}\n";
		}
	}

	void for_each(std::function<void(Pair&)> func){
		for(auto& p : _pairs){
			if(p.key != _empty_key){
				func(p);
			}
		}
	}

private:
	void clear_and_resize(size_t new_size){
		_size = 0;
		_pairs.clear();
		_pairs.resize(new_size, Pair{_empty_key, Value{}});
	}

	void rehash(size_t new_size){
		std::vector<Pair> copy_pairs{_pairs};

		clear_and_resize(new_size);
		
		for(size_t i = 0; i < copy_pairs.size(); i++){
			auto& pair = copy_pairs[i];
			if(pair.key == _empty_key)continue;
			insert(pair);
		}
	}

	void resize_if_necessary(){
		if(_size > _load_factor*_pairs.size()){
			rehash(_pairs.size() << 1);
		}
	}

	const size_t key_to_index(const Key& k) const {
		// Note if the size is a power of two
		// 	index % size = index & (size - 1)
		const size_t mask = _pairs.size() - 1;
		return std::hash<Key>{}(k) & mask;
	}

	const size_t probe_next(size_t index) const {
		// Note if the size is a power of two
		// 	index % size = index & (size - 1)
		const size_t mask = _pairs.size() - 1;
		return (index + 1) & mask;
	}

	const size_t diff(size_t a, size_t b) const {
		const size_t mask = _pairs.size() - 1;
		// _pairs.size() in order to handle b > a
		return (_pairs.size() + (a-b)) & mask;
	}

	size_t _size;
	Key _empty_key;
	std::vector<Pair> _pairs;
	float _load_factor{0.5f};
};
