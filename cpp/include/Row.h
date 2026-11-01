#ifndef ROW_H
#define ROW_H

#include <string>

class Row {
public:
    Row(const std::string& data) : data_(data) {}
    const std::string& getData() const { return data_; }
private:
    std::string data_;
};

#endif // ROW_H
