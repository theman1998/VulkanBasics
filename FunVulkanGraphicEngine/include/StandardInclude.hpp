#pragma once

#include <iostream>
#include <vector>
#include <sstream>
#include <map>


namespace std
{

    template <typename T>
    std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
        os << "[";
        for (size_t i = 0; i < vec.size(); i++)
        {
            os << vec[i];
            if (i + 1 != vec.size())
                os << ", ";
        }
        os << " ]";
        return os;
    }


    template <typename Key, typename Value>
    std::ostream& operator<<(std::ostream& os, const std::map<Key, Value>& m) {
        os << "{";
        size_t i = 0;
        for (auto it = m.begin(); it != m.end(); it++)
        {
            os << it->first << " : " << it->second;
            if (i + 1 != m.size())
                os << ", ";
            i++;
        }
        os << "}";
        return os;
    }
	
}