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

#ifndef _APL_VIDEO_COMPONENT_H
#define _APL_VIDEO_COMPONENT_H

#include "corecomponent.h"
#include "apl/primitives/mediastate.h"

namespace apl {


class VideoComponent : public CoreComponent {
public:
    static CoreComponentPtr create(const ContextPtr& context, Properties&& properties, const std::string& path);
    VideoComponent(const ContextPtr& context, Properties&& properties, const std::string& path);

    ComponentType getType() const override { return kComponentTypeVideo; };
    void updateMediaState(const MediaState& state, bool fromEvent) override;
    const MediaState& getMediaState() const { return mMediaState; };

    std::shared_ptr<ObjectMap> getEventTargetProperties() const override;

    bool getTags(rapidjson::Value& outMap, rapidjson::Document::AllocatorType& allocator) override;

protected:
    const ComponentPropDefSet& propDefSet() const override;
    void addEventSourceProperties(ObjectMap& event) const override;
    std::string getVisualContextType() override;

private:
    MediaState mMediaState;
};


} // namespace apl

#endif //_APL_VIDEO_COMPONENT_H
