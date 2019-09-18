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

#include "apl/utils/session.h"
#include "apl/engine/arrayify.h"
#include "apl/command/arraycommand.h"
#include "apl/command/commandfactory.h"
#include "apl/engine/evaluate.h"
#include "apl/command/corecommand.h"

namespace apl {

const bool DEBUG_COMMAND_FACTORY = false;

CommandFactory* CommandFactory::sInstance = nullptr;

CommandFactory&
CommandFactory::instance()
{
    if (!sInstance) {
        sInstance = new CommandFactory();
        sInstance->reset();
    }
    return *sInstance;
}

void
CommandFactory::reset()
{
    mCommandMap.clear();
    auto it = sCommandNameBimap.beginBtoA();
    for (; it != sCommandNameBimap.endBtoA(); ++it)
        mCommandMap.emplace(it->first, sCommandCreatorMap.at(it->second));
}

CommandFactory&
CommandFactory::set(const char *name, CommandFunc func)
{
    mCommandMap[name] = func;   // Allow overrwriting
    return *this;
}

CommandFunc
CommandFactory::get(const char *name) const
{
    auto it = mCommandMap.find(name);
    if (it != mCommandMap.end())
        return it->second;
    return nullptr;
}

/**
 * Evaluate the command and return an action.  This may return a nullptr!
 * @param context The data-binding context.
 * @param command The JSON representation of the command
 * @param base The base component that started this action.  May be null.
 * @param fastMode If the command should run in fast mode.
 * @return An action pointer or nullptr if there is nothing to execute.
 */
ActionPtr
CommandFactory::execute(const TimersPtr& timers,
                        const ContextPtr& context,
                        const Object& command,
                        const CoreComponentPtr& base,
                        bool fastMode)
{
    CommandPtr ptr = inflate(context, command, base);
    if (!ptr)
        return nullptr;
    return ptr->execute(timers, fastMode);
}

/**
 * Inflate macro command.
 * @param context The data-binding context.
 * @param properties Properties passed in from outside.
 * @param definition The definition of the command macro (should be a map).
 * @param base The base component in which the command was defined.
 * @return The inflated command or nullptr if it is invalid.
 */
CommandPtr
CommandFactory::expandMacro(const ContextPtr& context,
                            Properties& properties,
                            const rapidjson::Value& definition,
                            const CoreComponentPtr& base) {
    assert(definition.IsObject());

    LOG_IF(DEBUG_COMMAND_FACTORY) << "Expanding macro";

    // Build a new context for this command macro
    ContextPtr cptr = Context::create(context);

    // Add each parameter to the data-binding context and remove
    // the matching named property that was passed in.
    ParameterArray params(definition);
    for (const auto& param : params) {
        LOG_IF(DEBUG_COMMAND_FACTORY) << "Parsing parameter: " << param.name;
        cptr->putConstant(param.name, properties.forParameter(*cptr, param));
    }

    return ArrayCommand::create(cptr,
                                arrayifyProperty(cptr, definition, "command", "commands"),
                                base,
                                properties
    );
}


/**
 * Expand the JSON definition of a command into a command object.
 * @param context The context in which the command should be expanded.
 * @param command The command definition.  Should be a map.
 * @param base The base component in which the command was defined.
 * @return The inflated command or nullptr if it is invalid (or has a false "when' clause)
 */
CommandPtr
CommandFactory::inflate(const ContextPtr& context,
                        const Object& command,
                        const Properties& properties,
                        const CoreComponentPtr& base)
{
    if (!command.isMap())
        return nullptr;

    auto type = propertyAsString(*context, command, "type");
    if (type.empty()) {
        CONSOLE_CTP(context) << "Invalid type in command";
        return nullptr;
    }

    bool when = propertyAsBoolean(*context, command, "when", true);
    if (!when)
        return nullptr;

    // Copy object properties into the properties.  This will skip the "when" and "type" keys
    Properties props = properties;
    props.emplace(command);

    // If this is a primitive type, use that logic to expand.
    auto method = mCommandMap.find(type);
    if (method != mCommandMap.end())
        return method->second(context, std::move(props), base);

    // Look up a command macro.
    const auto& resource = context->getCommand(type);
    if (!resource.empty())
        return expandMacro(context, props, resource.json(), base);

    CONSOLE_CTP(context) << "Unable to find primitive or macro command '" << type << "'";
    return nullptr;
}

/**
 * Expand the JSON definition of a command into a command object.
 * @param context The context in which the command should be expanded.
 * @param command The command definition.  Should be a map.
 * @param base The base component in which the command was defined.
 * @param fastMode True if the command is executing in fast mode.
 * @return The inflated command or nullptr if it is invalid.
 */
CommandPtr
CommandFactory::inflate(const ContextPtr& context,
                        const Object& command,
                        const CoreComponentPtr& base)
{
    Properties properties;
    return inflate(context, command, properties, base);
}

} // namespace apl