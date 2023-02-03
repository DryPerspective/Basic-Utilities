#ifndef PHYSICSVECTOR_H
#define PHYSICSVECTOR_H
#pragma once

/*
* As the name suggests, the PhysicsVector class is used to hold a vector (the mathematical object of a number with direction) and various functions to manipulate it.
* Using the name PhysicsVector rather than Vector to try reduce confusion with the std::vector object included as part of the C++ standard library and used for dynamically sized arrays.
* This class should work for vectors of any dimension, but has more focus on 2D and 3D vectors since they are the overwhelming majority of use cases (plus after dimension 4 there isn't easy letter representation).
* 
* The core part of optimisation for 2D and 3D vectors is the PVData subclass - this uses template specialisations to ensure that in the case of 2D and 3D vectors, the data is stored on the stack rather than on the heap,
* meaning that there is no need for a costly retrieval from the heap. In all other dimensions except these specific use-cases, the data is stored on a vector so the components live on the heap.
*
* As several vector functions are mathematical nonsense when applied to vectors of differing sizes, this object is templated with each new object being a vector of a particular size.
* This gives compiler-level enforcement of such mathematical rules, with the only clear downside being that it is harder to resize a vector.
*/


#include <vector>
#include <initializer_list>
#include <iostream>	//For overloading operator<<
#include <cmath>	//For sqrt() and pow()
#include <numeric>	//For the inner product
#include <limits>	//For epsilon function in getUnitVector
#include <regex>	//Used to analyse strings for PhysicsVector layout
#include <charconv>



namespace Physics{

	//Forward declarations to resolve potential dependency issues.

	template<std::size_t dim>
	class PhysicsVector;

	template<std::size_t dim>
	bool readVector(std::string_view, PhysicsVector<dim>&);

	template<std::size_t dim>
	PhysicsVector<dim> readVector(std::string_view);





	template <std::size_t dim>
	class PhysicsVector
	{
	private:

		//--------------PVDATA DATA CONTAINER MEMBER DEFINITIONS--------------------------
		//PhysicsVector functions resume on line 350

		/*
		* As above, for optimisation reasons we store our member components inside the PVData class. This class has specialisations for 2 and 3 dimensions which store the member data on the stack rather than on the heap.
		* This provides a notable performance boost for the most common use-cases with the only key downside being that we can't internally use iterator-based arithmetic in the PhysicsVector class as these specialisations
		* do not provide iterator functionality.
		*/

		/*
		* Our general case - this version accounts for all dimensions, except for 2- and 3-dimensional Vectors. This uses a std::vector to store the components of the greater Vector object.
		*/
		template<std::size_t dim>
		class PVData final {
		private:
			//NB: m_components[0] -> X, m_components[1] -> Y, etc
			std::vector<double> m_components;

			//There are a handful of cases where a the m_components array may be a size mismatch with the amount of dimensions the vector is meant to be. Initialiser list construction for example.
			//There are several solutions to this, such as asserts and exception throwing. However in my opinion those are runtime solutions to what should be a compile-time problem.
			//Instead this function will be triggered every time there is a possibility of a mismatch and will either trim the vector down to size or pad it with 0s until it matches.
			//Kept private as this should only be used to solve internal problems - it should be impossible to get a mismatched vector size externally.
			void matchVectorSize() {
				//In the case where we have too many entries we cut the vector down
				if (m_components.size() > dim) {
					for (std::size_t i = m_components.size(); i > dim; --i) {
						m_components.pop_back();
					}
				}
				//And in the case we have too few, we pad it with 0s.
				else if (m_components.size() < dim) {
					for (std::size_t i = m_components.size(); i < dim; ++i) {
						m_components.push_back(0);
					}
				}
			}

		public:

			//Declare our outer PhysicsVector class so it can access the members here. Most notably during inner product calculation.
			//In general use however we try where possible to use the interface provided by this class, even with PhysicsVector.
			template<std::size_t vectorSize>
			friend class PhysicsVector;

			//Default constructor to fill the array with zeroes.
			PVData() {
				m_components.reserve(dim);
				for (std::size_t i = 0; i < dim; ++i) {
					m_components.push_back(0);
				}
			}

			//Copy constructor
			PVData(const PVData<dim>& inData) {
				m_components = inData.m_components;
				if (m_components.size() != dim)this->matchVectorSize();
			}

			//Move constructor
			PVData(const PVData<dim>&& inData) noexcept {
				m_components = std::move(inData.m_components);
				if (m_components.size() != dim) this->matchVectorSize;
			}

			//Initialiser list construction
			PVData(const std::initializer_list<double>& inList) : m_components{ inList } {
				/*
				m_components.reserve(dim);
				for (double d : inList) {
					m_components.push_back(d);
				}
				*/
				if (inList.size() != dim)this->matchVectorSize();   //As it is possible for the initialiser list to be a different size from the vector we are creating, we use this.
			}

			//Boilerplate getters and setters.
			double at(const std::size_t inIndex) const {
				return m_components.at(inIndex);
			}

			//Constant operator[] which returns by value and does not modify
			double operator[](const std::size_t inIndex) const & {
				return m_components[inIndex];
			}
			
			//Non-const which returns by reference and can modify.
			double& operator[](const std::size_t inIndex) & {
				return m_components[inIndex];
			}

			//rvalue to prevent a dangling reference
			double operator[](const std::size_t inIndex)&& {
				return m_components[inIndex];
			}
			
			void setAt(const std::size_t inIndex, const double inValue) {
				if (inIndex > dim - 1)throw std::out_of_range("Attempt to set value out of range of vector");	//dim-1 because of the zero-indexing of the vector vs dimension numbering starting at 1.
				m_components[inIndex] = inValue;
			}

			std::size_t size() const {
				return dim;
			}

			//Copy- and move-assignment operators.
			PVData<dim>& operator=(const PVData<dim>& inData) {
				m_components = inData.m_components;
				if (m_components.size() != dim)this->matchVectorSize();
				return *this;
			}

			PVData<dim>& operator=(const PVData<dim>&& inData) noexcept {
				m_components = inData.m_components;
				if (m_components.size() != dim)this->matchVectorSize();
				return *this;

			}


		};
		/*
		* The special case for 3-dimensions. As this is the most common use-case, we provide some optimisation by instead storing the three vector components on the stack.
		* On the author's machine, this actually reduces the stack size occupied by the data, so is a worthwhile optimisation which does not overpopulate the stack.
		*/
		template<>
		class PVData<3> final {
		private:
			double m_X;
			double m_Y;
			double m_Z;

		public:
			//Default constructor to zero-initialise the components.
			PVData() : m_X{ 0 }, m_Y{ 0 }, m_Z{ 0 } {};

			//Copy construction
			PVData(const PVData<3>&) = default;

			//Move construction. While this is meaningless for double member data, we must provide an implementation to preserve the interface.
			PVData(PVData<3>&&) noexcept = default;

			//Initialiser list construction
			//Initialiser lists which contain too few elements will have the rest set to 0.
			PVData(const std::initializer_list<double>& inList) : m_X{ 0 }, m_Y{ 0 }, m_Z{ 0 } { 
				//We can't iterate along the list using indices, so we first zero-initialise all elements, and the iterate using iterators.
				//We do need to introduce our own counter, but this approach allows us to directly write the elements to a vector.
				int i{ 0 };
				for (auto element : inList) {	//This type of for loop eliminates issues where the list is too short, as other elements are already 0.
					if (i == 3)break;			//Break if we go above 3 elements, eliminating risk from lists which are too long.
					(*this)[i++] = element;
				}
			}

			//Accessor with "bounds checking" (so far as this implementation allows)
			double at(const std::size_t inIndex) const {
				switch (inIndex) {
				case 0:
					return m_X;
				case 1:
					return m_Y;
				case 2:
					return m_Z;
				default:
					throw std::out_of_range("Error: Attempting to access value at a higher dimension than this PhysicsVector contains.");
				}
			}

			//Accessor without "bounds checking" and can modify
			double& operator[](const std::size_t inIndex) & {
				switch (inIndex) {
				case 0:
					return m_X;
				case 1:
					return m_Y;
				default:                    //As we are returning a reference type, we need to return a variable which exists. So we choose to let all incorrect indices return m_Z.
					return m_Z;

				}
			}

			//Accessor without "bounds checking" which preseves const-ness
			double operator[](const std::size_t inIndex) const & {
				switch (inIndex) {
				case 0:
					return m_X;
				case 1:
					return m_Y;
				case 2:                    
					return m_Z;
				default:
					return 0;			//We must return something so we return 0 for a failed attempt.

				}
			}

			//rvalue variant
			double operator[](const std::size_t inIndex) && {
				switch (inIndex) {
				case 0:
					return m_X;
				case 1:
					return m_Y;
				case 2:
					return m_Z;
				default:
					return 0;			

				}
			}

			//Setter with "bounds checking" - since operator[] returns a reference, that should be used for non-bounds-checking sets.
			void setAt(const std::size_t inIndex, const double inValue) {
				if (inIndex > 3) throw std::out_of_range("Attempt to set value out of range of vector");
				(*this)[inIndex] = inValue;
			}

			std::size_t size() const {
				return 3;
			}

			//Copy- and move- assignment. Again move assignment is meaningless in this case but we must preserve the interface used for cases where it isn't.
			PVData<3>& operator=(const PVData<3>&) = default;

			PVData<3>& operator=(PVData<3>&&) noexcept = default;


		};

		/*
		* The special case for 2-dimensional Vectors. As this is the second most likely use-case, we do the same stack-based optimisation to reduce costly retrievals from the heap.
		* Most of the functions here will heavily mirror that of a 3-dimensional PVData object.
		*/
		template<>
		class PVData<2> final {
		private:
			double m_X;
			double m_Y;

		public:
			PVData() :m_X{ 0 }, m_Y{ 0 }{};

			//Copy construction
			PVData(const PVData<2>&) = default;

			//Move construction. As with 3D it's meaningless for double member data but we must keep the interface consistent.
			PVData(PVData<2>&&) noexcept = default;

			//Initialiser list construction
			//Again we ignore issues of a list which is too long and throw exceptions for a shorter list.
			PVData(const std::initializer_list<double>& inList) : m_X{ 0 }, m_Y{ 0 } {
				int i{ 0 };
				for (auto element : inList) {	//This type of for loop eliminates issues where the list is too short, as other elements are already 0.
					if (i == 2)break;			//Break if we go above 2 elements, eliminating risk from lists which are too long.
					(*this)[i++] = element;
				}
			}

			//Accessor with "bounds checking" (so far as this implementation allows)
			double at(const std::size_t inIndex) const {
				switch (inIndex) {
				case 0:
					return m_X;
				case 1:
					return m_Y;
				default:
					throw std::out_of_range("Error: Attempting to access value at a higher dimension than this PhysicsVector contains.");
				}
			}

			//Accessor without "bounds checking" that can modify
			double& operator[](const std::size_t inIndex) & {
				switch (inIndex) {
				case 0:
					return m_X;
				default:
					return m_Y;             //Again we make the call to have invalud indices return the Y component.
				}
			}

			//Accessor without "bounds checking" that cannot modify
			double operator[](const std::size_t inIndex) const & {
				switch (inIndex) {
				case 0:
					return m_X;
				case 1:
					return m_Y;
				default:
					return 0;
				}
			}

			//rvalue accessor
			double operator[](const std::size_t inIndex) && {
				switch (inIndex) {
				case 0:
					return m_X;
				case 1:
					return m_Y;
				default:
					return 0;
				}
			}

			//Setter with "bounds checking" - since operator[] returns a reference, that should be used for non-bounds-checking sets.
			void setAt(const std::size_t inIndex, const double inValue) {
				if (inIndex > 2) throw std::out_of_range("Attempt to set value out of range of vector");
				(*this)[inIndex] = inValue;
			}

			std::size_t size() const {
				return 2;
			}

			//Copy- and move- assignment
			PVData<2>& operator=(const PVData<2>&) = default;

			PVData<2>& operator=(PVData<2>&&) noexcept = default;
			


		};



		//----------------PHYSICSVECTOR FUNCTIONS--------------------------
		 
		//Only one key set of internal data - the list of components.
		//This is kept protected for two reasons - it is not public because we don't want external meddling with this to attempt to transform the vector components directly (particularly for the n-dimensional case
		//where the std::vector of components could be resized to mismatch with the size of the PhysicsVector it represents. Secondly it maintains the interface between data and functionality.
		PVData<dim> m_components{};		//The components of the vector.

		//Printing uses this virtual function which is called by operator<<. This allows any derived classes to easily print differently.
		//This function is intended to only be called internally so is kept private.
		virtual std::ostream& print(std::ostream& out) const {
			out << "(";
			for (std::size_t i = 0; i<dim;++i) {
				out << m_components[i] << ",";
			}
			out << '\b' << ")";
			return out;
		}





	public:
		/*
		* Constructors.
		*/
		//Default constructor. Note that the default constructor for PVData will zero-initialise all component values.
		PhysicsVector() {}
		
		//Copy-constructor to clone another PhysicsVector.
		PhysicsVector(const PhysicsVector<dim>&) = default;

		//Move constructor
		PhysicsVector(PhysicsVector<dim>&&) = default;
		//Initialiser list constructor.
		//PVData should enforce that the list has the correct number of elements, so we call its initialiser list constructor.
		PhysicsVector(const std::initializer_list<double>& inList) : m_components{ inList } {}

		PhysicsVector(std::string_view inString) : PhysicsVector() {
			readVector(inString, *this);
		}
	

		//Virtual default destructor.
		virtual ~PhysicsVector() = default;



		/*
		* Setters and getters. The choice to keep the raw component std::vector private is intentional to prevent attempts to resize it and cause a mismatch between how many elements it contains and how many the functions can handle.
		* However the elements themselves should all be accessible.
		* Also, since X, Y, and Z are so universally known as labels when dealing with vectors, I include specific functions for those elements.
		* This should help with readability in use cases where we are clearly working with a particular dimension - getX() is immediately more recognisible than getAt(0).
		* Also note that operator[] is also overloaded in the operator overloads section to effect the expected result.
		*/

		//Get a general entry. We use vector::at() as it will throw an exception in the event of an attempt to access out of range entries.
		double getAt(const std::size_t inEntry) const {
			return m_components.at(inEntry);
		}
		//Alias to mirror the underlying std::vector object.
		double at(const std::size_t inEntry)const {
			return getAt(inEntry);
		}
		//And accessors for named dimensions.
		double getX() const {
			return getAt(0);
		}
		double getY() const {
			return getAt(1);
		}
		double getZ() const {
			return getAt(2);
		}
		std::size_t dimension() const {
			return dim;
		}

		//A general setter. This will throw an exception if you attempt to set a value outside the range of the vector.
		void setAt(const std::size_t inIndex, const double inValue) {
			m_components.setAt(inIndex, inValue);
		}
		void setX(const double XIn) {
			setAt(0, XIn);
		}
		void setY(const double YIn) {
			setAt(1, YIn);
		}
		void setZ(const double ZIn) {
			setAt(2, ZIn);
		}



		/*
		* Operator overloads for easy use. Some of these are return by value since you wouldn't usually expect evaluation of e.g. (x + y) to change the value of x.
		*/
		//A vector is only equal if all of their respective components match.
		bool operator==(const PhysicsVector<dim>& inVector) const {
			if (this == &inVector)return true;												//A vector is obviously equal to itself, so we can save some processing here.
			for (int i = 0; i < dim; ++i) {
				if (this->m_components[i] != inVector.m_components[i]) {					//Iterate over the values and if a single mismatch occurs, turn false.
					return false;															//If there's one mismatch we know the answer so don't need to check the rest.																							
				}
			}
			return true;																	//If we get this far we know there must be no mismatches, so the vectors are equal.
		}
		bool operator!=(const PhysicsVector<dim>& inVector) const {
			return !(*this == inVector);
		}
		//NB: Unary -, not subtraction. Also our first return-by-value. Because simply evauluating -X doesn't change the value of X.
		PhysicsVector<dim> operator-() const {
			PhysicsVector<dim> outVector{ *this };
			for (std::size_t i = 0; i < dim; ++i) {										//Use references in this loop as we want to actually change the values of the doubles.
				if (outVector.m_components[i] != 0) {									//If clause to prevent 0 -> -0, since -0 makes no sense.
					outVector.m_components[i] *= -1;									//Don't otherwise need to account for near-zero approximations. epsilon -> -epsilon is expected.
				}
			}
			return outVector;
		}
		//Binary addition.
		PhysicsVector<dim> operator+(const PhysicsVector<dim>& inVector) const {
			PhysicsVector<dim> outVector{ *this };											//Clone the current vector and add input values to it.							
			for (int i = 0; i < dim; ++i) {
				outVector.m_components[i] += inVector.m_components[i];
			}
			return outVector;
		}
		//Binary subtraction. Other than making sure you're doing (this - inVector) and not the reverse, this is nearly identical to adding.
		PhysicsVector<dim> operator-(const PhysicsVector<dim>& inVector) const {
			PhysicsVector outVector{ *this };
			for (int i = 0; i < dim; ++i) {
				outVector.m_components[i] -= inVector.m_components[i];
			}
			return outVector;
		}
		//Operator[] to round out accessing the data. Mirroring the std::vector, operator[] does no bounds checking.
		double& operator[](const std::size_t index) & {
			return m_components[index];
		}
		//Operator[] with const-ness.
		double operator[](const std::size_t inIndex) const & {
			return m_components[inIndex];
		}
		//rvalues
		double operator[](const std::size_t inIndex) && {
			return m_components[inIndex];
		}
		//As above, operator<< calls the print() function.
		friend std::ostream& operator<<(std::ostream& out, const PhysicsVector<dim>& inVector) {
			return inVector.print(out);
		}
		//Copy Assignment.
		PhysicsVector<dim>& operator=(const PhysicsVector<dim>&) = default;
		//Move assignment.
		PhysicsVector<dim>& operator=(PhysicsVector<dim>&&) noexcept = default;

		PhysicsVector<dim>& operator+=(const PhysicsVector<dim>& inVector) {
			for (std::size_t i = 0; i < dim; ++i) {
				m_components[i] += inVector.m_components[i];
			}
			return *this;
		}
		//As with binary subtraction, the -= operator is almost identical to its addition counterpart.
		PhysicsVector<dim>& operator-=(const PhysicsVector& inVector) {
			for (std::size_t i = 0; i < dim; ++i) {
				m_components[i] -= inVector.m_components[i];
			}
			return *this;
		}

		/*
		* Vector calculus functions.
		* These are what separates the PhysicsVector object from just being a generic wrapper around a container of numbers.
		*/
		//First the length squared - kept in a separate function to prevent unnecessary square rooting and then squaring back up again.
		double lengthSquared() const {
			double outValue{ 0.0 };
			for (std::size_t i = 0; i < dim; ++i) {
				outValue += pow(m_components[i], 2);
			}
			return outValue;
		}
		//Then the actual length of the vector.
		double length() const {
			return sqrt(this->lengthSquared());		//No need for checking for negatives since the squares of the components should always be positive (or 0)
		}
		//And an alias of magnitude since it is also known universally as that.
		double magnitude() const {
			return this->length();
		}
		//The inner product, or specifically the dot product for this kind of vector space. Fortunately the standard library already includes this functionality.
		double innerProduct(const PhysicsVector<dim>& inVector) const {
			//If our underlying data is stored in a STL container with iterator support (all dimensions except 2 and 3)
			if constexpr (dim > 3 || dim == 1) {
				//Static cast potentially unnecessary as it should match type 0.0.
				return static_cast<double>(std::inner_product(m_components.m_components.begin(), m_components.m_components.end(), inVector.m_components.m_components.begin(), 0.0));	
			}
			//Otherwise we have to grind it out manually.
			else {
				double product{ 0 };
				for (std::size_t i = 0; i < dim; ++i) {
					product += m_components[i] * inVector.m_components[i];
				}
				return product;
			}
		}
		//The vector product. Again, this one has to return by value since an evaluation of (A x B) doesn't change the value of A or B.
		//Unfortunately, the vector product is only well-defined for 3-dimensional vectors, though a version does exist for seven dimensions.
		PhysicsVector<dim> vectorProduct(const PhysicsVector<dim>& inVector) const {
			if constexpr (dim != 3 && dim != 7) throw std::logic_error("Error: Vector Product only defined for 3- and 7- dimensional vectors.");
			else if constexpr (dim == 3) {																									//Because this can't be generalised to n dimensions, have to just compute the specific cases.
				double newX{ (this->m_components[1] * inVector.m_components[2]) - (this->m_components[2] * inVector.m_components[1]) };
				double newY{ (this->m_components[2] * inVector.m_components[0]) - (this->m_components[0] * inVector.m_components[2]) };		//Note the negative sign is factored in, i.e. -(a1b3-a3b1) = (a3b1-a1b3)
				double newZ{ (this->m_components[0] * inVector.m_components[1]) - (this->m_components[1] * inVector.m_components[0]) };
				return PhysicsVector<dim>{ newX, newY, newZ };
			}
			else {																															//Else not strictly necessary since the if ends with a return, but worth being very clear
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
		PhysicsVector<dim>& scaleVector(const double inValue) {
			for (std::size_t i = 0; i < dim; ++i) {
				m_components[i] *= inValue;
			}
			return *this;
		}
		//A function to scale a vector without transforming the original vector. As such it must return by value.
		PhysicsVector<dim> scaledBy(const double inValue) const {
			PhysicsVector<dim> newVector{ *this };
			return newVector.scaleVector(inValue);
		} 
		//Get the unit vector equivalent of a particular vector, given by V/|V|
		PhysicsVector<dim> getUnitVector() const {
			if (this->magnitude() <= std::numeric_limits<double>::epsilon()) return PhysicsVector<dim>();	//Solve the problem of dividing by zero. NB: Default constructor will return a correctly sized vector filled with 0.
			PhysicsVector<dim> unitVector{ *this };															//Otherwise create a new vector and scale it accordingly.
			unitVector.scaleVector(1 / unitVector.magnitude());
			return unitVector;
		}
		//Static vector calculus functions. Sometimes it makes more sense in code to do innerProduct(Vec1,Vec2) than Vec1.innerProduct(Vec2).
		static double innerProduct(const PhysicsVector<dim>& inVector1, const PhysicsVector<dim>& inVector2) {
			return inVector1.innerProduct(inVector2);
		}
		static PhysicsVector<dim> vectorProduct(const PhysicsVector<dim>& inVector1, const PhysicsVector<dim>& inVector2) {
			return inVector1.vectorProduct(inVector2);
		}
	};

	/*
	* A pair of templated structs to easily determine if a templated type is a PhysicsVector without confining its dimension
	*/
	template <typename T>
	struct is_PhysicsVector : std::false_type {};

	template <std::size_t N>
	struct is_PhysicsVector<Physics::PhysicsVector<N>> : std::true_type {};

	template<typename T>
	constexpr inline bool is_PhysicsVector_v {is_PhysicsVector<T>::value};

	//Forward declarations and definitions because template
	namespace {
		//For the sake of keeping PhysicsVector self-contained, we keep a function to read the doubles from a string
		//As this function is only called internally from a function which already validates that the string has the correct layout,
		//we can be fairly confident that minimal error handling is needed.
		inline double getNumber(std::string_view input) {
			double output;
			std::from_chars(input.data(), input.data() + input.length(), output);
			return output;
		}
	}

	//This function is intended to read and construct a PhysicsVector object from a std::string
	//This version is very generalised and as such is a little heavy in performance.
	//Specific needs in specific projects can be met by providing a specialisation of this template which is specific to that project.
	//This function returns true if the vector was read in correctly, and false otherwise. In the event of an invalid call, it sets the input vector to {0,0,...0};
	template<std::size_t dim>
	bool readVector(std::string_view inputString, Physics::PhysicsVector<dim>& inVector) {
		//First, validate that the string is of the correct format (X,Y,Z)
		std::regex vectorReg{ "[\\{\\[\\(<]?([0-9]*[\\.]?[0-9]*[\\,]){0,}[0-9]+[\\.]?[0-9]*[\\}\\]\\)>]?" };
		//Optionally one of [{(<, then any amount of (0-9, optionally with a . and another [0-9] then ,), then another potentially decimal number and optionally a closing bracket
		if (!std::regex_match(std::string(inputString), vectorReg)) {
			inVector = PhysicsVector<dim>{};
			return false;
		}
		//We delimit around the comma to reach our individual numbers.
		//As we cannot easily insert our dim variable into the regex, the simplesy way to check we have the right number of dimensions is counting the number of commas
		auto numberOfCommas{ std::count(inputString.begin(), inputString.end(),',') };
		if (numberOfCommas != dim - 1) {
			inVector = PhysicsVector<dim>{};
			return false;
		}

		//If the vector is surrounded by brackets, we need to trim them off. We account for all common bracket styles.
		std::string brackets{ "{}[]()<>" };
		if (std::any_of(brackets.begin(), brackets.end(), [inputString](const char& x) {return x == inputString[0]; })) inputString.remove_prefix(1);
		if (std::any_of(brackets.begin(), brackets.end(), [inputString](const char& x) {return x == inputString[inputString.length() - 1]; })) inputString.remove_suffix(1);

		//If we get this far, we have a good degree of confidence that our vector is of the correct format and that external brackets have been trimmed.
		//All that remains is to separate out the numbers, read them, and write them to a PhysicsVector object.
		//This is trivial for 1D vectors, and in this case our entire inputString string should just be the number we want.
		if constexpr (dim == 1) {
			inVector.setAt(0, getNumber(inputString));
		}
		//Otherwise we just read every number up to each comma, and set the vector accordingly
		else {
			for (std::size_t i = 0; i < dim; ++i) {
				auto firstComma{ inputString.find_first_of(',') };
				std::string_view firstTerm{ inputString.substr(0,firstComma) };
				inputString.remove_prefix(firstComma + 1);
				inVector.setAt(i, getNumber(firstTerm));
			}
		}
		return true;
	}

	template<std::size_t dim>
	PhysicsVector<dim> readVector(std::string_view inString) {
		PhysicsVector<dim> out{};
		readVector(inString, out);
		return out;
	}


		




}




#endif

