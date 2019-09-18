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

#include "apl/component/textmeasurement.h"

namespace apl {

class DummyTextMeasurement : public TextMeasurement {
public:
    YGSize measure( TextComponent *component,
                    float width,
                    YGMeasureMode widthMode,
                    float height,
                    YGMeasureMode heightMode ) override {
        return { .width=10, .height=10 };
    }

    virtual float baseline( TextComponent *component,
                            float width,
                            float height ) override {
        return height * 0.5;
    }
};

static TextMeasurementPtr sTextMeasurement = std::make_shared<DummyTextMeasurement>();

void
TextMeasurement::install(const TextMeasurementPtr& textMeasurement)
{
    sTextMeasurement = textMeasurement;
}

const TextMeasurementPtr&
TextMeasurement::instance()
{
    return sTextMeasurement;
}

} // namespace apl
