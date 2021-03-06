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

#ifndef _APL_ANIMATE_ITEM_ACTION_H
#define _APL_ANIMATE_ITEM_ACTION_H

#include "apl/action/action.h"
#include "apl/animation/animatedproperty.h"

namespace apl {

class CoreCommand;

/**
 * Handle running an AnimateItem command
 */
class AnimateItemAction : public Action {
public:
    static std::shared_ptr<AnimateItemAction> make(const TimersPtr& timers,
                                               const std::shared_ptr<CoreCommand>& command,
                                               bool fastMode)
    {
        auto ptr = std::make_shared<AnimateItemAction>(timers, command, fastMode);
        ptr->start();
        return ptr;
    }

    AnimateItemAction(const TimersPtr& timers,
                      const std::shared_ptr<CoreCommand>& command,
                      bool fastMode);

private:
    void start();
    void advance();
    void finalize();

private:
    std::shared_ptr<CoreCommand> mCommand;
    std::vector<std::unique_ptr<AnimatedProperty>> mAnimators;
    int mNextIndex;
    int mRepeatCounter;
    bool mReversed;
    ActionPtr mCurrentAction;
    const float mDuration;
    const int mRepeatCount;
    const int mRepeatMode;
    const bool mFastMode;
    Easing mEasing;
};


} // namespace apl

#endif //_APL_ANIMATE_ITEM_ACTION_H
