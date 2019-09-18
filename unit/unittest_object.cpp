/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <iostream>
#include <memory>
#include <clocale>

#include "gtest/gtest.h"

#include "apl/engine/context.h"
#include "apl/content/metrics.h"
#include "apl/primitives/object.h"
#include "apl/primitives/rect.h"
#include "apl/primitives/transform.h"
#include "apl/engine/arrayify.h"
#include "apl/utils/session.h"

using namespace apl;

TEST(ObjectTest, Constants)
{
    ASSERT_TRUE( Object::TRUE().isBoolean());
    ASSERT_TRUE( Object::TRUE().getBoolean());
    ASSERT_TRUE( Object::FALSE().isBoolean());
    ASSERT_FALSE( Object::FALSE().getBoolean());

    ASSERT_TRUE( Object::NULL_OBJECT().isNull());
    ASSERT_TRUE( Object::NAN_OBJECT().isNumber());
    ASSERT_TRUE( Object::AUTO_OBJECT().isAutoDimension());
    ASSERT_TRUE( Object::EMPTY_ARRAY().isArray());
    ASSERT_TRUE( Object::EMPTY_RECT().isRect());
}

TEST(ObjectTest, Basic)
{
    ASSERT_TRUE( Object().isNull() );
    ASSERT_TRUE( Object(true).isBoolean());
    ASSERT_TRUE( Object(false).isBoolean());
    ASSERT_TRUE( Object(10).isNumber());
    ASSERT_TRUE( Object((uint32_t) 23).isNumber());
    ASSERT_TRUE( Object(10.2).isNumber());
    ASSERT_TRUE( Object("fuzzy").isString());
    ASSERT_TRUE( Object(std::string("fuzzy")).isString());
}

TEST(ObjectTest, Size)
{
    ASSERT_TRUE(Object::NULL_OBJECT().empty());

    ASSERT_FALSE(Object("fuzzy").empty());
    ASSERT_TRUE(Object("").empty());
    ASSERT_FALSE(Object(1).empty());
    ASSERT_FALSE(Object(false).empty());

    Object a = Object(std::make_shared<std::map<std::string, Object>>());
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0, a.size());

    a = Object(std::make_shared<std::vector<Object>>());
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0, a.size());

    a = Object(std::vector<Object>{});
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0, a.size());

    a = Object(rapidjson::Value(rapidjson::kArrayType));
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0, a.size());

    a = Object(rapidjson::Value(rapidjson::kObjectType));
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0, a.size());

    a = Object(Rect(0,0,0,0));
    ASSERT_TRUE(a.empty());
    ASSERT_EQ(0, a.size());

    ASSERT_TRUE(Object::EMPTY_ARRAY().empty());
    ASSERT_TRUE(Object::EMPTY_RECT().empty());
}

TEST(ObjectTest, SharedMap)
{
    Object a = Object(std::make_shared< std::map< std::string, Object> >(
            std::map<std::string, Object>({
        { "a", 1 }, { "b", false }, { "c", "fuzzy"}
    })));
    ASSERT_TRUE(a.isMap());
    ASSERT_EQ(3, a.size());
    ASSERT_FALSE(a.empty());
    ASSERT_TRUE(a.has("a"));
    ASSERT_FALSE(a.has("z"));

    ASSERT_STREQ("fuzzy", a.get("c").getString().c_str());
}

TEST(ObjectTest, SharedVector)
{
    Object a = Object(std::make_shared< std::vector<Object>>(
            std::vector<Object>({ true, 1, "fuzzy" })));

    ASSERT_TRUE(a.isArray());
    ASSERT_EQ(3, a.size());
    ASSERT_FALSE(a.empty());
    ASSERT_TRUE(a.at(0).isBoolean());
    ASSERT_EQ(1, a.at(1).getInteger());
    ASSERT_STREQ("fuzzy", a.at(2).getString().c_str());
}

TEST(ObjectTest, Vector)
{
    Object a = Object({ true, 1, "fuzzy" });
    ASSERT_TRUE(a.isArray());
    ASSERT_EQ(3, a.size());
    ASSERT_FALSE(a.empty());
    ASSERT_TRUE(a.at(0).isBoolean());
    ASSERT_EQ(1, a.at(1).getInteger());
    ASSERT_STREQ("fuzzy", a.at(2).getString().c_str());
}

TEST(ObjectTest, RapidJson)
{
    // Note: We require the JSON object to be persistent.
    rapidjson::Value v(10);
    Object o = Object(v);

    ASSERT_TRUE(o.isNumber());
    ASSERT_EQ(10, o.getInteger());

    rapidjson::Value v2("twelve");
    ASSERT_TRUE(Object(v2).isString());
    ASSERT_STREQ("twelve", Object(v2).getString().c_str());

    rapidjson::Value v3(true);
    ASSERT_TRUE(Object(v3).isBoolean());
    ASSERT_TRUE(Object(v3).getBoolean());

    rapidjson::Value v4;
    ASSERT_TRUE(Object(v4).isNull());

    rapidjson::Document doc;
    rapidjson::Value v5(rapidjson::kArrayType);
    v5.PushBack(rapidjson::Value(5).Move(), doc.GetAllocator());
    v5.PushBack(rapidjson::Value(10).Move(), doc.GetAllocator());
    Object o5(v5);

    ASSERT_TRUE(o5.isArray());
    ASSERT_EQ(2, o5.size());
    ASSERT_FALSE(o5.empty());
    ASSERT_EQ(5, o5.at(0).getInteger());

    rapidjson::Value v6(rapidjson::kObjectType);
    v6.AddMember("name", rapidjson::Value("Pat").Move(), doc.GetAllocator());
    v6.AddMember("firstname", rapidjson::Value("Siva").Move(), doc.GetAllocator());
    Object o6(v6);

    ASSERT_TRUE(o6.isMap());
    ASSERT_EQ(2, o6.size());
    ASSERT_FALSE(o6.empty());
    ASSERT_STREQ("Siva", o6.get("firstname").getString().c_str());
    ASSERT_FALSE(o6.has("surname"));
}

TEST(ObjectTest, Color)
{
    Object o = Object(Color(Color::RED));
    ASSERT_TRUE(o.isColor());
    ASSERT_EQ(Color::RED, o.asColor());

    o = Object(Color::RED);
    ASSERT_TRUE(o.isNumber());
    ASSERT_EQ(Color::RED, o.asColor());

    o = Object("red");
    ASSERT_TRUE(o.isString());
    ASSERT_EQ(Color::RED, o.asColor());

    o = Object::NULL_OBJECT();
    ASSERT_TRUE(o.isNull());
    ASSERT_EQ(Color::TRANSPARENT, o.asColor());

    class TestSession : public Session {
    public:
        void write(const char *filename, const char *func, const char *value) override {
            count++;
        }

        int count = 0;
    };

    auto session = std::make_shared<TestSession>();
    o = Object("blue");
    ASSERT_EQ(Color::BLUE, o.asColor(session));
    ASSERT_EQ(0, session->count);

    o = Object("splunge");
    ASSERT_EQ(Color::TRANSPARENT, o.asColor(session));
    ASSERT_EQ(1, session->count);
}

// TDOO: Dimension unit tests

TEST(ObjectTest, Gradient)
{
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    rapidjson::Value colorRange(rapidjson::kArrayType);
    colorRange.PushBack("red", allocator);
    colorRange.PushBack("blue", allocator);
    doc.AddMember("colorRange", colorRange, allocator);
    doc.AddMember("type", "radial", allocator);

    auto context = Context::create(Metrics().size(1024,800), makeDefaultSession());

    Object a = Gradient::create(*context, doc);

    ASSERT_TRUE(a.isGradient());
    ASSERT_EQ(Gradient::RADIAL, a.getGradient().getType());
    ASSERT_EQ(0xff0000ff, a.getGradient().getColorRange().at(0).get());

    Object b(a);
    ASSERT_TRUE(b.isGradient());
    ASSERT_EQ(Gradient::RADIAL, b.getGradient().getType());
    ASSERT_EQ(0xff0000ff, b.getGradient().getColorRange().at(0).get());

    Object c;
    c = a;
    ASSERT_TRUE(c.isGradient());
    ASSERT_EQ(Gradient::RADIAL, c.getGradient().getType());
    ASSERT_EQ(0xff0000ff, c.getGradient().getColorRange().at(0).get());

    {
        rapidjson::Document doc2;
        doc2.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc2.GetAllocator();

        rapidjson::Value colorRange(rapidjson::kArrayType);
        colorRange.PushBack("blue", allocator);
        colorRange.PushBack("green", allocator);
        doc2.AddMember("colorRange", colorRange, allocator);
        doc2.AddMember("type", "linear", allocator);

        auto p = Gradient::create(*context, doc2);
        c = p;
    }

    ASSERT_TRUE(c.isGradient());
    ASSERT_EQ(Gradient::LINEAR, c.getGradient().getType());
    ASSERT_EQ(0x0000ffff, c.getGradient().getColorRange().at(0).get());

    b = c;
    ASSERT_TRUE(b.isGradient());
    ASSERT_EQ(Gradient::LINEAR, b.getGradient().getType());
    ASSERT_EQ(0x0000ffff, b.getGradient().getColorRange().at(0).get());

    // Make sure a has not changed
    ASSERT_TRUE(a.isGradient());
    ASSERT_EQ(Gradient::RADIAL, a.getGradient().getType());
    ASSERT_EQ(0xff0000ff, a.getGradient().getColorRange().at(0).get());
}

const char *BAD_CASES =
    "{"
    "  \"badType\": {"
    "    \"type\": \"fuzzy\","
    "    \"colorRange\": ["
    "      \"red\","
    "      \"green\""
    "    ]"
    "  },"
    "  \"tooShort\": {"
    "    \"type\": \"linear\","
    "    \"colorRange\": ["
    "      \"red\""
    "    ]"
    "  },"
    "  \"mismatchedRange\": {"
    "    \"type\": \"radial\","
    "    \"colorRange\": ["
    "      \"red\","
    "      \"blue\","
    "      \"green\","
    "      \"purple\""
    "    ],"
    "    \"inputRange\": ["
    "      0,"
    "      0.5,"
    "      1"
    "    ]"
    "  },"
    "  \"rangeOutOfBounds\": {"
    "    \"type\": \"linear\","
    "    \"colorRange\": ["
    "      \"red\","
    "      \"blue\""
    "    ],"
    "    \"inputRange\": ["
    "      0,"
    "      1.2"
    "    ]"
    "  },"
    "  \"rangeOutOfBounds2\": {"
    "    \"type\": \"linear\","
    "    \"colorRange\": ["
    "      \"red\","
    "      \"blue\""
    "    ],"
    "    \"inputRange\": ["
    "      -0.3,"
    "      1.0"
    "    ]"
    "  },"
    "  \"rangeMisordered\": {"
    "    \"type\": \"linear\","
    "    \"colorRange\": ["
    "      \"red\","
    "      \"blue\""
    "    ],"
    "    \"inputRange\": ["
    "      1,"
    "      0"
    "    ]"
    "  }"
    "}";


TEST(ObjectTest, MalformedGradient)
{
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(BAD_CASES);
    ASSERT_TRUE(ok);

    auto context = Context::create(Metrics().size(1024,800), makeDefaultSession());

    for (auto& m : doc.GetObject()) {
        Object result = Gradient::create(*context, m.value);
        ASSERT_TRUE(result.isNull()) << " Failed on test " << m.name.GetString();
    }
}

TEST(ObjectTest, Rect)
{
    Object a = Object(Rect(0,10,100,200));
    ASSERT_TRUE(a.isRect());
    auto r = a.getRect();
    ASSERT_EQ(0, r.getX());
    ASSERT_EQ(10, r.getY());
    ASSERT_EQ(100, r.getWidth());
    ASSERT_EQ(200, r.getHeight());
}

const static char *SCALE =
    "["
    "  {"
    "    \"scale\": 3"
    "  }"
    "]";

TEST(ObjectTest, Transform)
{
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(SCALE);
    ASSERT_TRUE(ok);

    auto context = Context::create(Metrics().size(1024,800), makeDefaultSession());

    auto transform = Transformation::create(*context, arrayify(context, Object(doc)));

    Object a = Object(transform);
    ASSERT_TRUE(a.isTransform());
    ASSERT_EQ(Point(-20,-20), a.getTransformation()->get(20,20) * Point());
}

TEST(ObjectTest, Transform2)
{
    Object a = Object(Transform2D::rotate(90));
    ASSERT_TRUE(a.isTransform2D());
    ASSERT_EQ(Transform2D::rotate(90), a.getTransform2D());
}

TEST(ObjectTest, Easing)
{
    Object a = Object(Easing::linear());
    ASSERT_TRUE(a.isEasing());
    ASSERT_EQ(0.5, a.getEasing()(0.5));

    auto session = makeDefaultSession();
    a = Object(Easing::parse(session, "ease"));
    ASSERT_TRUE(a.isEasing());
    ASSERT_NEAR(0.80240017, a.getEasing()(0.5), 0.0001);
}

TEST(ObjectTest, Radii)
{
    Object a = Object(Radii());
    ASSERT_EQ(Object::EMPTY_RADII(), a);
    ASSERT_TRUE(a.getRadii().isEmpty());

    Object b = Object(Radii(4));
    ASSERT_TRUE(b.isRadii());
    ASSERT_EQ(4, b.getRadii().bottomLeft());
    ASSERT_EQ(4, b.getRadii().bottomRight());
    ASSERT_EQ(4, b.getRadii().topLeft());
    ASSERT_EQ(4, b.getRadii().topRight());
    ASSERT_FALSE(b.getRadii().isEmpty());

    Object c = Object(Radii(1,2,3,4));
    ASSERT_TRUE(c.isRadii());
    ASSERT_EQ(1, c.getRadii().topLeft());
    ASSERT_EQ(2, c.getRadii().topRight());
    ASSERT_EQ(3, c.getRadii().bottomLeft());
    ASSERT_EQ(4, c.getRadii().bottomRight());
    ASSERT_EQ(1, c.getRadii().radius(Radii::kTopLeft));
    ASSERT_EQ(2, c.getRadii().radius(Radii::kTopRight));
    ASSERT_EQ(3, c.getRadii().radius(Radii::kBottomLeft));
    ASSERT_EQ(4, c.getRadii().radius(Radii::kBottomRight));
    ASSERT_EQ(Radii(1,2,3,4), c.getRadii());
    ASSERT_NE(Radii(1,2,3,5), c.getRadii());
    ASSERT_FALSE(c.getRadii().isEmpty());

    auto foo = std::array<float, 4>{1,2,3,4};
    ASSERT_EQ(foo, c.getRadii().get());
}

// NOTE: These test cases assume a '.' decimal separator.
//       Different locales will behave differently
static const std::vector<std::pair<double, std::string>> DOUBLE_TEST {
    { 0, "0" },
    { -1, "-1"},
    { 1, "1"},
    { 123451, "123451"},
    { 2147483647, "2147483647"},  // Largest 32 bit signed integer
    { 10000000000, "10000000000"}, // Larger than 32 bit integer
    { 1234567890123, "1234567890123"}, // Really big
    { -2147483648, "-2147483648"},  // Smallest 32 bit signed integer
    { -10000000000, "-10000000000"}, // Smaller than 32 bit integer
    { -1234567890123, "-1234567890123"}, // Really small
    { 0.5, "0.5"},
    { -0.5, "-0.5"},
    { 0.0001, "0.0001"},
    { -0.0001, "-0.0001"},
    { 0.050501010101, "0.050501"},
    { 0.199999999999, "0.2"},  // Should round up appropriately
//    { 0.000000001, "0"},   // TODO: This formats in scientific notation, which we don't handle
//    { -0.000000001, "0"},  // TODO: This formats in scientific notation, which we don't handle
};

TEST(ObjectTest, DoubleConversion)
{
    for (const auto& m : DOUBLE_TEST) {
        Object object(m.first);
        std::string result = object.asString();
        ASSERT_EQ(m.second, result) << m.first << " : " << m.second;
    }
}