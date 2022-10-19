#include "BigInt.h"

/*
* PRIVATE FUNCTIONS
*/


void BigInt::trimLeadingZeroes() {
	//To prevent a for loop which could execute indefinitely considering size_t is unsigned, we count up and use arithmetic to simulate counting down.
	auto maxSize{ m_bits.size() };
	for (std::size_t i = 0; i < maxSize; ++i) {
		if (m_bits[maxSize - i - 1] == 0) m_bits.pop_back();//This should be a safe operation as popping m_bits[i] doesn't change the value returned by m_bits[i-1]
		else break;										//Break after the first non-zero element as we only want to trim leading zeroes.
	}
	if (m_bits.size() == 0)m_bits.push_back(0);			//In the case where this function is called on a BigInt with value zero, we don't want to leave the BigInt empty.
}


void BigInt::matchVectorSize(const BigInt& toMatch) {
	m_bits.reserve(toMatch.m_bits.size());
	for (auto i = m_bits.size(); i < toMatch.m_bits.size(); ++i) {
		m_bits.push_back(0);		
	}
}

void BigInt::matchVectorSize(std::size_t inSize) {
	m_bits.reserve(inSize);
	for (auto i = m_bits.size(); i < inSize; ++i) {
		m_bits.push_back(0);
	}

}

bool BigInt::getNthBit(arrayType inValue, std::size_t inIndex) {
	arrayType bit{ (inValue & (static_cast<arrayType>(1) << inIndex)) >> inIndex };
	return static_cast<bool>(bit);
}

bool BigInt::getTotalNthBit(std::size_t inIndex) const {
	auto remainder{ inIndex % unitSize };
	auto term{ inIndex / unitSize };	
	return getNthBit(m_bits[term], remainder);
}

void BigInt::setNthBit(arrayType& inValue, std::size_t inIndex, bool inBit) {
	if (getNthBit(inValue, inIndex) != inBit) {
		inValue ^= static_cast<arrayType>(1) << inIndex;
	}


}

void BigInt::setTotalNthBit(std::size_t inIndex, bool inBit) {
	auto remainder{ inIndex % unitSize };
	auto term{ inIndex / unitSize };
	setNthBit(m_bits[term], remainder, inBit);
}

//Our division function, using a somewhat optimised long division method.
//We pass the solution in by non-const reference from our operator/ and operator% so that we don't need to make an unnecessary copy between functions.
void BigInt::divide(const BigInt& dividend, const BigInt& divisor, BigInt& solution, bool returnRemainder) const {
	//We make the choice to let dividing by 0 be UB rather than be the only function in the class to throw an exception.
	//In this case, solution will have been default-initialised to 0
	if (divisor == 0) {
		return;
	}
	if (divisor == 1) {
		solution = dividend;
		return;
	}
	//Integer division => All cases of A/B for A < B -> 0.
	if (dividend < divisor) {
		if (returnRemainder) solution = dividend;
		return;
	}
	
	BigInt quotient;
	BigInt remainder;
	quotient.matchVectorSize(dividend);
	remainder.matchVectorSize(divisor);
	auto totalBits{ unitSize * dividend.m_bits.size() };
	for (auto i = 0; i < totalBits; ++i) {			//Can't count down to 0 as i will likely be an unsigned type

		//This if statement accounts for the fact that remainder may overflow its initial bounds. We unfortunately can't cleanly plug remainder with extra zeroes before the loop.
		//As we don't necessarily know how long remainder will need to be, and it may interfere with the calculations of operator>=
		if (getNthBit(remainder.m_bits[remainder.m_bits.size() - 1], unitSize - 1)) remainder.m_bits.push_back(0);
		remainder <<= 1;	
		remainder.setTotalNthBit(0, dividend.getTotalNthBit(totalBits - i - 1));
		if (remainder >= divisor) {
			remainder -= divisor;
			quotient.setTotalNthBit(totalBits - i - 1, true);
		}
	}
	if (returnRemainder) {
		remainder.trimLeadingZeroes();
		solution = std::move(remainder);
	}
	else {
		quotient.trimLeadingZeroes();
		solution = std::move(quotient);
	}
	return;
	
}

std::string BigInt::getBinaryString() const {
	std::string output{};
	for (auto j = 0; j < m_bits.size(); ++j) {
		for (auto i = 0; i < unitSize; ++i) {
			if (getNthBit(m_bits[m_bits.size() - j - 1], unitSize - 1 - i)) {
				output += "1";
			}
			else output += "0";
		}
		output += ' ';
	}
	return output;

}

//I know this is horribly inefficient. It will be optimised later.
std::string BigInt::getDecimalString() const {
	if (*this == 0) return std::string{ "0" };

	std::string output{};
	BigInt buffer{ *this };
	buffer.m_sign = true;
	char digit;
	while (buffer > 0) {
		digit = '0' + static_cast<char>(static_cast<arrayType>(buffer % 10));
		output += digit;

		buffer /= 10;

	}
	if (!m_sign) output += '-';
	std::reverse(output.begin(), output.end());
	return output;

}


/*
* CONSTRUCTORS
*/

BigInt::BigInt() {
	m_sign = true;
	m_bits.push_back(0);
}

//In the event that we want to construct from a value which can fit in unsigned unitSize bits, this is trivial.
BigInt::BigInt(arrayType inVal, bool sign) : m_sign{ sign } {
	m_bits.push_back(inVal);
}


/*
* ARITHMETIC OPERATORS
*/
//Unary minus operator
BigInt BigInt::operator-() const {
	BigInt negativeInt{ *this };
	negativeInt.m_sign = !m_sign;
	return negativeInt;
}

//Simple addition
BigInt BigInt::operator+(const BigInt& inInt) const {
	//Hurdle 1 - the signs of this and inInt may differ.
	if (m_sign != inInt.m_sign) {
		//If this is positive, inInt is negative. A + (-B) for positive A,B = A - B. In code, we can do this as A - |B|.
		if (m_sign) return (*this - inInt.abs());
		//Otherwise this is negative, inInt is positive. (-A) + B for positive A,B = B - A. In code, B - |A|.
		else return(inInt - this->abs());
	}
	
	//Hurdle 2 - we cannot necessarily assume the internal vectors for both numbers will be the same.
	//In addition this is simple to resolve. Our solution will have length equal to the longer operand, but we only need to iterate along the shorter operand to do addition.
	//The only potential hitch in this plan is overflow. But it is handled internally.
	BigInt solution{ *this };
	solution.matchVectorSize(inInt);					//As matchVectorSize will never decrease the size of a vector, this guarantees that solution is the right size.
	std::size_t smallerSize;
	std::size_t biggerSize;
	std::size_t thisSize{ this->m_bits.size() };
	std::size_t inIntSize{ inInt.m_bits.size() };
	if ( thisSize >= inIntSize ) {
		biggerSize = thisSize;
		smallerSize = inIntSize;
	}
	else {
		biggerSize = inIntSize;
		smallerSize = thisSize;
	}
	bool overflow{ false };
	constexpr arrayType	getMostSignificantBit = static_cast<arrayType>(1) << (unitSize - 1);					//Equivalent to 1000000... in binary.
	constexpr arrayType	removeMostSignificantBit = ~getMostSignificantBit;				//Equivalent to 0111111... in binary
	//Now the prep work is done we can do addition. The algorithm for this is relatively simple. In cases where the numbers don't overflow the container, solution[i] = a[i] + b[i].
	//We account for overflow in a bitwise manner. If we separate the most significant bit from each term to be added, add the 31-bit numbers together, and check the unitSizend bit of that solution,
	//the sum of those three most significant bits will tell you whether the container overflowed, and what state the most significant bit should be in.
	//In the event of overflow, we just need to add one to the next most significant vector element which can have 1 added without overflowing itself.
	for (auto i = 0; i < smallerSize; ++i) {
		//First we separate the most significant bit from the rest of each term.
		auto term1MostSigBit{ BigInt::getNthBit(solution.m_bits[i],unitSize - 1) };
		auto term2MostSigBit{ BigInt::getNthBit(inInt.m_bits[i],unitSize - 1) };

		auto term1{ removeMostSignificantBit & solution.m_bits[i] };
		auto term2{ removeMostSignificantBit & inInt.m_bits[i] };

		//Then we add the terms together and check if they overflowed into the most significant bit.
		//+= as it's possible this solution term has already been given a value due to overflow on the previous term.
		solution.m_bits[i] = term1 + term2;
		auto slnMostSigBit{ BigInt::getNthBit(solution.m_bits[i],unitSize - 1) };

		auto bitSum{ term1MostSigBit + term2MostSigBit + slnMostSigBit };

		//If the three bits sum to bigger than 1, the number overflowed.
		if (bitSum > 1) {
			overflow = true;
			//If it's bigger than 2, we know the most sig bit of our solution's ith arrayType is 1.
			if (bitSum > 2) {
				solution.m_bits[i] |= getMostSignificantBit;
			}
			//Otherwise we know it must be 0
			else {
				solution.m_bits[i] &= ~(getMostSignificantBit);
			}
		}
		//If the three bits are <=1, the most significant bit will be the sum of the three most sig bits.
		else if (static_cast<int>(slnMostSigBit) != bitSum) {
			solution.m_bits[i] ^= getMostSignificantBit;			
		}

		//Now all that's left is to deal with overflow.
		if (overflow) {
			//As this is effectively a bit array rather than a number, if bits[i] overflows, we only need to add 1 to bits[i+1]. This gives three possible scenarios:
			//1. It is possible to add 1 to bits[i+1]. In this case that is exactly what we do.
			//2. bits[i+1] is at the maximum possible value and cannot have 1 added without overflowing itself. In this case we set it to 0 and continue searching for the next term which can have bits added.
			//3. bits[i] is the most significant term in the final number already. In this case we add an additional term of 1.
			//Option 2 shouldn't ever happen with how we've set up our system.
			for (auto j = i; j < biggerSize; ++j) {
				//First the simplest case to code - if the most significant term in the number overflows, we add a new most significant digit with value 1.
				if (j == biggerSize - 1) {
					solution.m_bits.push_back(1);
					break;
				}
				//If the next value up can't be added to without overflowing, we set it to 0 and continue.
				else if (solution.m_bits[j + 1] == maxBitValue) {
					solution.m_bits[j + 1] = 0;
				}
				//Otherwise we add 1 to the next term up.
				else {
					solution.m_bits[j + 1] += 1;
					break;
				}
			}
			overflow = false;
		}
	}
	solution.trimLeadingZeroes();
	return solution;
}

BigInt BigInt::operator-(const BigInt& inInt) const {
	//Hurdle 1: Crossing 0. Fortunately we can use the anticommutative nature of subtraction to get around this pretty easily.
	if ((*this) < inInt) {
		return -(inInt - *this);
	}
	//Hurdle 2: Differing signs. Fortunately this is also simple.
	if (m_sign != inInt.m_sign) {
		//A - (-1)B = A + B
		if (m_sign) return (*this) + inInt.abs();
		//(-1)A - B = -(A + B)
		else return -(this->abs() + inInt);
	}
	//Otherwise subtraction is relatively simple.
	BigInt solution{ *this };
	std::size_t thisSize{ this->m_bits.size() };
	std::size_t inIntSize{ inInt.m_bits.size() };
	bool carryOver{ false };
	for (auto i = 0; i < inIntSize; ++i) {			//We know that thisSize >= inIntSize as we know that this >= inInt
		//Even though the overall this number is smaller than inInt, that does not guarantee that each component of this is smaller than its respective inInt component.
		//Then we do the easy case of termA > termB and no carryOver
		if (!carryOver && (this->m_bits[i] >= inInt.m_bits[i]))solution.m_bits[i] -= inInt.m_bits[i];
		else {
			//Otherwise, we unfortunately have to do this by bitwise iteration
			for (auto j = 0; j < unitSize; ++j) {
				bool term1Bit{ getNthBit(m_bits[i],j) };
				bool term2Bit{ getNthBit(inInt.m_bits[i],j) };

				auto bitSum{ term1Bit - term2Bit - carryOver };
				if (bitSum == -2)setNthBit(solution.m_bits[i], j, 0);
				else setNthBit(solution.m_bits[i], j, ::abs(bitSum));

				if (bitSum < 0)carryOver = true;
				else carryOver = false;	
			}
			
		}
	}
	//This algorithm should hold even between terms of m_bits,  however there is one edge case - we reach the end of inInt and there's still carryOver.
	//As we know *this > inInt, this case can only occur when there are additional *this terms to subtract from, and we know that all further inInt terms are implicitly 0.
	//It is very unlikely that we will go the full length of these loops, as all we want is the first 1 in the binary of *this. But we must account for the fringe case where we don't get it.
	if (carryOver) {
		for (auto i = inIntSize; i < thisSize; ++i) {
			for (auto j = 0; j < unitSize; ++j) {
				bool term1Bit{ getNthBit(solution.m_bits[i],j) };
				if (term1Bit) {
					setNthBit(solution.m_bits[i], j, false);
					carryOver = false;
					break;
				}
				else {
					setNthBit(solution.m_bits[i], j, true);
				}
			}
			if (!carryOver)break;
		}
	}

	solution.trimLeadingZeroes();
	return solution;
}

BigInt BigInt::operator*(const BigInt& inInt) const {
	BigInt solution;
	auto solutionSize{ this->m_bits.size() + inInt.m_bits.size() };
	//The number of bits to represent A*B = bits to represent A + bits to represent B.
	solution.matchVectorSize(solutionSize);
	for (auto i = 0; i < unitSize * m_bits.size(); ++i) {
		//0 terms in one operand can be skipped.
		if (this->getTotalNthBit(i)) {
			BigInt intermediate;
			//Note we explicitly set intermediate to this and not solution.size() as solution gets all excess zeroes popped off in the += stage.
			intermediate.matchVectorSize(solutionSize);
			for (auto j = 0; j < unitSize * inInt.m_bits.size(); ++j) {
				if (inInt.getTotalNthBit(j)) {
					intermediate.setTotalNthBit(i + j, true);
				}
			}
			solution += intermediate;
			solution.matchVectorSize(solutionSize);
		}
	}
	if (m_sign != inInt.m_sign) solution.m_sign = false;
	solution.trimLeadingZeroes();
	return solution;
}

BigInt BigInt::operator/(const BigInt& inInt) const {
	BigInt solution;
	divide(*this, inInt, solution, false);
	if (m_sign != inInt.m_sign) solution.m_sign = false;
	return solution;
}

BigInt BigInt::operator%(const BigInt& inInt) const {
	BigInt solution;
	divide(*this, inInt, solution, true);
	//We follow "truncated modulo" convention for negative signs.
	if (!m_sign)solution.m_sign = false;
	return solution;
}

BigInt& BigInt::operator++() {
	//We avoid a potentially complex addition algorithm where possible.
	if (m_bits[0] < std::numeric_limits<arrayType>::max()) {
		++m_bits[0];
		return *this;
	}
	else {
		*this += 1;
		return *this;
	}
}

BigInt BigInt::operator++(int) {
	BigInt copy{ *this };
	++(*this);
	return copy;
}

BigInt& BigInt::operator--() {
	if (m_bits[0] > 0) {
		--m_bits[0];
		return *this;
	}
	else {
		*this -= 1;
		return *this;
	}
}

BigInt BigInt::operator--(int) {
	BigInt copy{ *this };
	--(*this);
	return copy;
}

BigInt operator+(BigInt::arrayType inUInt, const BigInt& inInt) {
	return inInt + inUInt;
}

BigInt operator*(BigInt::arrayType inUInt, const BigInt& inInt) {
	return inInt * inUInt;
}

/*
* COMPARISON OPERATORS
*/
bool BigInt::operator==(const BigInt& inInt) const {
	if (&inInt == this)return true;								//An object is obviously equal to itself
	if (m_sign != inInt.m_sign) return false;					//Differing signs => not equal.
	if (inInt.m_bits.size() != m_bits.size()) return false;		//Differing vector sizes means different values (or an ill-formed BigInt)
	for (std::size_t i = 0; i < m_bits.size(); ++i) {			//Otherwise just iterate over the values and return false early if a mismatch occurs
		if (m_bits[i] != inInt.m_bits[i])return false;
	}
	return true;												//If we get to here we know the result is true.
}

//We include a "shortcut" equality operator for cases such as division where we need to ensure we're not dividing by zero.
bool BigInt::operator==(arrayType inInt) const {
	if (m_bits.size() != 1) return false;
	return(m_bits[0] == inInt);
}

bool BigInt::operator!=(const BigInt& inInt) const {
	return !(*this == inInt);
}

bool BigInt::operator<(const BigInt& inInt) const {
	if (this == &inInt) return false;				//An object is neither greater or less than itself.
	if (m_sign != inInt.m_sign) {					//All positive numbers are bigger than all negative, so if the signs differ we can do easy comparison.
		if (m_sign) return false;
		else return true;
	}
	std::size_t thisSize{ this->m_bits.size() };
	std::size_t inIntSize{ inInt.m_bits.size() };
	if (thisSize < inIntSize) return true;			//Simple check first. If the vectors are of differing sizes it's easy to deduce which represents a larger number.
	else if (thisSize > inIntSize) return false;
	//Otherwise we just compare, starting at the most significant value.
	//As we need indexing for both vectors and can't count down below 0 on an unsigned type, we need to do a slightly weird for loop.
	for (auto i = 0; i < thisSize; ++i) {
		if (this->m_bits[thisSize - i - 1] < inInt.m_bits[thisSize - i - 1]) return true;
		if (this->m_bits[thisSize - i - 1] > inInt.m_bits[thisSize - i - 1]) return false;
	}
	//Down here the only possibility is that we have two identical numbers.
	return false;
}

bool BigInt::operator<=(const BigInt& inInt) const {
	return !(*this > inInt);
}
bool BigInt::operator>(const BigInt& inInt) const {
	return (*this != inInt && !(*this < inInt));
}
bool BigInt::operator>=(const BigInt& inInt) const {
	return !(*this < inInt);
}

/*
* BITWISE OPERATORS
*/
BigInt BigInt::operator<<(arrayType inInt) const {
	if (inInt == 0)return BigInt{ *this };

	auto totalBits{ unitSize * m_bits.size() };
	//If we're shifting by more bits than exist in the BigInt, we return 0;
	if (inInt >= totalBits) {
		return BigInt{ 0 };
	}
	else {
		BigInt solution{ *this };

		for (auto i = 0; i < totalBits - inInt; ++i) {
			solution.setTotalNthBit(totalBits - i - 1, solution.getTotalNthBit(totalBits - i - inInt - 1));
		}
		for (auto i = totalBits - inInt; i < totalBits; ++i) {
			solution.setTotalNthBit(totalBits - i - 1, false);
			}
			
		return solution;
	}

}

BigInt BigInt::operator>>(arrayType inInt) const {
	if (inInt == 0)return BigInt{ *this };

	auto totalBits{ unitSize * m_bits.size() };
	//If we're shifting by more bits than exist in the BigInt, we return 0;
	if (inInt >= totalBits) {
		return BigInt{ 0 };
	}
	else {
		BigInt solution{ *this };

		for (auto i = 0; i < totalBits - inInt; ++i) {
			solution.setTotalNthBit(i, solution.getTotalNthBit(i + inInt));
		}
		for (auto i = totalBits - inInt; i < totalBits; ++i) {
			solution.setTotalNthBit(i, false);
		}

		return solution;
	}

}

BigInt BigInt::operator&(const BigInt& inInt) const {
	BigInt solution{ *this };
	(m_sign != inInt.m_sign) ? solution.m_sign = false : solution.m_sign = true;
	solution.matchVectorSize(inInt);
	//Account for possible differing vector sizes.
	std::size_t smallerSize{ inInt.m_bits.size() };
	std::size_t biggerSize{ m_bits.size() };
	if (m_bits.size() < inInt.m_bits.size()) {
		smallerSize = m_bits.size();
		biggerSize = inInt.m_bits.size();
	}
	for (auto i = 0; i < smallerSize; ++i) {
		solution.m_bits[i] &= inInt.m_bits[i];
	}
	//And set any leading terms to 0, as anything & 0 = 0.
	for (auto i = smallerSize; i < biggerSize; ++i) {
		solution.m_bits[i] = 0;
	}
	solution.trimLeadingZeroes();
	return solution;
}

BigInt BigInt::operator|(const BigInt& inInt) const {
	BigInt solution{ *this };
	(m_sign != inInt.m_sign) ? solution.m_sign = false : solution.m_sign = true;
	solution.matchVectorSize(inInt);
	//Account for possible differing vector sizes.
	std::size_t smallerSize{ inInt.m_bits.size() };
	std::size_t biggerSize{ m_bits.size() };
	bool thisBigger{ true };
	if (m_bits.size() < inInt.m_bits.size()) {
		smallerSize = m_bits.size();
		biggerSize = inInt.m_bits.size();
		thisBigger = false;
	}
	for (auto i = 0; i < smallerSize; ++i) {
		solution.m_bits[i] |= inInt.m_bits[i];
	}
	//We have two possibilities. 
	// this > inInt means that Solution also contains all leading terms and using operator| is equivalent to adding 0.
	// The reverse means that there are still terms left to add. Since in this case leading solution terms are set to 0, we can add without risk of overflow.
	if (!thisBigger) {
		for (auto i = smallerSize; i < biggerSize; ++i) {
			solution.m_bits[i] += inInt.m_bits[i];
		}
	}
	solution.trimLeadingZeroes();
	return solution;
}

BigInt BigInt::operator^(const BigInt& inInt) const {
	BigInt solution{ *this };
	(m_sign != inInt.m_sign) ? solution.m_sign = false : solution.m_sign = true;
	solution.matchVectorSize(inInt);
	//Account for possible differing vector sizes.
	std::size_t smallerSize{ inInt.m_bits.size() };
	std::size_t biggerSize{ m_bits.size() };
	bool thisBigger{ true };
	if (m_bits.size() < inInt.m_bits.size()) {
		smallerSize = m_bits.size();
		biggerSize = inInt.m_bits.size();
		thisBigger = false;
	}
	for (auto i = 0; i < smallerSize; ++i) {
		solution.m_bits[i] ^= inInt.m_bits[i];
	}
	//Again if this < inInt we need to plug solution with (inInt ^ 0) terms
	if (!thisBigger) {
		for (auto i = smallerSize; i < biggerSize; ++i) {
			solution.m_bits[i] = inInt.m_bits[i] ^ static_cast<arrayType>(0);
		}
	}
	//Otherwise solution already contains the pertinent data and we just need to ^0 it.
	else {
		for (auto i = smallerSize; i < biggerSize; ++i) {
			solution.m_bits[i] ^= static_cast<arrayType>(0);
		}
	}
	solution.trimLeadingZeroes();
	return solution;
}

BigInt BigInt::operator~() const {
	BigInt solution{ *this };
	for (auto i = 0; i < solution.m_bits.size(); ++i) {
		solution.m_bits[i] = ~(solution.m_bits[i]);
	}
	solution.trimLeadingZeroes();
	return solution;
}

/*
* ASSIGNMENT OPERATORS
*/
BigInt& BigInt::operator+=(const BigInt& inInt) {
	*this = *this + inInt;
	return *this;
}

BigInt& BigInt::operator-=(const BigInt& inInt) {
	*this = *this - inInt;
	return *this;
}

BigInt& BigInt::operator*=(const BigInt& inInt) {
	*this = *this * inInt;
	return *this;
}

BigInt& BigInt::operator/=(const BigInt& inInt) {
	*this = *this / inInt;
	return *this;
}

BigInt& BigInt::operator%=(const BigInt& inInt) {
	*this = *this % inInt;
	return *this;
}

BigInt& BigInt::operator<<=(arrayType inInt) {
	*this = *this << inInt;
	return *this;
}

BigInt& BigInt::operator>>=(arrayType inInt) {
	*this = *this >> inInt;
	return *this;
}

BigInt& BigInt::operator&=(const BigInt& inInt) {
	*this = *this & inInt;
	return *this;
}

BigInt& BigInt::operator|=(const BigInt& inInt) {
	*this = *this | inInt;
	return *this;
}

BigInt& BigInt::operator^=(const BigInt& inInt) {
	*this = *this ^ inInt;
	return *this;
}


/*
* MISC FUNCTIONALITY
*/
bool BigInt::sign() const {
	return m_sign;
}

BigInt BigInt::abs() const {
	BigInt absValue{ *this };
	absValue.m_sign = true;
	return absValue;
}

BigInt::operator arrayType() const {
	//We make the choice to let typecasts to values higher than we can contain be UB and simply trust the user to use this code correctly.
	return m_bits[0];
}

std::string BigInt::toString(int base) const {
	switch (base) {
	case 2:
		return this->getBinaryString();

		//More bases to be added as needed.
	default:
		return this->getDecimalString();
	}
}

