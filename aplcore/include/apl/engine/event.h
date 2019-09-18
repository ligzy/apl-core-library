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

#ifndef _APL_EVENT_H
#define _APL_EVENT_H

#include <memory>

#include "apl/action/action.h"
#include "apl/component/component.h"
#include "apl/primitives/objectbag.h"
#include "apl/command/commandproperties.h"

namespace apl {

class CoreCommand;
class EventData;

/**
 * Enumeration of event types
 */
enum EventType {
    /**
     * Control media
     *
     * kEventPropertyCommand: The command to execute
     * kEventPropertyValue: The integer value associated with that command
     *
     * Does not have an ActionRef.
     */
    kEventTypeControlMedia,

    /**
     * Change the focus
     *
     * Does not have an ActionRef.
     */
    kEventTypeFocus,

    /**
     * Request a URL to be opened
     *
     * kEventPropertySource: The URL to open.
     *
     * The server must resolve the ActionRef if the URL is opened.
     * The server should resolve the ActionRef with a non-zero argument
     * if the URL fails to open.
     */
    kEventTypeOpenURL,

    /**
     * Play media
     *
     * kEventPropertyAudioTrack: The audio track we should play this media on.
     * kEventPropertySource: An array of media sources
     *
     * The server must resolve the ActionRef when the audio track is set to foreground.
     * If the audio track is background or none, the ActionRef is not provided.
     */
    kEventTypePlayMedia,

    /**
     * Warn the view host that a speak event is coming.
     *
     * kEventPropertySource: The speech URI.
     *
     * Does not have an ActionRef.
     */
    kEventTypePreroll,

    /**
     * Requests the bounds information for a text component
     *
     * The component is a TextComponent that needs the first line bounds measured
     */
    kEventTypeRequestFirstLineBounds,

    /**
     * Scroll a component into view.
     *
     * The component is the component to scroll.
     * kEventPropertyPosition: The scroll position or page to change to.
     *
     * The server must resolve the ActionRef when the scroll is completed.
     */
    kEventTypeScrollTo,

    /**
     * Send an event to the server
     *
     * kEventPropertySource: The rich source object describing who raised this event.
     * kEventPropertyArguments: The argument array provided by the APL author
     * kEventPropertyComponents: The values of the components requested by the APL author
     *
     * Does not have an ActionRef
     */
    kEventTypeSendEvent,

    /**
     * Change the page in a pager.
     *
     * The component is the pager.
     * kEventPropertyPosition: The page to switch to (integer)
     * kEventPropertyDirection: The direction to move. Either kEventDirectionForward or kEventDirectionBackward
     *
     * The server must resolve the ActionRef when the scroll is completed.
     */
    kEventTypeSetPage,

    /**
     * Speak a single component.
     *
     * kEventPropertyHighlightMode: Highlight mode. kEventHighlightModeLine or kEventHighlightModeBlock
     * kEventPropertySource: The speech URI.
     *
     * The server must resolve the ActionRef when the scroll is completed.
     */
    kEventTypeSpeak,
};

enum EventProperty {
    kEventPropertyAlign,
    kEventPropertyArguments,
    kEventPropertyAudioTrack,
    kEventPropertyCommand,
    kEventPropertyComponent,
    kEventPropertyComponents,
    kEventPropertyDirection,
    kEventPropertyHighlightMode,
    kEventPropertyPosition,
    kEventPropertySource,
    kEventPropertyValue,
};

enum EventDirection {
    kEventDirectionForward = 0,
    kEventDirectionBackward = 1,
};

enum EventHighlightMode {
    kEventHighlightModeLine = CommandHighlightMode::kCommandHighlightModeLine,
    kEventHighlightModeBlock = CommandHighlightMode::kCommandHighlightModeBlock
};

enum EventAudioTrack {
    kEventAudioTrackBackground = AudioTrack::kAudioTrackBackground,
    kEventAudioTrackForeground = AudioTrack::kAudioTrackForeground,
    kEventAudioTrackNone = AudioTrack::kAudioTrackNone
};

enum EventControlMediaCommand {
    kEventControlMediaPlay = CommandControlMedia::kCommandControlMediaPlay,
    kEventControlMediaPause = CommandControlMedia::kCommandControlMediaPause,
    kEventControlMediaNext = CommandControlMedia::kCommandControlMediaNext,
    kEventControlMediaPrevious = CommandControlMedia::kCommandControlMediaPrevious,
    kEventControlMediaRewind = CommandControlMedia::kCommandControlMediaRewind,
    kEventControlMediaSeek = CommandControlMedia::kCommandControlMediaSeek,
    kEventControlMediaSetTrack = CommandControlMedia::kCommandControlMediaSetTrack
};

extern Bimap<int, std::string> sEventTypeBimap;
extern Bimap<int, std::string> sEventPropertyBimap;

using EventBag = ObjectBag<sEventPropertyBimap>;

/**
 * This class represents a single event sent from APL core to the native rendering layer.
 */
class Event : public UserData {
public:
    /**
     * Construct an asynchronous event
     * @param eventType The type of the event
     * @param bag Data associated with this event
     */
    Event(EventType eventType, EventBag&& bag);

    /**
     * Construct an asynchronous event
     * @param eventType The type of the event
     * @param component The component associated with this event
     */
    Event(EventType eventType, const ComponentPtr& component);

    /**
     * Construct an asynchronous event
     * @param eventType The type of the event
     * @param bag Data associated with this event
     * @param component The component associated with this event
     */
    Event(EventType eventType, EventBag&& bag, const ComponentPtr& component);

    /**
     * Construct a synchronous event.
     * @param eventType The type of the event
     * @param component The component associated with this event
     * @param actionRef The action reference for resolution.
     */
    Event(EventType eventType, const ComponentPtr& component, ActionRef actionRef);

    /**
     * Construct a synchronous event.
     * @param eventType The type of the event
     * @param bag Data associated with this event
     * @param component The component associated with this event
     * @param actionRef The action reference for resolution.
     */
    Event(EventType eventType, EventBag&& bag, const ComponentPtr& component, ActionRef actionRef);

    /**
     * @return The type of the event
     */
    EventType getType() const;

    /**
     * Retrieve a value from the event.
     * @param key The key to retrieve.
     * @return The value or null if it doesn't exist.
     */
    const Object getValue(EventProperty key) const;

    /**
     * The component associated with this event.  For the ScrollToIndex command this
     * is the actual component that the index points to.  In all other commands it is
     * what the componentId points to.
     *
     * @return The component associated with this event.
     */
    ComponentPtr getComponent() const;

    /**
     * @return The current action reference for resolution.  Will be null for asynchronous commands.
     */
    ActionRef getActionRef() const;

    /**
     * Serialize this event into a JSON object
     * @param allocator
     * @return
     */
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& allocator) const;

    /**
     * Equality test.  Used primarily by unit testing code; this does not guarantee
     * that two events are exactly the same, but does check to make sure they look
     * "approximately" the same
     * @param rhs The event to compare to
     * @return True if the two events match
     */
    bool matches(const Event& rhs) const;

private:
    std::shared_ptr<const EventData> mData;
};

}  // namespace apl

#endif //_APL_EVENT_H
