#ifndef INTERSECTION_CALCULATOR_H
#define INTERSECTION_CALCULATOR_H

#include "Function.h"

#include <cmath>
#include <vector>
#include <algorithm>


// ============================================================
// POINT
// ============================================================

struct Point {

    double x = 0.0;
    double y = 0.0;
};


// ============================================================
// VECTOR
// ============================================================

class Vector {

private:

    double slope_;
    double intercept_;


public:

    Vector(
        double slope,
        double intercept
    )
        : slope_(slope),
          intercept_(intercept)
    {
    }


    double evaluate(double x) const {

        return
            slope_ * x +
            intercept_;
    }
};


// ============================================================
// INTERSECTION CALCULATOR
// ============================================================

class IntersectionCalculator {

public:

    static std::vector<Point> calculate(
        const Function& function,
        const Vector& vector,
        double xMin,
        double xMax,
        int samples = 1000
    ) {

        std::vector<Point> intersections;

        if (samples <= 0) {

            return intersections;
        }


        double step =
            (xMax - xMin) /
            static_cast<double>(samples);


        double previousX =
            xMin;


        double previousDifference =
            function.evaluate(previousX)
            -
            vector.evaluate(previousX);


        for (
            int i = 1;
            i <= samples;
            ++i
        ) {

            double currentX =
                xMin +
                i * step;


            double currentDifference =
                function.evaluate(currentX)
                -
                vector.evaluate(currentX);


            // ----------------------------------------------------
            // Exact intersection
            // ----------------------------------------------------

            if (
                std::abs(currentDifference)
                <
                1e-10
            ) {

                intersections.push_back({

                    currentX,

                    function.evaluate(currentX)

                });
            }


            // ----------------------------------------------------
            // Sign change
            // ----------------------------------------------------

            else if (
                (
                    previousDifference < 0.0 &&
                    currentDifference > 0.0
                )
                ||
                (
                    previousDifference > 0.0 &&
                    currentDifference < 0.0
                )
            ) {

                double left =
                    previousX;

                double right =
                    currentX;


                double leftValue =
                    previousDifference;


                // ------------------------------------------------
                // Bisection
                // ------------------------------------------------

                for (
                    int iteration = 0;
                    iteration < 50;
                    ++iteration
                ) {

                    double middle =
                        (left + right) /
                        2.0;


                    double middleValue =
                        function.evaluate(middle)
                        -
                        vector.evaluate(middle);


                    if (
                        (
                            leftValue < 0.0 &&
                            middleValue > 0.0
                        )
                        ||
                        (
                            leftValue > 0.0 &&
                            middleValue < 0.0
                        )
                    ) {

                        right =
                            middle;
                    }
                    else {

                        left =
                            middle;

                        leftValue =
                            middleValue;
                    }
                }


                double x =
                    (left + right) /
                    2.0;


                intersections.push_back({

                    x,

                    function.evaluate(x)

                });
            }


            previousX =
                currentX;

            previousDifference =
                currentDifference;
        }


        return removeDuplicates(
            intersections
        );
    }


private:

    static std::vector<Point>
    removeDuplicates(
        const std::vector<Point>& points
    ) {

        std::vector<Point> result;


        for (const auto& point : points) {

            bool duplicate =
                false;


            for (
                const auto& existing :
                result
            ) {

                double dx =
                    point.x -
                    existing.x;

                double dy =
                    point.y -
                    existing.y;


                double distance =
                    std::sqrt(
                        dx * dx +
                        dy * dy
                    );


                if (
                    distance <
                    1e-6
                ) {

                    duplicate =
                        true;

                    break;
                }
            }


            if (!duplicate) {

                result.push_back(
                    point
                );
            }
        }


        return result;
    }
};

#endif