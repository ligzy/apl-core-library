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

#ifndef _APL_VERISON_H
#define _APL_VERISON_H

#include <cstdint>

namespace apl {

class APLVersion {
public:
    enum Value : uint32_t {
        kAPLVersionIgnore = 0x0, /// Ignore version numbers
        kAPLVersion10 = 0x1, /// Support version 1.0
        kAPLVersion11 = 0x1U << 1, /// Support version 1.1
        kAPLVersion12 = 0x1U << 2, /// Support version 1.2
        kAPLVersion13 = 0x1U << 3, /// Support version 1.3
        kAPLVersion10to11 = kAPLVersion10 | kAPLVersion11, /// Convenience for 1.0 through 1.1
        kAPLVersion10to12 = kAPLVersion10to11 | kAPLVersion12,
        kAPLVersion10to13 = kAPLVersion10to12 | kAPLVersion13,
        kAPLVersionLatest = kAPLVersion12, /// Support the most recent engine version
        kAPLVersionDefault = kAPLVersion10to12, /// Default value
        kAPLVersionAny = 0xffffffff, /// Support any versions in the list
    };

    APLVersion() = default;
    constexpr APLVersion(Value v) : mValue(v) {}

    bool isValid(Value other) const;
    bool isValid(const std::string& other) const;

    bool operator==(const APLVersion& rhs) const { return mValue == rhs.mValue; }
    bool operator!=(const APLVersion& rhs) const { return mValue != rhs.mValue; }

private:
    Value mValue;
};

} // namespace apl

#endif // _APL_VERISON_H