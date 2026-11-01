#ifndef FUNCTIONAL_SPACE_CSV_LOADER_H
#define FUNCTIONAL_SPACE_CSV_LOADER_H

#include "FunctionalSpace.h"

#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>


// ============================================================
// MARKET BAR
// ============================================================
//
// CSV format:
//
// timestamp,open,high,low,close,volume
//
// timestamp format:
//
// YYYY-MM-DD
//
// ============================================================

struct MarketBar {

    std::string timestamp;

    double open   = 0.0;
    double high   = 0.0;
    double low    = 0.0;
    double close  = 0.0;
    double volume = 0.0;
};


// ============================================================
// FUNCTIONAL SPACE CSV LOADER
// ============================================================

class FunctionalSpaceCSVLoader {


public:

    // ========================================================
    // LOAD
    // ========================================================

    static FunctionalSpace load(
        const std::string& filename
    ) {

        std::cout << "[1] Entered load()\n";

        std::vector<MarketBar> bars =
            readCSV(filename);

        std::cout
            << "[2] readCSV() returned: "
            << bars.size()
            << " rows\n";


        if (bars.empty()) {

            throw std::runtime_error(
                "CSV contains no valid observations."
            );
        }


        std::cout << "[3] Sorting...\n";

        std::sort(
            bars.begin(),
            bars.end(),
            [](
                const MarketBar& a,
                const MarketBar& b
            ) {

                return a.timestamp < b.timestamp;
            }
        );

        std::cout << "[4] Sorting complete\n";


        std::cout << "[5] Removing duplicates...\n";

        bars.erase(
            std::unique(
                bars.begin(),
                bars.end(),
                [](
                    const MarketBar& a,
                    const MarketBar& b
                ) {

                    return a.timestamp == b.timestamp;
                }
            ),
            bars.end()
        );

        std::cout
            << "[6] Rows after duplicate removal: "
            << bars.size()
            << "\n";


        if (bars.empty()) {

            throw std::runtime_error(
                "No observations remain after cleaning."
            );
        }


        std::cout << "[7] Building temporal vector...\n";

        std::vector<double> v;

        v.reserve(bars.size());


        if (bars.size() == 1) {

            v.push_back(0.0);
        }
        else {

            const double denominator =
                static_cast<double>(bars.size() - 1);

            for (
                std::size_t i = 0;
                i < bars.size();
                ++i
            ) {

                v.push_back(
                    static_cast<double>(i) /
                    denominator
                );
            }
        }

        std::cout
            << "[8] Temporal vector created: "
            << v.size()
            << " values\n";


        // ----------------------------------------------------
        // Extract market columns
        // ----------------------------------------------------

        std::vector<double> open;
        std::vector<double> high;
        std::vector<double> low;
        std::vector<double> close;
        std::vector<double> volume;


        open.reserve(bars.size());
        high.reserve(bars.size());
        low.reserve(bars.size());
        close.reserve(bars.size());
        volume.reserve(bars.size());


        for (
            const MarketBar& bar :
            bars
        ) {

            open.push_back(
                bar.open
            );

            high.push_back(
                bar.high
            );

            low.push_back(
                bar.low
            );

            close.push_back(
                bar.close
            );

            volume.push_back(
                bar.volume
            );
        }


        // ----------------------------------------------------
        // Construct FunctionalSpace
        // ----------------------------------------------------

        FunctionalSpace space;


        // Independent variable.
        //
        // v represents normalized time.

        for (
            double time :
            v
        ) {

            space.addRow(
                time
            );
        }


        // ----------------------------------------------------
        // Create continuous functions
        // ----------------------------------------------------

        std::cout << "[9] Creating open function...\n";

        space.addFunction(
            createInterpolatedFunction(
                v,
                open,
                "Open"
            )
        );

        std::cout << "[10] Open function created\n";

        std::cout << "[11] Creating high function...\n";

        space.addFunction(
            createInterpolatedFunction(
                v,
                high,
                "High"
            )
        );

        std::cout << "[12] High function created\n";

        std::cout << "[13] Creating low function...\n";

        space.addFunction(
            createInterpolatedFunction(
                v,
                low,
                "Low"
            )
        );

        std::cout << "[14] Low function created\n";

        space.addFunction(
            createInterpolatedFunction(
                v,
                close,
                "Close"
            )
        );

        std::cout << "[15] Close function created\n";

        space.addFunction(
            createInterpolatedFunction(
                v,
                volume,
                "Volume"
            )
        );

        std::cout << "[16] Volume function created\n";

        return space;
    }


private:

    // ========================================================
    // CSV READER
    // ========================================================

    static std::vector<MarketBar>
    readCSV(
        const std::string& filename
    ) {

        std::cout
            << "[CSV 1] Opening file: "
            << filename
            << "\n";


        std::ifstream file(filename);


        if (!file.is_open()) {

            throw std::runtime_error(
                "Unable to open CSV file: " +
                filename
            );
        }


        std::cout
            << "[CSV 2] File opened successfully\n";


        std::vector<MarketBar> bars;

        std::string line;

        bool firstLine = true;

        std::size_t lineNumber = 0;


        while (
            std::getline(
                file,
                line
            )
        ) {

            ++lineNumber;


            std::cout
                << "[CSV 3] Reading line "
                << lineNumber
                << "\n";


            if (
                !line.empty() &&
                line.back() == '\r'
            ) {

                line.pop_back();
            }


            if (line.empty()) {

                continue;
            }


            if (firstLine) {

                firstLine = false;


                std::cout
                    << "[CSV 4] Header: "
                    << line
                    << "\n";


                if (
                    line ==
                    "timestamp,open,high,low,close,volume"
                ) {

                    continue;
                }
            }


            std::stringstream stream(line);

            std::vector<std::string> fields;

            std::string field;


            while (
                std::getline(
                    stream,
                    field,
                    ','
                )
            ) {

                fields.push_back(field);
            }


            std::cout
                << "[CSV 5] Fields: "
                << fields.size()
                << "\n";


            if (
                fields.size() != 6
            ) {

                std::cout
                    << "[CSV] Skipping malformed line "
                    << lineNumber
                    << "\n";

                continue;
            }


            try {

                MarketBar bar;


                bar.timestamp =
                    fields[0];


                bar.open =
                    std::stod(
                        fields[1]
                    );


                bar.high =
                    std::stod(
                        fields[2]
                    );


                bar.low =
                    std::stod(
                        fields[3]
                    );


                bar.close =
                    std::stod(
                        fields[4]
                    );


                bar.volume =
                    std::stod(
                        fields[5]
                    );


                if (
                    !isValid(bar)
                ) {

                    std::cout
                        << "[CSV] Invalid row "
                        << lineNumber
                        << "\n";

                    continue;
                }


                bars.push_back(bar);


            }
            catch (
                const std::exception& e
            ) {

                std::cout
                    << "[CSV] Parse error on line "
                    << lineNumber
                    << ": "
                    << e.what()
                    << "\n";

                continue;
            }
        }


        std::cout
            << "[CSV 6] Finished reading file\n";


        std::cout
            << "[CSV 7] Valid bars: "
            << bars.size()
            << "\n";


        return bars;
    }



    // ========================================================
    // VALIDATION
    // ========================================================

    static bool isValid(
        const MarketBar& bar
    ) {

        // ----------------------------------------------------
        // Date
        // ----------------------------------------------------

        if (
            !isValidDate(
                bar.timestamp
            )
        ) {

            return false;
        }


        // ----------------------------------------------------
        // Numeric values
        // ----------------------------------------------------

        if (
            !std::isfinite(bar.open) ||
            !std::isfinite(bar.high) ||
            !std::isfinite(bar.low) ||
            !std::isfinite(bar.close) ||
            !std::isfinite(bar.volume)
        ) {

            return false;
        }


        // ----------------------------------------------------
        // Price constraints
        // ----------------------------------------------------

        if (
            bar.open <= 0.0 ||
            bar.high <= 0.0 ||
            bar.low <= 0.0 ||
            bar.close <= 0.0
        ) {

            return false;
        }


        // ----------------------------------------------------
        // Volume constraint
        // ----------------------------------------------------

        if (
            bar.volume < 0.0
        ) {

            return false;
        }


        // ----------------------------------------------------
        // OHLC consistency
        // ----------------------------------------------------

        if (
            bar.high < bar.low
        ) {

            return false;
        }


        if (
            bar.open < bar.low ||
            bar.open > bar.high
        ) {

            return false;
        }


        if (
            bar.close < bar.low ||
            bar.close > bar.high
        ) {

            return false;
        }


        return true;
    }



    // ========================================================
    // DATE VALIDATION
    // ========================================================
    //
    // Expected:
    //
    // YYYY-MM-DD
    //
    // Example:
    //
    // 2026-08-11
    //
    // ========================================================

    static bool isValidDate(
        const std::string& date
    ) {

        if (
            date.size() != 10
        ) {

            return false;
        }


        if (
            date[4] != '-' ||
            date[7] != '-'
        ) {

            return false;
        }


        for (
            std::size_t i = 0;
            i < date.size();
            ++i
        ) {

            if (
                i == 4 ||
                i == 7
            ) {

                continue;
            }


            if (
                date[i] < '0' ||
                date[i] > '9'
            ) {

                return false;
            }
        }


        return true;
    }



    // ========================================================
    // INTERPOLATION
    // ========================================================
    //
    // Converts discrete market observations into a continuous
    // mathematical function:
    //
    //     f(t)
    //
    // using piecewise-linear interpolation.
    //
    // ========================================================

    static Function
    createInterpolatedFunction(
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::string& name
    ) {

        if (
            x.empty() ||
            y.empty()
        ) {

            throw std::invalid_argument(
                "Cannot create function from empty data."
            );
        }


        if (
            x.size() != y.size()
        ) {

            throw std::invalid_argument(
                "Interpolation vectors must have equal sizes."
            );
        }


        std::function<double(double)> interpolation =

            [
                x,
                y
            ](
                double value
            ) {

                // ------------------------------------------------
                // Before first point
                // ------------------------------------------------

                if (
                    value <= x.front()
                ) {

                    return y.front();
                }


                // ------------------------------------------------
                // After final point
                // ------------------------------------------------

                if (
                    value >= x.back()
                ) {

                    return y.back();
                }


                // ------------------------------------------------
                // Find surrounding points
                // ------------------------------------------------

                auto upper =
                    std::upper_bound(
                        x.begin(),
                        x.end(),
                        value
                    );


                std::size_t right =
                    static_cast<std::size_t>(
                        std::distance(
                            x.begin(),
                            upper
                        )
                    );


                std::size_t left =
                    right - 1;


                // ------------------------------------------------
                // Coordinates
                // ------------------------------------------------

                double x1 =
                    x[left];

                double x2 =
                    x[right];

                double y1 =
                    y[left];

                double y2 =
                    y[right];


                // ------------------------------------------------
                // Linear interpolation
                // ------------------------------------------------

                double denominator =
                    x2 - x1;


                if (
                    denominator == 0.0
                ) {

                    return y1;
                }


                double ratio =
                    (
                        value - x1
                    )
                    /
                    denominator;


                return
                    y1 +
                    ratio *
                    (
                        y2 - y1
                    );
            };


        return Function(
            std::move(interpolation),
            name
        );
    }
};


#endif