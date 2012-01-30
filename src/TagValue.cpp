#include "FCam/TagValue.h"

#include <ctype.h>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace FCam {

    TagValue::TagValue() : type(Null), data(NULL) {

    }

    TagValue::~TagValue() {
        nullify();
    }

    void TagValue::nullify() {
        switch (type) {
        case Null:
            break;
        case Int:
            delete (int *)data;
            break;
        case Float:
            delete (float *)data;
            break;
        case Double:
            delete (double *)data;
            break;
        case String:
            delete (std::string *)data;
            break;
        case Time:
            delete (FCam::Time *)data;
            break;
        case IntVector:
            delete (std::vector<int> *)data;
            break;
        case FloatVector:
            delete (std::vector<float> *)data;
            break;
        case DoubleVector:
            delete (std::vector<double> *)data;
            break;
        case StringVector:
            delete (std::vector<std::string> *)data;
            break;
        case TimeVector:
            delete (std::vector<FCam::Time> *)data;
            break;
        }
        type = Null;
        data = NULL;
    }

    TagValue::TagValue(int x) {
        type = Int;
        int *ptr = new int;
        *ptr = x;
        data = (void *)ptr;
    }

    TagValue::TagValue(float x) {
        type = Float;
        float *ptr = new float;
        *ptr = x;
        data = (void *)ptr;
    }

    TagValue::TagValue(double x) {
        type = Double;
        double *ptr = new double;
        *ptr = x;
        data = (void *)ptr;
    }

    TagValue::TagValue(std::string x) {
        type = String;
        std::string *ptr = new std::string;
        *ptr = x;
        data = (void *)ptr;
    }

    TagValue::TagValue(FCam::Time x) {
        type = Time;
        FCam::Time *ptr = new FCam::Time;
        *ptr = x;
        data = (void *)ptr;
    }

    TagValue::TagValue(std::vector<int> x) {
        type = IntVector;
        std::vector<int> *ptr = new std::vector<int>;
        *ptr = x;
        data = (void *)ptr;
    }

    TagValue::TagValue(std::vector<float> x) {
        type = FloatVector;
        std::vector<float> *ptr = new std::vector<float>;
        *ptr = x;
        data = (void *)ptr;
    }

    TagValue::TagValue(std::vector<double> x) {
        type = DoubleVector;
        std::vector<double> *ptr = new std::vector<double>;
        *ptr = x;
        data = (void *)ptr;
    }

    TagValue::TagValue(std::vector<std::string> x) {
        type = StringVector;
        std::vector<std::string> *ptr = new std::vector<std::string>;
        *ptr = x;
        data = (void *)ptr;
    }

    TagValue::TagValue(std::vector<FCam::Time> x) {
        type = TimeVector;
        std::vector<FCam::Time> *ptr = new std::vector<FCam::Time>;
        *ptr = x;
        data = (void *)ptr;
    }

    const TagValue &TagValue::operator=(const int &x) {
        if (type == Int) {
            ((int *)data)[0] = x;
        } else {
            nullify();
            type = Int;
            int *ptr = new int;
            ptr[0] = x;
            data = (void *)ptr;
        }
        return *this;
    }

    const TagValue &TagValue::operator=(const float &x) {
        if (type == Float) {
            ((float *)data)[0] = x;
        } else {
            nullify();
            type = Float;
            float *ptr = new float;
            ptr[0] = x;
            data = (void *)ptr;
        }
        return *this;
    }

    const TagValue &TagValue::operator=(const double &x) {
        if (type == Double) {
            ((double *)data)[0] = x;
        } else {
            nullify();
            type = Double;
            double *ptr = new double;
            ptr[0] = x;
            data = (void *)ptr;
        }
        return *this;
    }

    const TagValue &TagValue::operator=(const std::string &x) {
        if (type == String) {
            ((std::string *)data)[0] = x;
        } else {
            nullify();
            type = String;
            std::string *ptr = new std::string;
            ptr[0] = x;
            data = (void *)ptr;
        }
        return *this;
    }

    const TagValue &TagValue::operator=(const FCam::Time &x) {
        if (type == Time) {
            ((FCam::Time *)data)[0] = x;
        } else {
            nullify();
            type = Time;
            FCam::Time *ptr = new FCam::Time;
            ptr[0] = x;
            data = (void *)ptr;
        }
        return *this;
    }

    const TagValue &TagValue::operator=(const std::vector<int> &x) {
        if (type == IntVector) {
            ((std::vector<int> *)data)[0] = x;
        } else {
            nullify();
            type = IntVector;
            std::vector<int> *ptr = new std::vector<int>;
            ptr[0] = x;
            data = (void *)ptr;
        }
        return *this;
    }

    const TagValue &TagValue::operator=(const std::vector<float> &x) {
        if (type == FloatVector) {
            ((std::vector<float> *)data)[0] = x;
        } else {
            nullify();
            type = FloatVector;
            std::vector<float> *ptr = new std::vector<float>;
            ptr[0] = x;
            data = (void *)ptr;
        }
        return *this;
    }

    const TagValue &TagValue::operator=(const std::vector<double> &x) {
        if (type == DoubleVector) {
            ((std::vector<double> *)data)[0] = x;
        } else {
            nullify();
            type = DoubleVector;
            std::vector<double> *ptr = new std::vector<double>;
            ptr[0] = x;
            data = (void *)ptr;
        }
        return *this;
    }

    const TagValue &TagValue::operator=(const std::vector<std::string> &x) {
        if (type == StringVector) {
            ((std::vector<std::string> *)data)[0] = x;
        } else {
            nullify();
            type = StringVector;
            std::vector<std::string> *ptr = new std::vector<std::string>;
            ptr[0] = x;
            data = (void *)ptr;
        }
        return *this;
    }

    const TagValue &TagValue::operator=(const std::vector<FCam::Time> &x) {
        if (type == TimeVector) {
            ((std::vector<FCam::Time> *)data)[0] = x;
        } else {
            nullify();
            type = TimeVector;
            std::vector<FCam::Time> *ptr = new std::vector<FCam::Time>;
            ptr[0] = x;
            data = (void *)ptr;
        }
        return *this;
    }

    const TagValue &TagValue::operator=(const TagValue &other) {
        switch(other.type) {
        case Null:
            nullify();
            return *this;
        case Int:
            *this = (int)other;
            return *this;
        case Float:
            *this = (float)other;
            return *this;
        case Double:
            *this = (double)other;
            return *this;
        case String:
            *this = (std::string)other;
            return *this;
        case Time:
            *this = (FCam::Time)other;
            return *this;
        case IntVector: {
            std::vector<int> temp = other;
            *this = temp;
            return *this;
        }
        case FloatVector: {
            std::vector<float> temp = other;
            *this = temp;
            return *this;
        }
        case DoubleVector: {
            std::vector<double> temp = other;
            *this = temp;
            return *this;
        }
        case StringVector: {
            std::vector<std::string> temp = other;
            *this = temp;
            return *this;
        }
        case TimeVector: {
            std::vector<FCam::Time> temp = other;
            *this = temp;
            return *this;
        }
        }
        return *this;
    }

    TagValue::TagValue(const TagValue &other) : type(Null), data(NULL) {
        *this = other;
    }

    TagValue::operator int &() const {
        switch (type) {
        case Null:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a null to an int");
            dummyInt = 0; return dummyInt;
        case Int:
            return ((int *)data)[0];
        case Float:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float to an int");
            dummyInt = 0; return dummyInt;
        case Double:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double to an int");
            dummyInt = 0; return dummyInt;
        case String:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string to an int");
            dummyInt = 0; return dummyInt;
        case Time:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time to an int");
            dummyInt = 0; return dummyInt;
        case IntVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int vector to an int");
            dummyInt = 0; return dummyInt;
        case FloatVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float vector to an int");
            dummyInt = 0; return dummyInt;
        case DoubleVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double vector to an int");
            dummyInt = 0; return dummyInt;
        case StringVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string vector to an int");
            dummyInt = 0; return dummyInt;
        case TimeVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time vector to an int");
            dummyInt = 0; return dummyInt;
        }
        dummyInt = 0; return dummyInt;
    }

    TagValue::operator float &() const {
        switch (type) {
        case Null:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a null to a float");
            dummyFloat = 0; return dummyFloat;
        case Int:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int to a float");
            dummyFloat = 0; return dummyFloat;
        case Float:
            return (((float *)data)[0]);
        case Double:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double to a float");
            dummyFloat = 0; return dummyFloat;
        case String:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string to a float");
            dummyFloat = 0; return dummyFloat;
        case Time:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time to a float");
            dummyFloat = 0; return dummyFloat;
        case IntVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int vector to a float");
            dummyFloat = 0; return dummyFloat;
        case FloatVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float vector to a float");
            dummyFloat = 0; return dummyFloat;
        case DoubleVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double vector to a float");
            dummyFloat = 0; return dummyFloat;
        case StringVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string vector to a float");
            dummyFloat = 0; return dummyFloat;
        case TimeVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time vector to a float");
            dummyFloat = 0; return dummyFloat;
        }
        dummyFloat = 0; return dummyFloat;
    }

    TagValue::operator double &() const {
        switch (type) {
        case Null:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a null to a double");
            dummyDouble = 0; return dummyDouble;
        case Int:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int to a double");
            dummyDouble = 0; return dummyDouble;
        case Float:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float to a double");
            dummyDouble = 0; return dummyDouble;
        case Double:
            return (((double *)data)[0]);
        case String:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string to a double");
            dummyDouble = 0; return dummyDouble;
        case Time:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time to a double");
            dummyDouble = 0; return dummyDouble;
        case IntVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int vector to a double");
            dummyDouble = 0; return dummyDouble;
        case FloatVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float vector to a double");
            dummyDouble = 0; return dummyDouble;
        case DoubleVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double vector to a double");
            dummyDouble = 0; return dummyDouble;
        case StringVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string vector to a double");
            dummyDouble = 0; return dummyDouble;
        case TimeVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time vector to a double");
            dummyDouble = 0; return dummyDouble;
        }
        dummyDouble = 0; return dummyDouble;
    }

    TagValue::operator std::string &() const {
        switch (type) {
        case Null:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a null to a string");
            dummyString.clear(); return dummyString;
        case Int:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int to a string");
            dummyString.clear(); return dummyString;
        case Float:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float to a string");
            dummyString.clear(); return dummyString;
        case Double:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double to a string");
            dummyString.clear(); return dummyString;
        case String:
            return ((std::string *)data)[0];
        case Time:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time to a string");
            dummyString.clear(); return dummyString;
        case IntVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int vector to a string");
            dummyString.clear(); return dummyString;
        case FloatVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float vector to a string");
            dummyString.clear(); return dummyString;
        case DoubleVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double vector to a string");
            dummyString.clear(); return dummyString;
        case StringVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string vector to a string");
            dummyString.clear(); return dummyString;
        case TimeVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time vector to a string");
            dummyString.clear(); return dummyString;
        }
        dummyString.clear(); return dummyString;
    }

    TagValue::operator FCam::Time &() const {
        switch (type) {
        case Null:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a null to a time");
            dummyTime = FCam::Time(0, 0); return dummyTime;
        case Int:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int to a time");
            dummyTime = FCam::Time(0, 0); return dummyTime;
        case Float:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float to a time");
            dummyTime = FCam::Time(0, 0); return dummyTime;
        case Double:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double to a time");
            dummyTime = FCam::Time(0, 0); return dummyTime;
        case String:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string to a time");
            dummyTime = FCam::Time(0, 0); return dummyTime;
        case Time:
            return ((FCam::Time *)data)[0];
        case IntVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int vector to a time");
            dummyTime = FCam::Time(0, 0); return dummyTime;
        case FloatVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float vector to a time");
            dummyTime = FCam::Time(0, 0); return dummyTime;
        case DoubleVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double vector to a time");
            dummyTime = FCam::Time(0, 0); return dummyTime;
        case StringVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string vector to a time");
            dummyTime = FCam::Time(0, 0); return dummyTime;
        case TimeVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time vector to a time");
            dummyTime = FCam::Time(0, 0); return dummyTime;
        }
        dummyTime = FCam::Time(0, 0); return dummyTime;
    }

    TagValue::operator std::vector<int> &() const {
        switch (type) {
        case Null:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a null to an int vector");
            dummyIntVector.clear(); return dummyIntVector;
        case Int:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int to an int vector");
            dummyIntVector.clear(); return dummyIntVector;
        case Float:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float to an int vector");
            dummyIntVector.clear(); return dummyIntVector;
        case Double:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double to an int vector");
            dummyIntVector.clear(); return dummyIntVector;
        case String:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string to an int vector");
            dummyIntVector.clear(); return dummyIntVector;
        case Time:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time to an int vector");
            dummyIntVector.clear(); return dummyIntVector;
        case IntVector:
            return ((std::vector<int> *)data)[0];
        case FloatVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float vector to an int vector");
            dummyIntVector.clear(); return dummyIntVector;
        case DoubleVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double vector to an int vector");
            dummyIntVector.clear(); return dummyIntVector;
        case StringVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string vector to an int vector");
            dummyIntVector.clear(); return dummyIntVector;
        case TimeVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time vector to an int vector");
            dummyIntVector.clear(); return dummyIntVector;
        }
        dummyIntVector.clear(); return dummyIntVector;
    }


    TagValue::operator std::vector<float> &() const {
        switch (type) {
        case Null:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a null to a float vector");
            dummyFloatVector.clear(); return dummyFloatVector;
        case Int:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int to a float vector");
            dummyFloatVector.clear(); return dummyFloatVector;
        case Float:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float to a float vector");
            dummyFloatVector.clear(); return dummyFloatVector;
        case Double:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double to a float vector");
            dummyFloatVector.clear(); return dummyFloatVector;
        case String:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string to a float vector");
            dummyFloatVector.clear(); return dummyFloatVector;
        case Time:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time to a float vector");
            dummyFloatVector.clear(); return dummyFloatVector;
        case IntVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int vector to a float vector");
            dummyFloatVector.clear(); return dummyFloatVector;
        case FloatVector:
            return ((std::vector<float> *)data)[0];
        case DoubleVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double vector to a float vector");
            dummyFloatVector.clear(); return dummyFloatVector;
        case StringVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string vector to a float vector");
            dummyFloatVector.clear(); return dummyFloatVector;
        case TimeVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time vector to a float vector");
            dummyFloatVector.clear(); return dummyFloatVector;
        }
        dummyFloatVector.clear(); return dummyFloatVector;
    }

    TagValue::operator std::vector<double> &() const {
        switch (type) {
        case Null:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a null to a double vector");
            dummyDoubleVector.clear(); return dummyDoubleVector;
        case Int:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int to a double vector");
            dummyDoubleVector.clear(); return dummyDoubleVector;
        case Float:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float to a double vector");
            dummyDoubleVector.clear(); return dummyDoubleVector;
        case Double:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double to a double vector");
            dummyDoubleVector.clear(); return dummyDoubleVector;
        case String:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string to a double vector");
            dummyDoubleVector.clear(); return dummyDoubleVector;
        case Time:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time to a double vector");
            dummyDoubleVector.clear(); return dummyDoubleVector;
        case IntVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int vector to a double vector");
            dummyDoubleVector.clear(); return dummyDoubleVector;
        case FloatVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float vector to a double vector");
            dummyDoubleVector.clear(); return dummyDoubleVector;
        case DoubleVector:
            return ((std::vector<double> *)data)[0];
        case StringVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string vector to a double vector");
            dummyDoubleVector.clear(); return dummyDoubleVector;
        case TimeVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time vector to a double vector");
            dummyDoubleVector.clear(); return dummyDoubleVector;
        }
        dummyDoubleVector.clear(); return dummyDoubleVector;
    }



    TagValue::operator std::vector<std::string> &() const {
        switch (type) {
        case Null:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a null to a string vector");
            dummyStringVector.clear(); return dummyStringVector;
        case Int:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int to a string vector");
            dummyStringVector.clear(); return dummyStringVector;
        case Float:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float to a string vector");
            dummyStringVector.clear(); return dummyStringVector;
        case Double:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double to a string vector");
            dummyStringVector.clear(); return dummyStringVector;
        case String:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string to a string vector");
            dummyStringVector.clear(); return dummyStringVector;
        case Time:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time to a string vector");
            dummyStringVector.clear(); return dummyStringVector;
        case IntVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int vector to a string vector");
            dummyStringVector.clear(); return dummyStringVector;
        case FloatVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float vector to a string vector");
            dummyStringVector.clear(); return dummyStringVector;
        case DoubleVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double vector to a string vector");
            dummyStringVector.clear(); return dummyStringVector;
        case StringVector:
            return ((std::vector<std::string> *)data)[0];
        case TimeVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time vector to a string vector");
            dummyStringVector.clear(); return dummyStringVector;
        }
        dummyStringVector.clear(); return dummyStringVector;
    }

    TagValue::operator std::vector<FCam::Time> &() const {
        switch (type) {
        case Null:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a null to a time vector");
            dummyTimeVector.clear(); return dummyTimeVector;
        case Int:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int to a time vector");
            dummyTimeVector.clear(); return dummyTimeVector;
        case Float:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float to a time vector");
            dummyTimeVector.clear(); return dummyTimeVector;
        case Double:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double to a time vector");
            dummyTimeVector.clear(); return dummyTimeVector;
        case String:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string to a time vector");
            dummyTimeVector.clear(); return dummyTimeVector;
        case Time:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a time to a time vector");
            dummyTimeVector.clear(); return dummyTimeVector;
        case IntVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast an int vector to a time vector");
            dummyTimeVector.clear(); return dummyTimeVector;
        case FloatVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a float vector to a time vector");
            dummyTimeVector.clear(); return dummyTimeVector;
        case DoubleVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a double vector to a time vector");
            dummyTimeVector.clear(); return dummyTimeVector;
        case StringVector:
            postEvent(Event::Error, Event::BadCast, "Cannot cast a string vector to a time vector");
            dummyTimeVector.clear(); return dummyTimeVector;
        case TimeVector:
            return ((std::vector<FCam::Time> *)data)[0];
        }
        dummyTimeVector.clear(); return dummyTimeVector;
    }

    std::string TagValue::toString() const {
        std::ostringstream sstr;
        sstr << *this;
        return sstr.str();
    }

    TagValue TagValue::fromString(const std::string &str) {
        TagValue t;
        std::istringstream sstr(str);
        sstr >> t;
        return t;
    }

    std::string TagValue::toBlob() const {
        std::string str;
        switch(type) {
        case Null:
        {
            str.resize(2);
            str[0] = 'b';
            str[1] = (char)type;
            return str;
        }
        case Int:
        {
            str.resize(4 + sizeof(int));
            str[0] = 'b';
            str[1] = (char)type;
            int *ptr = (int *)((void *)(&(str[4])));
            ptr[0] = (int)(*this);
            return str;
        }
        case Float:
        {
            str.resize(4 + sizeof(float));
            str[0] = 'b';
            str[1] = (char)type;
            float *ptr = (float *)((void *)(&(str[4])));
            ptr[0] = (float)(*this);
            return str;
        }
        case Double:
        {
            str.resize(4 + sizeof(double));
            str[0] = 'b';
            str[1] = (char)type;
            double *ptr = (double *)((void *)(&(str[4])));
            ptr[0] = (double)(*this);
            return str;
        }
        case String: {
            std::string &x = *this;
            str.resize(8 + x.size());
            str[0] = 'b';
            str[1] = (char)type;
            int *ptr = (int *)((void *)(&(str[4])));
            ptr[0] = x.size();
            for (size_t i = 0; i < x.size(); i++) {
                str[i+8] = x[i];
            }
            return str;
        }
        case Time: {
            str.resize(4 + sizeof(int)*2);
            str[0] = 'b';
            str[1] = (char)type;
            int *ptr = (int *)((void *)(&(str[4])));
            FCam::Time time = *this;
            ptr[0] = time.s();
            ptr[1] = time.us();
            return str;
        }
        case IntVector: {
            std::vector<int> &x = *this;
            str.resize(4 + (1+x.size())*sizeof(int));
            str[0] = 'b';
            str[1] = (char)type;
            int *ptr = (int *)((void *)(&(str[4])));
            ptr[0] = x.size();
            for (size_t i = 0; i < x.size(); i++) {
                ptr[i+1] = x[i];
            }
            return str;
        }
        case FloatVector: {
            std::vector<float> &x = *this;
            str.resize(4 + sizeof(int) + x.size()*sizeof(float));
            str[0] = 'b';
            str[1] = (char)type;
            int *iptr = (int *)((void *)(&(str[4])));
            float *fptr = (float *)((void *)(&(str[4 + sizeof(int)])));
            iptr[0] = x.size();
            for (size_t i = 0; i < x.size(); i++) {
                fptr[i] = x[i];
            }
            return str;
        }
        case DoubleVector: {
            std::vector<double> &x = *this;
            str.resize(4 + (1+x.size())*sizeof(double));
            str[0] = 'b';
            str[1] = (char)type;
            int *iptr = (int *)((void *)(&(str[4])));
            double *fptr = (double *)((void *)(&(str[4 + sizeof(int)])));
            iptr[0] = x.size();
            for (size_t i = 0; i < x.size(); i++) {
                fptr[i] = x[i];
            }
            return str;
        }
        case StringVector: {
            std::vector<std::string> &x = *this;
            size_t total = 0;
            for (size_t i = 0; i < x.size(); i++) {
                total += x[i].size();
            }
            str.resize(4 + sizeof(int)*(1+x.size()) + total);
            str[0] = 'b';
            str[1] = (char)type;
            int *ptr = (int *)((void *)(&(str[4])));
            ptr[0] = x.size();
            for (size_t i = 0; i < x.size(); i++) {
                ptr[i+1] = x[i].size();
            }
            int idx = 4 + sizeof(int)*(1+x.size());
            for (size_t i = 0; i < x.size(); i++) {
                for (size_t j = 0; j < x[i].size(); j++) {
                    str[idx+j] = x[i][j];
                }
                idx += x[i].size();
            }
            return str;
        }

        case TimeVector:
            std::vector<FCam::Time> &x = *this;
            str.resize(4 + sizeof(int)*(1+2*x.size()));
            str[0] = 'b';
            str[1] = (char)type;
            int *ptr = (int *)((void *)(&(str[4])));
            ptr[0] = x.size();
            for (size_t i = 0; i < x.size(); i++) {
                ptr[i*2+1] = x[i].s();
                ptr[i*2+2] = x[i].us();
            }
            return str;
        }
        return "";
    }

    std::ostream & operator<<(std::ostream& out, const TagValue &t) {
        out << std::scientific;
        switch (t.type) {
        case TagValue::Null: {
            return (out << "None");
        }

        case TagValue::Int: {
            int contents = t;
            return (out << contents);
        }

        case TagValue::Float: {
            out.precision(8);
            float contents = t;
            return (out << contents << 'f');
        }

        case TagValue::Double: {
            out.precision(16);
            double contents = t;
            return (out << contents);
        }

        case TagValue::String: {
            std::string contents = t;
            out << '"';
            for (size_t j = 0; j < contents.size(); j++) {
                char c = contents[j];
                if (isprint(c) && c != '"' && c != '\\') {
                    out << c;
                } else {
                    int x = (int)c;
                    if (x < 16) {
                        out << "\\x0" << std::hex << x << std::dec;
                    } else {
                        out << "\\x" << std::hex << x << std::dec;
                    }
                }
            }
            out << '"';
            return out;
        }

        case TagValue::Time: {
            Time contents = t;
            return (out << "(" << contents.s() << ", " << contents.us() << ")");
        }

        case TagValue::IntVector: {
            std::vector<int> &contents = t;
            out << "[";
            if (contents.size() > 0) {
                for (size_t i = 0; i < contents.size()-1; i++) {
                    out << contents[i] << ", ";
                }
                out << contents[contents.size()-1];
            }
            out << "]";
            return out;
        }

        case TagValue::FloatVector: {
            out.precision(8);
            std::vector<float> &contents = t;
            out << "[";
            if (contents.size() > 0) {
                for (size_t i = 0; i < contents.size()-1; i++) {
                    out << contents[i] << "f, ";
                }
                out << contents[contents.size()-1];
            }
            out << "]";
            return out;
        }

        case TagValue::DoubleVector: {
            out.precision(16);
            std::vector<double> &contents = t;
            out << "[";
            if (contents.size() > 0) {
                for (size_t i = 0; i < contents.size()-1; i++) {
                    out << contents[i] << ", ";
                }
                out << contents[contents.size()-1];
            }
            out << "]";
            return out;
        }

        case TagValue::StringVector: {
            std::vector<std::string> &contents = t;
            out << "[";
            if (contents.size() > 0) {
                for (size_t i = 0; i < contents.size(); i++) {
                    if (i) out << ", ";
                    out << '"';
                    for (size_t j = 0; j < contents[i].size(); j++) {
                        char c = contents[i][j];
                        if (isprint(c) && c != '"' && c != '\\') {
                            out << c;
                        } else {
                            int x = (int)c;
                            if (x < 16) {
                                out << "\\x0" << std::hex << x << std::dec;
                            } else {
                                out << "\\x" << std::hex << x << std::dec;
                            }
                        }
                    }
                    out << '"';
                }
            }
            out << "]";
            return out;
        }

        case TagValue::TimeVector: {
            std::vector<FCam::Time> &contents = t;
            out << "[";
            if (contents.size() > 0) {
                for (size_t i = 0; i < contents.size(); i++) {
                    if (i) out << ", ";
                    out << "(" << contents[i].s() << ", " << contents[i].us() << ")";
                }
            }
            out << "]";
            return out;
        }

        }
        return out;
    }

    TagValue::Type readType(int x);

    TagValue::Type readType(int x) {
        switch (x) {
        case TagValue::Null:
            return TagValue::Null;
        case TagValue::Int:
            return TagValue::Int;
        case TagValue::Float:
            return TagValue::Float;
        case TagValue::Double:
            return TagValue::Double;
        case TagValue::String:
            return TagValue::String;
        case TagValue::Time:
            return TagValue::Time;
        case TagValue::IntVector:
            return TagValue::IntVector;
        case TagValue::FloatVector:
            return TagValue::FloatVector;
        case TagValue::DoubleVector:
            return TagValue::DoubleVector;
        case TagValue::StringVector:
            return TagValue::StringVector;
        case TagValue::TimeVector:
            return TagValue::TimeVector;
        default:
            error(Event::ParseError, "Invalid type for tag value: %d", x);
            return TagValue::Null;
        }
    }

    std::istream & operator>>(std::istream& in, TagValue &t) {
        switch (in.peek()) {
        case 'b':
            // binary blob
        {
            in.get();
            TagValue::Type type = readType(in.get());
            // skip the next two chars (they are for word-alignment and future proofing)
            if (in.get() != 0 || in.get() != 0) {
                goto error;
            }
            switch (type) {
            case TagValue::Null: {
                TagValue nullTag;
                t = nullTag;
                return in;
            }
            case TagValue::Int: {
                int x;
                in.read((char *)&x, sizeof(int));
                t = x;
                return in;
            }
            case TagValue::Float: {
                float x;
                in.read((char *)&x, sizeof(float));
                t = x;
                return in;
            }
            case TagValue::Double: {
                double x;
                in.read((char *)&x, sizeof(double));
                t = x;
                return in;
            }
            case TagValue::Time: {
                int x[2];
                in.read((char *)x, sizeof(int)*2);
                t = FCam::Time(x[0], x[1]);
                return in;
            }
            case TagValue::String: {
                int size;
                std::string x;
                in.read((char *)&size, sizeof(int));
                x.resize(size);
                in.read((char *)(&(x[0])), size);
                t = x;
                return in;
            }
            case TagValue::IntVector: {
                int size;
                std::vector<int> v;
                t = v;
                std::vector<int> &x = t;
                in.read((char *)&size, sizeof(int));
                x.resize(size);
                in.read((char *)(&(x[0])), sizeof(int)*size);
                return in;
            }
            case TagValue::FloatVector: {
                int size;
                std::vector<float> v;
                t = v;
                std::vector<float> &x = t;
                in.read((char *)&size, sizeof(int));
                x.resize(size);
                in.read((char *)(&(x[0])), sizeof(float)*size);
                return in;
            }
            case TagValue::DoubleVector: {
                int size;
                std::vector<double> v;
                t = v;
                std::vector<double> &x = t;
                in.read((char *)&size, sizeof(int));
                x.resize(size);
                in.read((char *)(&(x[0])), sizeof(double)*size);
                return in;
            }
            case TagValue::StringVector: {
                int size;
                std::vector<std::string> v;
                t = v;
                std::vector<std::string> &x = t;
                in.read((char *)&size, sizeof(int));
                x.resize(size);
                for (size_t i = 0; i < x.size(); i++) {
                    in.read((char *)&size, sizeof(int));
                    x[i].resize(size);
                }
                for (size_t i = 0; i < x.size(); i++) {
                    in.read((char *)(&(x[i][0])), x[i].size());
                }
                return in;
            }
            case TagValue::TimeVector: {
                int size;
                std::vector<FCam::Time> v;
                t = v;
                std::vector<FCam::Time> &x = t;
                in.read((char *)&size, sizeof(int));
                x.resize(size);
                for (size_t i = 0; i < x.size(); i++) {
                    int time[2];
                    in.read((char *)time, sizeof(int)*2);
                    x[i] = FCam::Time(time[0], time[1]);
                }
                return in;
            }
            default:
                goto error;
            }
        }
        case '-': case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            // numeric
        {
            // suck it into a string
            std::ostringstream osstr;

            bool floating = false;
            if (in.peek() == '-') {
                osstr << char(in.get());
                if (!isdigit(in.peek())) {
                    // oops
                    in.putback('-');
                    goto error;
                }
            }
            while (isdigit(in.peek())) osstr << char(in.get());
            if (in.peek() == '.') {
                floating = true;
                osstr << char(in.get());
                if (!isdigit(in.peek())) {
                    // oops!
                    in.putback('.');
                    floating = false;
                    goto done;
                }
            }
            while (isdigit(in.peek())) osstr << char(in.get());
            if (in.peek() == 'e') osstr << char(in.get());
            else goto done;
            if (in.peek() == '+') osstr << char(in.get());
            else if (in.peek() == '-') osstr << char(in.get());
            else {
                // oops!
                in.putback('e');
                goto done;
            }
            while (isdigit(in.peek())) osstr << char(in.get());

          done:
            std::istringstream isstr(osstr.str());
            if (floating && in.peek() == 'f') {
                in.get();
                float x;
                isstr >> x;
                t = x;
            } else if (floating) {
                double x;
                isstr >> x;
                t = x;
            } else {
                int x;
                isstr >> x;
                t = x;
            }
            return in;
        }
        case 'N':
        {
            in.get();
            if (in.peek() != 'o') {
                in.putback('N');
                goto error;
            }
            in.get();
            if (in.peek() != 'n') {
                in.putback('o');
                in.putback('N');
                goto error;
            }
            in.get();
            if (in.peek() != 'e') {
                in.putback('n');
                in.putback('o');
                in.putback('N');
                goto error;
            }
            in.get();
            // None
            TagValue null;
            t = null;
            return in;
        }
        case '(':
        {
            in.get();
            timeval tv;
            in >> tv.tv_sec;
            in >> std::ws;
            if (in.peek() != ',') goto error;
            in.get();
            in >> std::ws;
            in >> tv.tv_usec;
            if (in.peek() != ')') goto error;
            in.get();
            FCam::Time time(tv);
            t = time;
            return in;
        }
        case '"':
        {
            // string
            in.get();
            std::ostringstream osstr;
            while (in.peek() != '"') {
                if (in.peek() < 0) {
                    t = osstr.str();
                    goto error;
                } else if (in.peek() == '\\') {
                    in.get();
                    switch(in.get()) {
                    case '0':
                        osstr << '\0';
                        break;
                    case 'n':
                        osstr << '\n';
                        break;
                    case 'r':
                        osstr << '\r';
                        break;
                    case 'x': {
                        int v = 0;
                        char c1 = in.get();
                        char c2 = in.get();
                        if (c1 >= '0' && c1 <= '9') {
                            v = (c1 - '0');
                        } else if (c1 >= 'a' && c1 <= 'f') {
                            v = (c1 - 'a' + 10);
                        } else {
                            in.putback(c2);
                            in.putback(c1);
                            t = osstr.str();
                            goto error;
                        }
                        v = v << 4;
                        if (c2 >= '0' && c2 <= '9') {
                            v += (c2 - '0');
                        } else if (c2 >= 'a' && c2 <= 'f') {
                            v += (c2 - 'a' + 10);
                        } else {
                            in.putback(c2);
                            in.putback(c1);
                            t = osstr.str();
                            goto error;
                        }
                        osstr << (char)v;
                        break;
                    }
                    case '"':
                        osstr << '"';
                        break;
                    case '\\':
                        osstr << '\\';
                        break;
                    case 'a':
                        osstr << '\a';
                        break;
                    case 'b':
                        osstr << '\b';
                        break;
                    case 'v':
                        osstr << '\v';
                        break;
                    case '\'':
                        osstr << '\'';
                        break;
                    case 'f':
                        osstr << '\f';
                        break;
                    case 't':
                        osstr << '\t';
                        break;
                    default:
                        t = osstr.str();
                        goto error;
                    }
                } else {
                    osstr << (char)in.get();
                }
            }
            in.get();
            t = osstr.str();
            return in;
        }
        case '[':
        {
            // an array
            in.get();

            // consume whitespace
            in >> std::ws;
            // check it's not a list of lists
            if (in.peek() == '[') goto error;
            // recursively parse the first entry
            TagValue first;
            in >> first;
            switch (first.type) {
            case TagValue::Int:
            {
                std::vector<int> v;
                t = v;
                std::vector<int> &l = t;
                l.push_back(first);
                in >> std::ws;
                while (in.peek() != ']') {
                    int x;
                    if (in.peek() != ',') goto error;
                    in.get();
                    in >> std::ws >> x >> std::ws;
                    l.push_back(x);
                }
                in.get();
                return in;
            }
            case TagValue::Float:
            {
                std::vector<float> v;
                t = v;
                std::vector<float> &l = t;
                l.push_back(first);
                in >> std::ws;
                while (in.peek() != ']') {
                    float x;
                    if (in.peek() != ',') goto error;
                    in.get();
                    in >> std::ws >> x;
                    if (in.peek() == 'f') in.get();
                    in >> std::ws;
                    l.push_back(x);
                }
                in.get();
                return in;
            }
            case TagValue::Double:
            {
                std::vector<double> v;
                t = v;
                std::vector<double> &l = t;
                l.push_back(first);
                in >> std::ws;
                while (in.peek() != ']') {
                    double x;
                    if (in.peek() != ',') goto error;
                    in.get();
                    in >> std::ws >> x >> std::ws;
                    l.push_back(x);
                }
                in.get();
                return in;
            }
            case TagValue::Time:
            {
                std::vector<FCam::Time> v;
                t = v;
                std::vector<FCam::Time> &l = t;
                l.push_back(first);
                in >> std::ws;
                while (in.peek() != ']') {
                    TagValue x;
                    if (in.peek() != ',') goto error;
                    in.get();
                    in >> std::ws >> x >> std::ws;
                    if (x.type != TagValue::Time) goto error;
                    l.push_back((FCam::Time)x);
                }
                in.get();
                return in;
            }
            case TagValue::String:
            {
                std::vector<std::string> v;
                t = v;
                std::vector<std::string> &l = t;
                l.push_back(first);
                in >> std::ws;
                while (in.peek() != ']') {
                    TagValue x;
                    if (in.peek() != ',') goto error;
                    in.get();
                    in >> std::ws >> x >> std::ws;
                    if (x.type != TagValue::String) goto error;
                    l.push_back((std::string)x);
                }
                in.get();
                return in;
            }
            default:
                goto error;
            }
            if (first.type == TagValue::Null) {
                goto error;
            }

            return in;
        }
        case EOF:
            // the stream eof bit is now set
            t = TagValue();
            return in;
        default:
            goto error;
        }

        return in;
      error:
        error(Event::ParseError, "Could not parse TagValue");
        return in;
    }

    int TagValue::dummyInt;
    float TagValue::dummyFloat;
    double TagValue::dummyDouble;
    std::string TagValue::dummyString;
    FCam::Time TagValue::dummyTime;
    std::vector<int> TagValue::dummyIntVector;
    std::vector<float> TagValue::dummyFloatVector;
    std::vector<double> TagValue::dummyDoubleVector;
    std::vector<std::string> TagValue::dummyStringVector;
    std::vector<FCam::Time> TagValue::dummyTimeVector;

}
