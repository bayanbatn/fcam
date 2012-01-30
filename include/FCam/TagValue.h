#ifndef FCAM_TAGVALUE_H
#define FCAM_TAGVALUE_H

#include <string>
#include <vector>

#include "Time.h"
#include "Event.h"

/** \file 
 * Class for the values a frame can be tagged with. */

namespace FCam {

    /** The values with which a Device can tag a Frame */
    class TagValue {

    public:
        /** A TagValue can represent one of a number of types listed
         * below. */
        enum Type {
            Null = 0, //!< A Null TagValue represents nothing, and indicates no such tag exists.
            Int,      //!< A 32-bit signed integer
            Float,    //!< A 32-bit float
            Double,   //!< A 64-bit double
            String,   //!< An std::string. Also useful to represent a blob of binary data.
            Time,     //!< A \ref FCam::Time "Time"
            IntVector,    //!< A std::vector of ints
            FloatVector,  //!< A std::vector of floats
            DoubleVector, //!< A std::vector of doubles
            StringVector, //!< A std::vector of std::strings
            TimeVector    //!< A std::vector of \ref FCam::Time "Times"
        };

        ~TagValue();

        /** @name Constructors
         * 
         * TagValues can be constructed from many different types. The
         * default constructor makes a \ref Null TagValue. Each other
         * constructor makes a copy of its argument. */
        //@{

        TagValue(); 
        TagValue(int);
        TagValue(float);
        TagValue(double);
        TagValue(std::string);
        TagValue(FCam::Time);
        TagValue(std::vector<int>);
        TagValue(std::vector<float>);
        TagValue(std::vector<double>);
        TagValue(std::vector<std::string>);
        TagValue(std::vector<FCam::Time>);
        TagValue(const TagValue &);
        //@}

        /** @name Assignment
         * 
         * You assign to a TagValue from any of the \ref Type "types"
         * a TagValue supports. This sets the TagValue to a copy of
         * the argument. */
        //@{
        const TagValue &operator=(const int &);
        const TagValue &operator=(const float &);
        const TagValue &operator=(const double &);
        const TagValue &operator=(const std::string &);
        const TagValue &operator=(const FCam::Time &);
        const TagValue &operator=(const std::vector<int> &);
        const TagValue &operator=(const std::vector<float> &);
        const TagValue &operator=(const std::vector<double> &);
        const TagValue &operator=(const std::vector<std::string> &);
        const TagValue &operator=(const std::vector<FCam::Time> &);
        const TagValue &operator=(const TagValue &);
        //@}
        
        /** @name Cast Operators
         * 
         * A TagValue can be implicitly cast to the appropriate
         * type. Trying to cast a TagValue to the wrong type will
         * result in a default value returned and a BadCast error
         * placed on the event queue. If you wish to inspect or modify
         * a large tag value in-place (e.g. an IntVector), cast it to
         * a reference to the appropriate type, rather than a concrete
         * instance of that type. The latter will make a fresh copy of
         * the potentially large data. For example: \code
Frame frame = Sensor.getFrame();

// This makes a copy of a potentially large array
std::vector<int> bigTag = frame["bigTag"];

// This does not, and so is usually what you want to do.
std::vector<int> &bigTagRef = frame["bigTag"];
\endcode
        */
        //@{
        operator int &() const;
        operator float &() const;
        operator double &() const;
        operator std::string &() const;
        operator FCam::Time &() const;
        operator std::vector<int> &() const; 
        operator std::vector<float> &() const;
        operator std::vector<double> &() const;
        operator std::vector<std::string> &() const;
        operator std::vector<FCam::Time> &() const;
        //@}

        /** @name Cast methods
         *
         * A TagValue also has explicit methods to cast it to various
         * types. These have the same semantics as the implicit casts,
         * but are more readable in many cases. For example: \code
TagValue tag = std::vector<int>();

// Use the overloaded cast operator
((std::vector<int> &)tag).push_back(3);

// Do the same thing using the cast method
tag.asIntVector().push_back(3);
\endcode
        */
        //@{
       
        int &asInt() {return *this;}
        float &asFloat() {return *this;}
        double &asDouble() {return *this;}
        std::string &asString() {return *this;}
        FCam::Time &asTime() {return *this;}
        std::vector<int> &asIntVector() {return *this;}
        std::vector<float> &asFloatVector() {return *this;}
        std::vector<double> &asDoubleVector() {return *this;}
        std::vector<std::string> &asStringVector() {return *this;}
        std::vector<FCam::Time> &asTimeVector() {return *this;}

        //@}
 

        /** Does this tag exist? That is, is type equal to Null. */
        bool valid() { return type != Null; }

        /** Serialize into a human-and-python readable format */
        std::string toString() const;

        /** Serialize into a more efficient binary format. For tags
         * containing large arrays of data, this format can be over
         * 100x more time-efficient than the human readable format,
         * and about 2.5x more space efficient. */
        std::string toBlob() const;

        /** Deserialize from either format */
        static TagValue fromString(const std::string &);

        /** The type of this tag. */
        Type type;

        /** A pointer to the actual value of this tag. */
        void *data;

    private:
        void nullify();


        // Dummy objects to return references to. Set them to zero or
        // clear them before returning a reference to them.
        static int dummyInt;
        static float dummyFloat;
        static double dummyDouble;
        static std::string dummyString;
        static FCam::Time dummyTime;
        static std::vector<int> dummyIntVector;
        static std::vector<float> dummyFloatVector;
        static std::vector<double> dummyDoubleVector;
        static std::vector<std::string> dummyStringVector;
        static std::vector<FCam::Time> dummyTimeVector;

    };

    /** Serialize the TagValue into a human readable format. 
     * \relates TagValue */
    std::ostream & operator<<(std::ostream& out, const TagValue &t);

    /** Deserialize a TagValue from either the human-readable or the
     * binary format. \relates TagValue  */
    std::istream & operator>>(std::istream& in, TagValue &t);

}
    
#endif
