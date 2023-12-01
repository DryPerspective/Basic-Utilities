#ifndef PHYSICSVECTOR_H
#define PHYSICSVECTOR_H


/*
* As the name suggests, the PhysicsVector class is used to hold a vector (the mathematical object of a number with direction) and various functions to manipulate it.
* Using the name PhysicsVector rather than Vector to try reduce confusion with the std::vector object included as part of the C++ standard library and used for dynamically sized arrays.
* This class should work for vectors of any dimension, but has more focus on 2D and 3D vectors since they are the overwhelming majority of use cases (plus after dimension 4 there isn't easy letter representation).
*
* The optimization mimicks string SBO - namely for the very small and common use-cases, data is stored on the stack. For less common and larger cases, data is kept on the heap. This is a balance
* - while an individual PhysicsVector object, even for a larger number of dimensions, will be pretty small; it is expected that there may be a great many of them in memory at any given time for
* most common uses such as simulations.
*
* As several vector functions are mathematical nonsense when applied to vectors of differing sizes, this object is templated with each new object being a vector of a particular size.
* This gives compiler-level enforcement of such mathematical rules, and allows much error checking to be shifted to compile time.
*/


#include <vector>
#include <array>
#include <initializer_list>
#include <iostream>	
#include <cmath>	
#include <numeric>		//For the inner product
#include <limits>		//For epsilon function in getUnitVector
#include <regex>		//Used to analyse strings - regex not used elsewhere
#include <charconv>		//Here's hoping your compiler supports floating point conversions
#include <type_traits>
#include <algorithm>
#include <functional>  




namespace dp {


	template <std::size_t dim>
	class PhysicsVector
	{

		static_assert(dim > 0, "PhysicsVector dimension must be a positive number");

	private:

		//The maximum number of dimensions a PhysicsVector should have before we start heap-allocating them.
		static constexpr inline std::size_t MaxStackDims = 3;

		//Only one key set of internal data - the list of components.
		//This is kept protected for two reasons - it is not public because we don't want external meddling with this to attempt to transform the vector components directly (particularly for the n-dimensional case
		//where the std::vector of components could be resized to mismatch with the size of the PhysicsVector it represents. Secondly it maintains the interface between data and functionality.

		std::conditional_t<dim <= MaxStackDims, std::array<double, dim>, std::vector<double>> m_components;



		//----------------PHYSICSVECTOR FUNCTIONS--------------------------	



		auto print(std::ostream& out) const -> std::ostream& {
			out << "(";
			//In this instance, a raw range-for seemed like the more pleasant solution than needing a std::for_each, specifying a range, and constructing a lambda
			for (auto elem : m_components) {
				out << elem << ',';
			}
			out << '\b' << ")";
			return out;
		}
		
		//An internal "at" function (note, no bounds checking), used to cut down on code duplication by providing an underlying common function
		//If we were in C++23 this would definitely be a public deducing this function
		template <typename Self>
		static auto&& int_at(Self&& self, const std::size_t index) {
			return self.m_components[index];
		}


	public:
		/*
		* Constructors.
		*/
		explicit constexpr PhysicsVector() {
			if constexpr (dim <= MaxStackDims) {
				m_components.fill(0);
			}
			else {
				m_components.assign(dim, 0);
			}
		}

		//Copy-constructor to clone another PhysicsVector.
		constexpr PhysicsVector(const PhysicsVector<dim>&) = default;

		//Move constructor
		constexpr PhysicsVector(PhysicsVector<dim>&&) noexcept = default;

		//Copy Assignment.
		constexpr PhysicsVector<dim>& operator=(const PhysicsVector<dim>&) = default;
		//Move assignment.
		constexpr PhysicsVector<dim>& operator=(PhysicsVector<dim>&&) noexcept = default;


		constexpr PhysicsVector(std::initializer_list<double> inList) : PhysicsVector<dim>() {
			std::copy(inList.begin(), inList.end(), m_components.begin());
		}

		~PhysicsVector() noexcept = default;



		/*
		* Setters and getters. The choice to keep the raw component std::vector private is intentional to prevent attempts to resize it and cause an inconsistent state
		* Also, since X, Y, and Z are so universally known as labels when dealing with vectors, I include specific functions for those elements.
		* This should help with readability in use cases where we are clearly working with a particular dimension - getX() is immediately more recognisible than getAt(0).
		*/


		constexpr decltype(auto) at(const std::size_t inEntry) const {
			return m_components.at(inEntry);
		}

		constexpr decltype(auto) at(const std::size_t inEntry) {
			return m_components.at(inEntry);
		}

		//And accessors for named dimensions.
		//What I wouldn't do for deducing this...
		constexpr decltype(auto) x() const {
			return int_at(*this, 0);
		}
		constexpr decltype(auto) y() const {
			return int_at(*this, 1);
		}
		constexpr decltype(auto) z() const {
			return int_at(*this, 2);
		}

		constexpr decltype(auto) x() {
			return int_at(*this, 0);
		}
		constexpr decltype(auto) y() {
			return int_at(*this, 1);
		}
		constexpr decltype(auto) z() {
			return int_at(*this, 2);
		}

		constexpr auto dimension() const -> std::size_t {
			return dim;
		}

		//A general setter. 
		constexpr auto setAt(std::size_t inIndex, double inValue) {
			int_at(*this, inIndex) = inValue;
		}

		//Operator[] to round out accessing the data. Mirroring the std::vector, operator[] does no bounds checking.
		constexpr decltype(auto) operator[](std::size_t index) {
			return int_at(*this, index);
		}
		//Operator[] with const-ness.
		constexpr decltype(auto) operator[](std::size_t index) const {
			return int_at(*this, index);
		}

		constexpr auto swap(PhysicsVector<dim>& inVector) noexcept {
			m_components.swap(inVector.m_components);
		}


		/*
		* Operator overloads
		*/
		// Clunky friend declarations are clunky, but I think free functions work better than members in this case, particularly as it allows for a
		// neater implementation for some of them.
		// But, the snappiest implementation requires access to m_components, so we need to be friends.
		template<std::size_t N>
		friend constexpr auto operator==(const PhysicsVector<N>&, const PhysicsVector<N>&) -> bool;

		template<std::size_t N>
		friend constexpr auto operator!=(const PhysicsVector<N>&, const PhysicsVector<N>&) -> bool;

		template<std::size_t N>
		friend constexpr auto operator-(PhysicsVector<N> inVector)->PhysicsVector<N>;

		template<std::size_t N>
		friend constexpr auto operator+(PhysicsVector<N> lhsVector, const PhysicsVector<N>& rhsVector)->PhysicsVector<N>;

		template<std::size_t N>
		friend constexpr auto operator-(PhysicsVector<N> lhsVector, const PhysicsVector<N>& rhsVector)->PhysicsVector<N>;

		template<std::size_t N>
		friend constexpr auto operator+=(PhysicsVector<N>& lhsVector, const PhysicsVector<N>& rhsVector)->PhysicsVector<N>&;

		template<std::size_t N>
		friend constexpr auto operator-=(PhysicsVector<N>& lhsVector, const PhysicsVector<N>& rhsVector)->PhysicsVector<N>&;

		//As above, operator<< calls the print() function.
		template<std::size_t N>
		friend constexpr auto operator<<(std::ostream& out, const PhysicsVector<N>& inVector)->std::ostream&;


		/*
		* Vector calculus functions.
		* These are what separates the PhysicsVector object from just being a generic wrapper around a container of numbers.
		*/
		//First the length squared - kept in a separate function to prevent unnecessary square rooting and then squaring back up again.
		constexpr auto lengthSquared() const -> double {
			return std::accumulate(m_components.cbegin(), m_components.cend(), 0.0, [](double acc, double val) { return acc + std::pow(val, 2); });
		}
		//Then the actual length of the vector.
		constexpr auto length() const -> double {
			return std::sqrt(this->lengthSquared());		//No need for checking for negatives since the squares of the components should always be positive (or 0)
		}
		//And an alias of length since the underlying property is also known universally by that name.
		constexpr auto magnitude() const -> double {
			return this->length();
		}
		//The inner product, or specifically the dot product for this kind of vector space. Fortunately the standard library already includes this functionality.
		constexpr auto innerProduct(const PhysicsVector<dim>& inVector) const -> double {
			return std::inner_product(m_components.cbegin(), m_components.cend(), inVector.m_components.cbegin(), 0.0);
		}

		//The vector product. Again, this one has to return by value since an evaluation of (A x B) doesn't change the value of A or B.
		//Unfortunately, the vector product is only well-defined for 3-dimensional vectors, though a version does exist for seven dimensions.
		constexpr auto vectorProduct(const PhysicsVector<dim>& inVector) const -> PhysicsVector<dim> {
			static_assert(dim == 3 || dim == 7, "Error: Vector Product only defined for 3- and 7- dimensional vectors.");
			if constexpr (dim == 3) {																									//Because this can't be generalised to n dimensions, have to just compute the specific cases.
				double newX{ (this->m_components[1] * inVector.m_components[2]) - (this->m_components[2] * inVector.m_components[1]) };
				double newY{ (this->m_components[2] * inVector.m_components[0]) - (this->m_components[0] * inVector.m_components[2]) };		//Note the negative sign is factored in, i.e. -(a1b3-a3b1) = (a3b1-a1b3)
				double newZ{ (this->m_components[0] * inVector.m_components[1]) - (this->m_components[1] * inVector.m_components[0]) };
				return PhysicsVector<dim>{ newX, newY, newZ };
			}
			else {
				//There are seven components to the vector (e1-e7) each with 3 terms to calculate.
				double e1{ 0.0 };																											//I've split it up to one term per line for easier readability.
				e1 += ((this->m_components[1] * inVector.m_components[3]) - (this->m_components[3] * inVector.m_components[1]));	//e1 term 1
				e1 += ((this->m_components[2] * inVector.m_components[6]) - (this->m_components[6] * inVector.m_components[2]));	//e1 term 2
				e1 += ((this->m_components[4] * inVector.m_components[5]) - (this->m_components[5] * inVector.m_components[4]));	//e1 term 3
				double e2{ 0.0 };
				e2 += ((this->m_components[2] * inVector.m_components[4]) - (this->m_components[4] * inVector.m_components[2]));	//e2 term 1
				e2 += ((this->m_components[3] * inVector.m_components[0]) - (this->m_components[0] * inVector.m_components[3]));	//e2 term 2
				e2 += ((this->m_components[5] * inVector.m_components[6]) - (this->m_components[6] * inVector.m_components[5]));	//e2 term 3
				double e3{ 0.0 };
				e3 += ((this->m_components[3] * inVector.m_components[5]) - (this->m_components[5] * inVector.m_components[3]));	//e3 term 1
				e3 += ((this->m_components[4] * inVector.m_components[1]) - (this->m_components[1] * inVector.m_components[4]));	//e3 term 2
				e3 += ((this->m_components[6] * inVector.m_components[0]) - (this->m_components[0] * inVector.m_components[6]));	//e3 term 3
				double e4{ 0.0 };
				e4 += ((this->m_components[4] * inVector.m_components[6]) - (this->m_components[6] * inVector.m_components[4]));	//e4 term 1
				e4 += ((this->m_components[5] * inVector.m_components[2]) - (this->m_components[2] * inVector.m_components[5]));	//e4 term 2
				e4 += ((this->m_components[0] * inVector.m_components[1]) - (this->m_components[1] * inVector.m_components[0]));	//e4 term 3
				double e5{ 0.0 };
				e5 += ((this->m_components[5] * inVector.m_components[0]) - (this->m_components[0] * inVector.m_components[5]));	//e5 term 1
				e5 += ((this->m_components[6] * inVector.m_components[3]) - (this->m_components[3] * inVector.m_components[6]));	//e5 term 2
				e5 += ((this->m_components[1] * inVector.m_components[2]) - (this->m_components[2] * inVector.m_components[1]));	//e5 term 3
				double e6{ 0.0 };
				e6 += ((this->m_components[6] * inVector.m_components[1]) - (this->m_components[1] * inVector.m_components[6]));	//e6 term 1
				e6 += ((this->m_components[0] * inVector.m_components[4]) - (this->m_components[4] * inVector.m_components[0]));	//e6 term 2
				e6 += ((this->m_components[2] * inVector.m_components[3]) - (this->m_components[3] * inVector.m_components[2]));	//e6 term 3
				double e7{ 0.0 };
				e7 += ((this->m_components[0] * inVector.m_components[2]) - (this->m_components[2] * inVector.m_components[0]));	//e7 term 1
				e7 += ((this->m_components[1] * inVector.m_components[5]) - (this->m_components[5] * inVector.m_components[1]));	//e7 term 2
				e7 += ((this->m_components[3] * inVector.m_components[4]) - (this->m_components[4] * inVector.m_components[3]));	//e7 term 3
				return PhysicsVector<dim>{e1, e2, e3, e4, e5, e6, e7};
			}
		}
		//A function to scale a vector in place.
		constexpr auto scaleVector(const double inValue) -> PhysicsVector<dim>& {
			for (auto& elem : m_components) {
				elem *= inValue;
			}
			return *this;
		}
		//A function to scale a vector without transforming the original vector. As such it must return by value.
		constexpr auto scaledBy(const double inValue) const -> PhysicsVector<dim> {
			PhysicsVector<dim> newVector{ *this };
			return newVector.scaleVector(inValue);
		}
		//Get the unit vector equivalent of a particular vector, given by V/|V|
		constexpr auto getUnitVector() const -> PhysicsVector<dim> {
			if (this->magnitude() <= std::numeric_limits<double>::epsilon()) return PhysicsVector<dim>();	//Solve the problem of dividing by zero. NB: Default constructor will return a correctly sized vector filled with 0.
			PhysicsVector<dim> unitVector{ *this };															//Otherwise create a new vector and scale it accordingly.
			unitVector.scaleVector(1 / unitVector.magnitude());
			return unitVector;
		}

		//Static vector calculus functions. Sometimes it makes more sense in code to do innerProduct(Vec1,Vec2) than Vec1.innerProduct(Vec2).
		static constexpr auto innerProduct(const PhysicsVector<dim>& inVector1, const PhysicsVector<dim>& inVector2) -> double {
			return inVector1.innerProduct(inVector2);
		}
		static constexpr auto vectorProduct(const PhysicsVector<dim>& inVector1, const PhysicsVector<dim>& inVector2) -> PhysicsVector<dim> {
			return inVector1.vectorProduct(inVector2);
		}




	};

	//Deduction guide
	template<typename T, typename... U>
	PhysicsVector(T, U...) -> PhysicsVector<1 + sizeof...(U)>;


	//*Operator overloads*//
	//A vector is only equal if all of their respective components match.
	template<std::size_t dim>
	constexpr auto operator==(const PhysicsVector<dim>& lhsVector, const PhysicsVector<dim>& rhsVector) -> bool {
		if (&lhsVector == &rhsVector)return true;
		return lhsVector.m_components == rhsVector.m_components;
	}
	template<std::size_t dim>
	constexpr auto operator!=(const PhysicsVector<dim>& lhsVector, const PhysicsVector<dim>& rhsVector) -> bool {
		return !(lhsVector == rhsVector);
	}

	//NB - pass by value as we need to make a copy anyway
	template<std::size_t dim>
	constexpr auto operator-(PhysicsVector<dim> inVector) -> PhysicsVector<dim> {
		std::transform(inVector.m_components.cbegin(), inVector.m_components.cend(), inVector.m_components.begin(), std::negate<double>{});
		return inVector;
	}

	template<std::size_t dim>
	constexpr auto operator+(PhysicsVector<dim> lhsVector, const PhysicsVector<dim>& rhsVector) -> PhysicsVector<dim> {
		return lhsVector += rhsVector;
	}

	template<std::size_t dim>
	constexpr auto operator-(PhysicsVector<dim> lhsVector, const PhysicsVector<dim>& rhsVector) ->PhysicsVector<dim> {
		return lhsVector -= rhsVector;
	}

	template<std::size_t dim>
	constexpr auto operator+=(PhysicsVector<dim>& lhsVector, const PhysicsVector<dim>& rhsVector) -> PhysicsVector<dim>& {
		std::transform(lhsVector.m_components.cbegin(), lhsVector.m_components.cend(), rhsVector.m_components.cbegin(), lhsVector.m_components.begin(), std::plus<double>{});
		return lhsVector;
	}

	template<std::size_t dim>
	constexpr auto operator-=(PhysicsVector<dim>& lhsVector, const PhysicsVector<dim>& rhsVector) -> PhysicsVector<dim>& {
		std::transform(lhsVector.m_components.cbegin(), lhsVector.m_components.cend(), rhsVector.m_components.cbegin(), lhsVector.m_components.begin(), std::minus<double>{});
		return lhsVector;
	}

	//As above, operator<< calls the print() function.
	template<std::size_t dim>
	constexpr auto operator<<(std::ostream& out, const PhysicsVector<dim>& inVector) -> std::ostream& {
		return inVector.print(out);
	}



	/*
	* A quick trait to identify specialisations of the template
	* Since this is so absolutely exclusive to PhysicsVector it belongs here rather than in the general traits library
	*/
	template <typename T>
	struct is_PhysicsVector : std::false_type {};

	template <std::size_t N>
	struct is_PhysicsVector<dp::PhysicsVector<N>> : std::true_type {};

	template<typename T>
	constexpr inline bool is_PhysicsVector_v{ is_PhysicsVector<T>::value };


	template<std::size_t dim>
	constexpr auto swap(PhysicsVector<dim>& lhs, PhysicsVector<dim>& rhs) noexcept {
		lhs.swap(rhs);
	}


}



#endif