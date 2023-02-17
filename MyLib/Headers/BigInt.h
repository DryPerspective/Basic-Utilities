#ifndef BIGINT
#define BIGINT
#pragma once
/*
* A "BigInt" class - a class which can represent an arbitrarily large (signed) integer. Intended only for cases where standard numerical types can't contain it as it is inherently
* less performant than primitive types.
*/

#include<iostream>
#include<vector>
#include<string>
#include<limits>

namespace dp {

	class BigInt
	{
	public:
		//The type used should hopefully not change and this alias is largely a stayover from testing, however some specific cases may require it to be changed.
		using arrayType = uint64_t;


	private:

		bool						m_sign;		//The sign. True = positive, false = negative
		std::vector<arrayType>		m_bits;		//The data structure which contains our number's value. We use fixed-width unsigned intergers as bit collections to represent a larger number.
		//The array is "little endian" in that the index of 0 represents the least significant term.

		static constexpr arrayType	unitSize{ 8 * sizeof(arrayType) };					//As we can hypothetically use any unsigned integer type for this object, we define these constants to represent that type's limits.
		static constexpr arrayType	maxBitValue{ std::numeric_limits<arrayType>::max() };	//The maximum value which can be represented per entry in our "bits" array. Used for checking for overflow.


		/*
		* INTERNAL PROCESSING FUNCTIONS
		*/
		//Trim leading zeroes off a BigInt so an m_bits vector never contains unnecessary values.
		void trimLeadingZeroes();

		//A working function to match the sizes of the internal arrays of two BigInts. 
		void matchVectorSize(const BigInt& toMatch);

		void matchVectorSize(std::size_t inSize);

		//Returns the bit representing for the inIndex'th bit of inValue
		static bool getNthBit(arrayType inValue, std::size_t inIndex);

		//Ditto for the nth bit of the composite bit array.
		bool getTotalNthBit(std::size_t inIndex) const;

		//Set a specific bit of one component to a specific value.
		static void setNthBit(arrayType& inValue, std::size_t inIndex, bool inBit);

		//Ditto for the nth bit of the composite bit array.
		void setTotalNthBit(std::size_t inIndex, bool inBit);

		//As division and modulo use essentially the same algorithm, they share the underlying code here.
		void divide(const BigInt& dividend, const BigInt& divisor, BigInt& solution, bool returnRemainder) const;

		/*
		* REPRESENTATION FUNCTIONS
		*/
		//Used to get a string representing the total number
		std::string getBinaryString() const;
		std::string getDecimalString() const;


	public:
		/*
		* CONSTRUCTOR AND DESTRUCTOR
		*/
		BigInt();
		BigInt(const BigInt&) = default;
		BigInt(BigInt&&) noexcept = default;
		BigInt(const std::string& inNumber);			//Currently unimplemented pending a better algorithm for converting between bases 10 and 2.
		BigInt(arrayType inVal, bool sign = true);


		virtual ~BigInt() = default;

		/*
		* ARITHMETIC OPERATORS
		*/
		BigInt operator-() const;
		BigInt operator+(const BigInt& inInt) const;
		BigInt operator-(const BigInt& inInt) const;
		BigInt operator*(const BigInt& inInt) const;
		BigInt operator/(const BigInt& inInt) const;
		BigInt operator%(const BigInt& inInt) const;
		BigInt& operator++();
		BigInt operator++(int);
		BigInt& operator--();
		BigInt operator--(int);

		//We provide friend function variants of these operators to preserve commutivity with non-BigInt integer types.
		friend BigInt operator+(arrayType inUInt, const BigInt& inInt);
		friend BigInt operator*(arrayType inUInt, const BigInt& inInt);

		/*
		* COMPARISON OPERATORS
		*/
		bool operator==(const BigInt& inInt) const;
		//Shortcut equality to compare to smaller integer values.
		bool operator==(arrayType inInt) const;
		bool operator!=(const BigInt& inInt) const;
		bool operator<(const BigInt& inInt) const;
		bool operator<=(const BigInt& inInt) const;
		bool operator>(const BigInt& inInt) const;
		bool operator>=(const BigInt& inInt) const;

		/*
		* BITWISE OPERATORS
		*/
		BigInt operator<<(arrayType inInt) const;
		BigInt operator>>(arrayType inInt) const;
		BigInt operator&(const BigInt& inInt) const;
		BigInt operator|(const BigInt& inInt) const;
		BigInt operator^(const BigInt& inInt) const;
		BigInt operator~() const;

		//Expanding Left Shift. This operates as a bitwise << operator, except it will expand the range of the BigInt to fit the equation rather than truncating off any overflow.
		BigInt xLS(arrayType inInt) const;


		/*
		* ASSIGNMENT OPERATORS
		*/
		BigInt& operator=(const BigInt&) = default;
		BigInt& operator=(BigInt&& inInt) noexcept = default;
		BigInt& operator+=(const BigInt& inInt);
		BigInt& operator-=(const BigInt& inInt);
		BigInt& operator*=(const BigInt& inInt);
		BigInt& operator/=(const BigInt& inInt);
		BigInt& operator%=(const BigInt& inInt);
		BigInt& operator<<=(arrayType inInt);
		BigInt& operator>>=(arrayType inInt);
		BigInt& operator&=(const BigInt& inInt);
		BigInt& operator|=(const BigInt& inInt);
		BigInt& operator^=(const BigInt& inInt);


		/*
		* MISC FUNCTIONALITY
		*/
		bool sign() const;
		BigInt abs() const;
		//Determines if a BigInt can be safely cast to the arrayType without losing value.
		bool canBeShortened() const;
		explicit operator arrayType() const;
		std::string toString(int base = 10) const;



	};
}

namespace numeric {
	using BigInt [[deprecated("BigInt now exists in namespace dp")]] = dp::BigInt;
}

#endif