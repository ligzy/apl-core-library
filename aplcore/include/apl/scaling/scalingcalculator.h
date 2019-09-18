/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef _APL_SCALINGCALCULATOR_H
#define _APL_SCALINGCALCULATOR_H

#include "apl/scaling/metricstransform.h"
#include <utility>
#include <cmath>
#include <tuple>

namespace apl {
namespace scaling {
// Don't expose ScalingCalculator internals
namespace {

/** Constant PI */
constexpr double PI = 3.14159265358979323846;

/** How far to check each angle for square viewport in round screen */
const double ANGLE_DELTA = PI * 1.0f / 180;

/** Maximum allowable viewport aspect ration for square viewport in round screen */
const double MAX_VIEWPORT_RATIO = 3.0f;

/**
 * Sets the pixrl width and height of the metrics object
 * @param width in dp
 * @param height in dp
 * @param metrics the metrics object
 */
void setMetricsSize(Metrics& metrics, double width, double height);

class ScalingCalculator {
public:
    ScalingCalculator(double Vw, double Vh, double k)
            : mViewportWidth(Vw), mViewportHeight(Vh), k(k) {}

    class Size {
    public:
        Size() : w(0), h(0) {}
        Size(double w, double h, bool isRound, const ViewportSpecification& spec) : w(w), h(h), spec(spec) {}
        double w;
        double h;
        ViewportSpecification spec;
    };

    /**
     * Calculates the cost at a given size.
     * @param size The specification size
     * @return The cost
     */
    double cost(const Size& size);

    double cost(double w, double h);

    void minimumFixedHeight(double height, double wmin, double wmax, Size& size, double& minCost);

    void minimumFixedWidth(double width, double hmin, double hmax, Size& size, double& minCost);

    /**
     * Calculates the scale factor at the given size
     * @param size
     * @return The scale
     */
    double scaleFactor(const Size& size) { return scaleFactor(size.w, size.h); }
    double scaleFactor(double w, double h) {
        return std::min(mViewportWidth / w, mViewportHeight / h);
    }

    /** Device's actual viewport width  */
    double mViewportWidth;

    /** Device's actual viewport height  */
    double mViewportHeight;

    /** Weighting constant to bias between scaling and percentage of the covered screen */
    double k;
};

} // namespace


/**
 * Calculate the scaling factor for the given metrics and scaling
 * options
 * @param metrics Current metrics object
 * @param options Scaling options
 * @return pair containing the scalefactor and a new metrics object with the new virtual
 *         viewport width and height.
 */
std::tuple<double, Metrics, ViewportSpecification>
calculate(const Metrics& metrics, const ScalingOptions& options);


} // namespace scaling

} // namespace apl

#endif // APL_SCALINGCALCULATOR_H
