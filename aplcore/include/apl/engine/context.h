/*
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

#ifndef _APL_CONTEXT_H
#define _APL_CONTEXT_H

#include <memory>
#include <string>
#include <exception>
#include <memory>
#include <map>
#include <yoga/Yoga.h>

#include "apl/common.h"
#include "apl/primitives/object.h"
#include "apl/engine/jsonresource.h"
#include "apl/engine/recalculatesource.h"
#include "apl/engine/recalculatetarget.h"
#include "apl/engine/styleinstance.h"
#include "apl/utils/path.h"
#include "apl/engine/contextobject.h"

namespace apl {

class Metrics;
class Styles;
class RootContextData;
class State;
class Event;
class Sequencer;
class RootConfig;
class FocusManager;
class HoverManager;

class KeyboardManager;

/*
 * The data-binding context holds information about the local environment, metrics, and resources.
 * Context objects should be heap-allocated with a shared pointer to their parent context.
 */
class Context : public RecalculateTarget<std::string>,
                public RecalculateSource<std::string>,
                public std::enable_shared_from_this<Context> {
public:
    /**
     * Create a context that is the child of another context.
     * @param parent The parent context.
     * @return The child context.
     */
    static ContextPtr create(const ContextPtr& parent) {
        return std::make_shared<Context>(parent);
    }

    /**
     * Create a top-level context for testing. Do not use this for non-testing code
     * @param metrics Display metrics
     * @param session The logging session
     * @return The context
     */
    static ContextPtr create(const Metrics& metrics, const SessionPtr& session);

    /**
     * Create a top-level context for testing. Do not use this for production.
     * @param metrics Display metrics.
     * @param config Root configuration
     * @return The context
     */
    static ContextPtr create(const Metrics& metrics, const RootConfig& config);

    /**
     * Create a top-level context for the document background extraction.
     * @param metrics Display metrics.
     * @param config Root configuration
     * @param theme Theme
     * @return The context
     */
    static ContextPtr create(const Metrics& metrics, const RootConfig& config, const std::string& theme);

    /**
     * Create a top-level context.  Only used by RootContext
     * @param metrics Display metrics
     * @param core Internal core data.
     * @return The context.
     */
    static ContextPtr create(const Metrics& metrics,
                                           const std::shared_ptr<RootContextData>& core);

    /**
     * Create a "clean" context.  This shares the same root data, but does not contain any
     * of the built content.  It does contain the top-level resources.
     * Used when creating clean data-binding contexts for graphics.
     * @param other The context to build from.
     * @return The context.
     */
    static ContextPtr createClean(const ContextPtr& other);

    /**
     * Construct a child context. Do not call this method directly; use the ::create() method instead.
     * @param parent The parent of this context.
     */
    Context(const ContextPtr& parent)
        : mParent(parent), mTop(parent->top() ? parent->top() : parent), mCore(parent->mCore) {}

    /**
     * Construct a free-standing context.  Do not call this directly; use the ::create method instead
     * @param metrics The display metrics.
     * @param core A pointer to the common core data.
     */
    Context(const Metrics& metrics, const std::shared_ptr<RootContextData>& core);

    /**
     * Standard destructor
     */
    ~Context();

    /**
     * Look up a value in the context.  If the value doesn't exist, return null.
     * @param key The string name to look up.
     * @return The value or null.
     */
    Object opt(const std::string& key) const {
        auto it = mMap.find(key);
        if (it != mMap.end())
            return it->second.value();

        if (mParent)
            return mParent->opt(key);

        return Object::NULL_OBJECT();
    }

    /**
     * Check to see if a value exists in the context.
     * @param key The string name to look up.
     * @return True if the value is defined somewhere in this context or an ancestor context.
     */
    bool has(const std::string& key) const {
        auto it = mMap.find(key);
        if (it != mMap.end())
            return true;

        if (mParent)
            return mParent->has(key);

        return false;
    }

    /**
     * Check if a key exists somewhere in the context chain as an immutable value.
     * @param key The string name to look up.
     * @return True if the value is defined in the context chain and is immutable
     */
    bool hasImmutable(const std::string& key) const {
        auto it = mMap.find(key);
        if (it != mMap.end())
            return !it->second.isMutable();

        if (mParent)
            return mParent->hasImmutable(key);

        return false;
    }

    /**
     * Find the first context containing a specific key.
     * @param key The key to search for.
     * @return The first context or nullptr if one can't be found.
     */
    ContextPtr findContextContaining(const std::string& key) {
        auto it = mMap.find(key);
        if (it != mMap.end())
            return shared_from_this();

        if (mParent)
            return mParent->findContextContaining(key);

        return nullptr;
    }

    /**
     * Propagate a changed value in the context.  This can only be called if the value already exists. Updating
     * a value will also cause all dependants of this value to be updated.  This method should only be called
     * by an upstream dependant
     * @param key The string key name
     * @param value The new value to assign
     * @param useDirtyFlag If true, mark downstream changes as dirty
     * @return True if the key name exists in this context; false if there is no binding value with this name.
     */
    bool propagate(const std::string& key, const Object& value, bool useDirtyFlag) {
        auto it = mMap.find(key);
        if (it == mMap.end())
            return false;

        if (it->second.set(value))
            recalculateDownstream(key, useDirtyFlag);

        return true;
    }

    /**
     * Write a value in the current context.  This only works for user-writeable values.
     * It will fail if the value does not already exist.  This method will search in parent
     * contexts if the value is not found in the current context.
     * @param key The string key name.
     * @param value The value to store.
     * @param useDirtyFlag If true, mark changes downstream with a dirty flag
     * @return True if the key already exists in this context (it may not be changed)
     */
    bool userUpdateAndRecalculate(const std::string& key, const Object& value, bool useDirtyFlag);

    /**
     * Mutate a value in the current context.  This only works for user- and system-writeable values.
     * It will fail if the value does not already exist.  This method ONLY searches in the current context.
     * @param key The string key name.
     * @param value The value to store.
     * @param useDirtyFlag If true, mark changes downstream with a dirty flag
     * @return True if the key already exists in this context (it may not be changed)
     */
    bool systemUpdateAndRecalculate(const std::string& key, const Object& value, bool useDirtyFlag);

    /**
     * Store a value in the current context.  If the value already exists in the current
     * context, nothing is written.  The value is stored as a fixed property and may not be changed.
     * @param key The string key name.
     * @param value The value to store.
     */
    void putConstant(const std::string& key, const Object& value)
    {
        mMap.emplace(key, ContextObject(value));
    }

    /**
     * Store a value in the current context.  If the value already exists in the current
     * context, nothing is written.  The value may be changed with the userUpdateAndRecalculate method.
     * User-writeable value include component "bind" properties, "parameters" from a layout inflation,
     * and graphic "parameters".
     *
     * @param key The string key name
     * @param value The value to store.
     */
    void putUserWriteable(const std::string& key, const Object& value)
    {
        mMap.emplace(key, ContextObject(value).userWriteable());
    }

    /**
     * Store a value in the current context and mark it as mutable.  If the value already exists in the
     * current context, nothing is written.  The value may be changed with the systemUpdateAndRecalculate
     * method.  System-writeable values include the "width"/"height" properties assigned to a graphic
     * during layout.
     * @param key The string key name
     * @param value The value to store
     */
    void putSystemWriteable(const std::string& key, const Object& value)
    {
        mMap.emplace(key, ContextObject(value).systemWriteable());
    }

    /**
     * Store a resource and provenance path data in the current context.
     * Resources are allowed to overwrite an existing resource with the same name.
     *
     * @param key The string key name
     * @param value The value to store
     * @param path The path data to associate with this key
     * @return True if the key already exists in this context.
     */
    void putResource(const std::string& key, const Object& value, const Path& path) {
        // Toss away a resource if it already exists (we overwrite it)
        auto it = mMap.find(key);
        if (it != mMap.end())
            mMap.erase(it);

        mMap.emplace(key, ContextObject(value).provenance(path));
    }

    /**
     * Return the provenance associated with this key.
     * @param key The string key name
     * @return The provenance path data, or an empty string if it can't be found
     */
    std::string provenance(const std::string& key) const {
        // The provenance for a key can only be used if the current map has that key entry
        auto it = mMap.find(key);
        if (it != mMap.end())
            return it->second.provenance().toString();

        if (mParent)
            return mParent->provenance(key);

        return "";
    }

    /**
     * Check if a value is mutable
     * @param key The string key name
     * @return True if the value is mutable.
     */
    bool isMutable(const std::string& key) const {
        auto it = mMap.find(key);
        if (it != mMap.end())
            return it->second.isMutable();

        if (mParent)
            return mParent->isMutable(key);

        return false;
    }

    /**
     * @return An iterator to the beginning of defined bindings
     */
    std::map<std::string, ContextObject>::const_iterator begin() const { return mMap.begin(); }

    /**
     * @return An iterator to the end of the defined bindings
     */
    std::map<std::string, ContextObject>::const_iterator end() const { return mMap.end(); }

    /**
     * @return The parent of this context or nullptr if there is no parent
     */
    std::shared_ptr<const Context> parent() const { return mParent; }
    ContextPtr parent() { return mParent; }

    /**
     * @return The top context for data evaluation
     */
    std::shared_ptr<const Context> top() const { return mTop ? mTop : shared_from_this(); }
    ContextPtr top() { return mTop ? mTop : shared_from_this(); }

    /**
     * Convert "vw" units to "dp"
     * @param vw The amount of vw units
     * @return The corresponding amount in dp
     */
    double vwToDp(double vw) const;

    /**
     * Convert "vh" units to "dp"
     * @param vh The amount of vh units
     * @return The corresponding amount in dp
     */
    double vhToDp(double vh) const;

    /**
     * Convert pixel units to dp
     * @param px The amount of px units
     * @return The corresponding amount in dp
     */
    double pxToDp(double px) const;

    /**
     * @return The height of the viewport in dp
     */
    double width() const;

    /**
     * @return The width of the viewport in dp
     */
    double height() const;

    /**
     * @return The root configuration provided by the viewhost
     */
    const RootConfig& getRootConfig() const;

    friend streamer& operator<<(streamer& os, const Context& context);

    /**
     * Lookup and return a named layout
     * @param name The name of the layout
     * @return The JSON resource defining this layout.  May be null if the layout cannot be found
     */
    const JsonResource getLayout(const std::string& name) const;

    /**
     * Lookup and return a style by name
     * @param name The name of the style
     * @param state The operating state to use when evaluating the style.
     * @return An instance of the style or nullptr if the style cannot be found.
     */
    StyleInstancePtr getStyle(const std::string& name, const State& state);

    /**
     * Lookup and return a named command
     * @param name The name of the command
     * @return The JSON resource defining this command.  May be null if the command cannot be found.
     */
    const JsonResource getCommand(const std::string& name) const;

    /**
     * Lookup and return a graphic by name.
     * @param name The name of the graphic.
     * @return The JSON resource defining this graphic.  May be null if the graphic cannot be found
     */
    const JsonResource getGraphic(const std::string& name) const;

    /**
     * Find a component somewhere in the DOM with the given id or uniqueId.
     * @param id The id or uniqueID to search for.
     * @return The component or nullptr if it is not found.
     */
    ComponentPtr findComponentById(const std::string& id) const;

    /**
     * @return The current theme
     */
    std::string getTheme() const;

    /**
     * @return The APL version requested by the document
     */
    std::string getRequestedAPLVersion() const;

    /**
     * Internal routine used by components to mark themselves as changed.
     * @param id The id of the component.
     */
    void setDirty(const ComponentPtr& ptr);
    void clearDirty(const ComponentPtr& ptr);

    void pushEvent(Event&& event);

    Sequencer& sequencer() const;
    FocusManager& focusManager() const;
    HoverManager& hoverManager() const;

    KeyboardManager& keyboardManager() const;

    const SessionPtr& session() const;

    YGConfigRef ygconfig() const;

    const TextMeasurementPtr& measure() const;

    void takeScreenLock() const;
    void releaseScreenLock() const;

    /**
     * Inflate raw JSON into a component.  This method assumes the presence of a document and
     * inflates the JSON using the layouts, resources, and styles defined by that document.
     * @param component The raw JSON component definition.
     * @return The inflated component or nullptr if the JSON is invalid.
     */
    ComponentPtr inflate(const rapidjson::Value& component);

protected:
    ContextPtr mParent;
    ContextPtr mTop;
    std::shared_ptr<RootContextData> mCore;
    std::map<std::string, ContextObject> mMap;
};

}  // namespace apl

#endif // _APL_CONTEXT_H
