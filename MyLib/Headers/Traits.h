#ifndef DPTRAITS
#define DPTRAITS

#include <type_traits>
#include <iostream>
/*
* A few common traits used across multiple projects, and similar template utilities.
*/

namespace dp {

	//First, a pair of structs to determine if specialised template type T is a specialisation of template R.
	//Note these only work for templates which have typename template parameters, and will not work for other template parameters or a mix of the two.
	template<typename R, template<typename...>class T, typename... Args>
	struct isSpecialisation : std::false_type {};

	//In the event that T is a specialisation of the second argument, the call will resolve to this specialisation and allow us to fetch true from its value
	template<template<typename...>class T, typename... Args>
	struct isSpecialisation<T<Args...>, T> : std::true_type {};

	//Sample call: isSpecialisation<std::vector<int>,std::vector>::value -> resolves to true.


	//Some dependent true/false traits. To be used sparingly as tricking the compiler is rarely a good practice.
	//However, in some examples it is required, e.g. wanting a static_assert to fail in the else branch of a constexpr if in compilers which haven't implemented CWG2518
	template<typename T>
	static constexpr inline bool dependentFalse = false;

	template<typename T>
	static constexpr inline bool dependentTrue = true;


	//Traits to determine whether a type T supports extraction from a std::istream
	template<typename, typename = std::void_t<>>
	struct isExtractible : std::false_type {};

	template<typename T>
	struct isExtractible<T, std::void_t<decltype(std::declval<std::istream&>() >> std::declval<T&>())>> : std::true_type {};

	template<typename T>
	constexpr inline bool isExtractible_v{ isExtractible<T>::value };




}



#endif