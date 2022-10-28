#include<type_traits>

namespace utility {

	//First, a pair of structs to determine if specialised template type T is a specialisation of template R.
	//Note these only work for templates which have typename template parameters, and will not work for other template parameters or a mix of the two.
	template<typename R, template<typename...>class T, typename... Args>
	struct isSpecialisation : std::false_type {};

	//In the event that T is a specialisation of the second argument, the call will resolve to this specialisation and allow us to fetch true from its value
	template<template<typename...>class T, typename... Args>
	struct isSpecialisation<T<Args...>, T> : std::true_type {};

	//Sample call: isSpecialisation<std::vector<int>,std::vector>::value -> resolves to true.

}