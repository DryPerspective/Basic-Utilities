#include<type_traits>
#include <string>
#include <string_view>

namespace utility {

	//First, a pair of structs to determine if specialised template type T is a specialisation of template R.
	//Note these only work for templates which have typename template parameters, and will not work for other template parameters or a mix of the two.
	template<typename R, template<typename...>class T, typename... Args>
	struct isSpecialisation : std::false_type {};

	//In the event that T is a specialisation of the second argument, the call will resolve to this specialisation and allow us to fetch true from its value
	template<template<typename...>class T, typename... Args>
	struct isSpecialisation<T<Args...>, T> : std::true_type {};

	//Sample call: isSpecialisation<std::vector<int>,std::vector>::value -> resolves to true.

	//Grab a string view of a substring from a particular string.
	inline std::string_view substr_view(const std::string& source, std::size_t offset = 0, std::string_view::size_type count = std::numeric_limits<std::string_view::size_type>::max()) {
		if (offset < source.size())   return std::string_view(source.data() + offset, std::min(source.size() - offset, count));
		return {};
	}

	//Delete rvalue reference version to prevent returning a view to a non-existent string.
	//Default args may seem redundant for a deleted function but without them calls with fewer than 3 args will resolve to the lvalue version before this one.
	std::string_view substr_view(const std::string&&, std::size_t = 0, std::string_view::size_type = std::numeric_limits<std::string_view::size_type>::max()) = delete;


}