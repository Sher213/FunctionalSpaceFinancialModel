#ifndef FUNCTION_H
#define FUNCTION_H

#include <functional>
#include <string>
#include <utility>

class Function {

private:

    std::function<double(double)> function_;

    std::string name_;

    // Parameters exposed to optimization.
    double verticalOffset_;
    double horizontalOffset_;
    double scale_;


public:

    Function(
        std::function<double(double)> function,
        std::string name,
        double verticalOffset = 0.0,
        double horizontalOffset = 0.0,
        double scale = 1.0
    )
        : function_(std::move(function)),
          name_(std::move(name)),
          verticalOffset_(verticalOffset),
          horizontalOffset_(horizontalOffset),
          scale_(scale)
    {
    }


    // ------------------------------------------------------------
    // EVALUATION
    // ------------------------------------------------------------

    double evaluate(double x) const {

        return
            scale_ *
            function_(x - horizontalOffset_)
            +
            verticalOffset_;
    }


    // ------------------------------------------------------------
    // NAME
    // ------------------------------------------------------------

    const std::string& getName() const {

        return name_;
    }


    const std::string& name() const {

        return name_;
    }


    void setName(const std::string& name) {

        name_ = name;
    }


    // ------------------------------------------------------------
    // PARAMETERS
    // ------------------------------------------------------------

    double verticalOffset() const {

        return verticalOffset_;
    }


    double horizontalOffset() const {

        return horizontalOffset_;
    }


    double scale() const {

        return scale_;
    }


    void setVerticalOffset(double value) {

        verticalOffset_ = value;
    }


    void setHorizontalOffset(double value) {

        horizontalOffset_ = value;
    }


    void setScale(double value) {

        scale_ = value;
    }
};

#endif