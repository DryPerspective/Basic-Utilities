#ifndef PHYSICSVECTOR_H
#define PHYSICSVECTOR_H
#pragma once

/*
* As the name suggests, the PhysicsVector class is used to hold a vector (the mathematical object of a number with direction) and various functions to manipulate it.
* Using the name PhysicsVector rather than Vector to try reduce confusion with the std::vector object included as part of the C++ standard library and used for dynamically sized arrays.
* This class should work for vectors of any dimension, but has more focus on 2D and 3D vectors since they are the overwhelming majority of use cases (plus after dimension 4 there isn't easy letter representation).
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



namespace Physics{


	template <std::size_t dim>
	class PhysicsVector
	{
	private:

		//Only one key set of internal data - the list of components.
		//The class is structured such that m_components[0] -> X, m_components[1] -> Y, etc.
		//The vector itself is kept private to prevent it being resized. The elements themselves have public accessors.
		std::vector<double> m_components{};		//The components of the vector.

		//Printing uses this virtual function which is called by operator<<. This allows any derived classes to easily print differently.
		//This function is intended to only be called internally so is kept private.
		virtual std::ostream& print(std::ostream& out) const {
			out << "(";
			for (double d : m_components) {
				out << d << ",";
			}
			out << '\b' << ")";
			return out;
		}

		//There are a handful of cases where a the m_components array may be a size mismatch with the amount of dimensions the vector is meant to be. Initialiser list construction for example.
		//There are several solutions to this, such as asserts and exception throwing. However in my opinion those are runtime solutions to what should be a compile-time problem.
		//Instead this function will be triggered every time there is a possibility of a mismatch and will either trim the vector down to size or pad it with 0s until it matches.
		//Kept private as this should only be used to solve internal problems - it should be impossible to get a mismatched vector size externally.
		void matchVectorSize() {
			//In the case where we have too many entries we cut the vector down
			if (m_components.size() > dim) {
				for (unsigned int i = m_components.size(); i > dim; --i) {	//Unsigned int in this case as both comparison values are of unsigned type size_t
					m_components.pop_back();
				}
			}
			//And in the case we have too few, we pad it with 0s.
			else if (m_components.size() < dim) {
				for (unsigned int i = m_components.size(); i < dim; ++i) {
					m_components.push_back(0);
				}
			}
		}


	public:
		/*
		* Constructors.
		*/
		PhysicsVector() {
			for (int i = 0; i < dim; ++i) {	//Fully initialise our components array to prevent potential out-of-bounds errors which may come from trying to set an uninitialised (but otherwise valid) component.
				m_components.push_back(0);
			}
		}
		//Copy-constructor to clone another PhysicsVector
		PhysicsVector(const PhysicsVector<dim>& inVector) {
			for (double d : inVector.m_components) {
				m_components.push_back(d);
			}
		}
		//Move constructor
		PhysicsVector(PhysicsVector<dim>&& inVector) noexcept {
			m_components = std::move(inVector.m_components);
		}
		//Initialiser list constructor.
		//We need to enforce that the initialiser list has the same number of elements as the vector it's trying to create.
		PhysicsVector(const std::initializer_list<double>& inList) {
			for (double d : inList) {
				m_components.push_back(d);
			}
			if (inList.size() != dim)this->matchVectorSize();
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
		double getAt(int inEntry) const {
			return m_components.at(inEntry);
		}
		//Alias to mirror the underlying std::vector object.
		double at(int inEntry)const {
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
		void setAt(int indexIn, double valueIn) {
			if (indexIn > dim - 1)throw std::out_of_range("Attempt to set value out of range of vector");	//dim-1 because of the zero-indexing of the vector vs dimension numbering starting at 1.
			m_components[indexIn] = valueIn;
		}
		void setX(double XIn) {
			setAt(0, XIn);
		}
		void setY(double YIn) {
			setAt(1, YIn);
		}
		void setZ(double ZIn) {
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
			for (double& d : outVector.m_components) {										//Use references in this loop as we want to actually change the values of the doubles.
				if (d != 0) {																//If clause to prevent 0 -> -0, since -0 makes no sense.
					d *= -1;																//Don't otherwise need to account for near-zero approximations. epsilon -> -epsilon is expected.
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
		double operator[](const int index) const {
			return m_components[index];
		}
		//As above, operator<< calls the print() function.
		friend std::ostream& operator<<(std::ostream& out, const PhysicsVector<dim>& inVector) {
			return inVector.print(out);
		}
		//Copy Assignment.
		PhysicsVector<dim>& operator=(const PhysicsVector<dim>& inVector) {
			if (this == &inVector)return *this;											//Check for self-assignment and return early in that case.
			m_components.clear();														//Clear our current vector...		
			for (int i = 0; i < dim; ++i) {
				m_components.push_back(inVector.m_components[i]);						//...And assign it all the values of the inVector.
			}
			if (m_components.size() != dim)this->matchVectorSize();						//Ensure size matching.

			return *this;																//Explicitly return *this so operation can be chained if needed.
		}
		//Move assignment.
		PhysicsVector<dim>& operator=(PhysicsVector<dim>&& inVector) noexcept {
			if (this == &inVector)return *this;											//Check for self-assignment
			m_components = std::move(inVector.m_components);							//Move-assign our vector
			if (m_components.size() != dim)this->matchVectorSize();						//And ensure size-matching.
			return *this;
		}
		PhysicsVector<dim>& operator+=(const PhysicsVector<dim>& inVector) {
			for (int i = 0; i < dim; ++i) {
				m_components[i] += inVector.m_components[i];
			}
			return *this;
		}
		//As with binary subtraction, the -= operator is almost identical to its addition counterpart.
		PhysicsVector<dim>& operator-=(const PhysicsVector& inVector) {
			for (int i = 0; i < dim; ++i) {
				m_components[i] -= inVector.m_components[i];
			}
			return *this;
		}

		/*
		* Vector calculus functions.
		* These are what separates the PhysicsVector object from just being a generic wrapper around an array of numbers.
		*/
		//First the length squared - kept in a separate function to prevent unnecessary square rooting and then squaring back up again.
		double lengthSquared() const {
			double outValue{ 0.0 };
			for (double d : m_components) {
				outValue += pow(d, 2);
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
			return static_cast<double>(std::inner_product(m_components.begin(), m_components.end(), inVector.m_components.begin(), 0.0));	//Static cast potentially unnecessary as it should match type 0.0.
		}
		//The vector product. Again, this one has to return by value since an evaluation of (A x B) doesn't change the value of A or B.
		//Unfortunately, the vector product is only well-defined for 3-dimensional vectors, though a version does exist for seven dimensions.
		PhysicsVector<dim> vectorProduct(const PhysicsVector<dim>& inVector) const {
			if (dim != 3 && dim != 7) throw std::logic_error("Vector Product only defined for 3- and 7- dimensional vectors.");
			if (dim == 3) {																								//Because this can't be generalised to any dimensions, have to just compute the specific cases.
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
			for (double& d : m_components) {
				d *= inValue;
			}
			return *this;
		}
		//A function to scale a vector without transforming the original vector. As such it must return by value.
		PhysicsVector<dim> scaledBy(const double inValue) const {
			PhysicsVector<dim> newVector{ *this };
			return newVector.scaleVector(inValue);
		}
		PhysicsVector<dim> scaledByVector(PhysicsVector<dim> inVector) {
			PhysicsVector<dim> output;
			for (int i = 0; i < dim; ++i) {
				output.m_components[i] = this->m_components[i] * inVector.m_components[i];
			}
			return output;
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

}




#endif

