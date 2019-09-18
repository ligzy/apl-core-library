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

#include <apl/engine/contextdependant.h>
#include "apl/engine/arrayify.h"
#include "apl/graphic/graphic.h"
#include "apl/utils/log.h"
#include "apl/engine/propdef.h"
#include "apl/component/vectorgraphiccomponent.h"

namespace apl {

const bool DEBUG_GRAPHIC = false;

/**************************************************************************/

GraphicPtr
Graphic::create(const ContextPtr& context,
                const GraphicContentPtr& json,
                Properties&& properties,
                const StyleInstancePtr& styledPtr)
{
    return create(context, json->get(), std::move(properties), styledPtr);
}

GraphicPtr
Graphic::create(const ContextPtr& context,
                const rapidjson::Value& json,
                Properties&& properties,
                const StyleInstancePtr& styledPtr)
{
    LOG_IF(DEBUG_GRAPHIC) << "Creating graphic data=" << context->opt("data").toDebugString();

    auto graphic = std::make_shared<Graphic>(context, json);
    graphic->initialize(context, json, std::move(properties), styledPtr);
    return graphic;
}

Graphic::Graphic(const ContextPtr& context, const rapidjson::Value& json)
    : mInternalContext(Context::createClean(context)),
      mParameterArray(json)
{
    // Put in some dummy values.  This will allow internal GraphicElements to set up dependant relationships
    mInternalContext->putSystemWriteable("width", 100);
    mInternalContext->putSystemWriteable("height", 100);
}

/*
 * Some notes on how we hook up context and properties and parameters
 *
 * The VectorGraphic component has a context, a style, a parameter array, and a list of assigned properties
 * The Graphic has an internal context which is used to inflate the graphic elements and a parameter list.
 *
 * The internal context has a "width", "height", and one entry for each named PARAMETER.
 * The internal context uses GraphicDependant objects to connect context changes to the GraphicElement.
 *
 * The values of the parameters come from the following sources:
 *
 * 1. If the parameter appears in the list of assigned properties, it explicitly assigned.
 *    The assignment may be a constant value or it may be a data-binding expression that depends on some
 *    upstream data-binding contexts.
 *
 * 2. Otherwise, if the parameter appears in the STYLE assigned to the VectorGraphic, then the parameter
 *    value is copied from the style.  As the style changes the parameter changes.
 *
 * 3. If all else fails the parameter picks up the default parameter value.
 *
 * When the parameter is assigned a data-binding value, a ContextDependant object makes the connection
 * between the context where the dependancy is defined, the context where the expression will be evaluated
 * (that is, in the VectorGraphic context), and the context where the newly calculated value will be stored
 * (the internal context).
 */
void
Graphic::initialize(const ContextPtr& sourceContext,
                    const rapidjson::Value& json,
                    Properties&& properties,
                    const StyleInstancePtr& styledPtr)
{
    // Populate the data-binding context with parameters
    for (const auto& param : mParameterArray) {
        LOG_IF(DEBUG_GRAPHIC) << "Parse parameter: " << param.name;
        const auto& conversionFunc = sBindingFunctions.at(param.type);
        auto value = conversionFunc(*sourceContext, evaluate(*sourceContext, param.defvalue));
        Object parsed;

        // Check if there is an assigned property
        auto it = properties.find(param.name);
        if (it != properties.end()) {
            mAssigned.emplace(param.name);  // Mark this as an assigned property

            // If the assigned property is a string, check for data-binding
            if (it->second.isString()) {
                parsed = parseDataBinding(mInternalContext, it->second.getString());
                value = conversionFunc(*sourceContext, evaluate(*sourceContext, parsed));
            }
            else {
                value = conversionFunc(*sourceContext, evaluate(*sourceContext, it->second));
            }
        }
        else if (styledPtr) {  // Look for a styled value
            auto itStyle = styledPtr->find(param.name);
            if (itStyle != styledPtr->end())
                value = conversionFunc(sourceContext, itStyle->second);
        }

        // Store the calculated value in the data-binding context
        LOG_IF(DEBUG_GRAPHIC) << "Storing parameter '" << param.name << "' = " << value;
        mInternalContext->putUserWriteable(param.name, value);

        // After storing the parameter we can wire up any necessary data dependant
        if (parsed.isNode()) {
            std::set<std::string> symbols;
            parsed.symbols(symbols);
            auto self = std::static_pointer_cast<Graphic>(shared_from_this());
            for (const auto& symbol : symbols) {
                auto upstream = sourceContext->findContextContaining(symbol);
                if (upstream != nullptr)
                    ContextDependant::create(upstream, symbol,
                                             mInternalContext, param.name,
                                             sourceContext,   // The evaluation context is NOT the target context
                                             parsed, conversionFunc);
            }
        }

    }

    auto self = std::static_pointer_cast<Graphic>(shared_from_this());
    mRootElement = GraphicElement::build(self, mInternalContext, json);
}

bool
Graphic::setProperty(const std::string& key, const apl::Object& value)
{
    for (const auto& param : mParameterArray) {
        if (param.name == key) {
            mInternalContext->userUpdateAndRecalculate(key, value, true);
            mAssigned.emplace(key);
            return true;
        }
    }

    return false;
}

static double calculateScale(double scale, GraphicScale scaleType) {
    switch (scaleType) {
        case kGraphicScaleGrow:
            return scale > 1.0 ? scale : 1.0;
        case kGraphicScaleShrink:
            return scale < 1.0 ? scale : 1.0;
        case kGraphicScaleStretch:
             return scale;
        default:
            return 1.0;
    }
}

double
Graphic::getIntrinsicHeight() const
{
    return mRootElement ? mRootElement->getValue(kGraphicPropertyHeightOriginal).getAbsoluteDimension() : 0;
}

double
Graphic::getIntrinsicWidth() const
{
    return mRootElement ? mRootElement->getValue(kGraphicPropertyWidthOriginal).getAbsoluteDimension() : 0;
}

double
Graphic::getViewportWidth() const
{
    return mRootElement ? mRootElement->getValue(kGraphicPropertyViewportWidthActual).getDouble() : 0;
}

double
Graphic::getViewportHeight() const
{
    return mRootElement ? mRootElement->getValue(kGraphicPropertyViewportHeightActual).getDouble() : 0;
}

bool
Graphic::layout(double width, double height, bool useDirtyFlag) {
    if (!mRootElement)
        return false;

    // First, check to see if the stored "actual" width and height are being changed
    double widthActual = mRootElement->getValue(kGraphicPropertyWidthActual).getAbsoluteDimension();
    double heightActual = mRootElement->getValue(kGraphicPropertyHeightActual).getAbsoluteDimension();

    if (widthActual == width && heightActual == height)
        return false;

    // Okay, they've been changed.  Store the new values
    mRootElement->setValue(kGraphicPropertyWidthActual, Dimension(width), useDirtyFlag);
    mRootElement->setValue(kGraphicPropertyHeightActual, Dimension(height), useDirtyFlag);

    // Retrieve the original dimensions, viewport dimensions, and scaling factors
    double widthOriginal = mRootElement->getValue(kGraphicPropertyWidthOriginal).getAbsoluteDimension();
    double heightOriginal = mRootElement->getValue(kGraphicPropertyHeightOriginal).getAbsoluteDimension();
    double viewportWidthOriginal = mRootElement->getValue(kGraphicPropertyViewportWidthOriginal).getDouble();
    double viewportHeightOriginal = mRootElement->getValue(kGraphicPropertyViewportHeightOriginal).getDouble();
    auto scaleWidth = static_cast<GraphicScale>(mRootElement->getValue(kGraphicPropertyScaleTypeWidth).getInteger());
    auto scaleHeight = static_cast<GraphicScale>(mRootElement->getValue(kGraphicPropertyScaleTypeHeight).getInteger());

    // Calculate the updated viewport size
    double viewportWidthNew = viewportWidthOriginal * calculateScale( width / widthOriginal, scaleWidth );
    double viewportHeightNew = viewportHeightOriginal * calculateScale( height / heightOriginal, scaleHeight );

    // Retrieve the most recently set viewport size
    double viewportWidthActual = mRootElement->getValue(kGraphicPropertyViewportWidthActual).getDouble();
    double viewportHeightActual = mRootElement->getValue(kGraphicPropertyViewportHeightActual).getDouble();

    // If the viewport size has changed, store the new values and recalculate the entire graphic
    if (viewportWidthNew != viewportWidthActual || viewportHeightNew != viewportHeightActual) {
        mRootElement->setValue(kGraphicPropertyViewportWidthActual, viewportWidthNew, useDirtyFlag);
        mRootElement->setValue(kGraphicPropertyViewportHeightActual, viewportHeightNew, useDirtyFlag);
        mInternalContext->systemUpdateAndRecalculate("height", viewportHeightNew, useDirtyFlag);
        mInternalContext->systemUpdateAndRecalculate("width", viewportWidthNew, useDirtyFlag);
    }

    // If we've reached this point, we know that at least one of width or height change, so we're dirty.
    if (useDirtyFlag)
        addDirtyChild(mRootElement);

    return true;
}

bool
Graphic::updateStyle(const StyleInstancePtr& styledPtr)
{
    // Walk the list of parameters.  If the parameter is NOT in mAssigned, then
    // it can change based on style.
    bool changed = false;

    for (const auto& m : mParameterArray) {
        if (!mAssigned.count(m.name)) { // Not in the assigned set - try styling
            auto newValue = m.defvalue;
            auto itStyle = styledPtr->find(m.name);
            if (itStyle != styledPtr->end())
                newValue = sBindingFunctions.at(m.type)(mInternalContext, itStyle->second);

            if (mInternalContext->opt(m.name) != newValue) {  // Ah - there's a change
                mInternalContext->userUpdateAndRecalculate(m.name, newValue, true);
                changed = true;
            }
        }
    }

    return changed;
}

void
Graphic::clearDirty()
{
    for (const auto& element : mDirty)
        element->clearDirtyProperties();
    mDirty.clear();
}

void
Graphic::addDirtyChild(const GraphicElementPtr& child)
{
    if (mDirty.emplace(child).second) {
        auto component = mComponent.lock();
        if (component)
            component->setDirty(kPropertyGraphic);
    }
}

rapidjson::Value
Graphic::serialize(rapidjson::Document::AllocatorType& allocator) const {
    using rapidjson::Value;
    Value v(rapidjson::kObjectType);
    v.AddMember("isValid", isValid(), allocator);
    v.AddMember("intrinsicWidth", getIntrinsicWidth(), allocator);
    v.AddMember("intrinsicHeight", getIntrinsicHeight(), allocator);
    v.AddMember("viewportWidth", getViewportWidth(), allocator);
    v.AddMember("viewportHeight", getViewportHeight(), allocator);
    v.AddMember("root", getRoot()->serialize(allocator), allocator);
    return v;
}

} // namespace apl