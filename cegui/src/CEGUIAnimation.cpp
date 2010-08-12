/***********************************************************************
    filename:   CEGUIAnimation.cpp
    created:    7/8/2010
    author:     Martin Preisler

    purpose:    Implements the Animation class
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2010 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/
#include "CEGUIAnimation.h"
#include "CEGUIAffector.h"
#include "CEGUIExceptions.h"
#include "CEGUIAnimationInstance.h"
#include "CEGUIEventSet.h"
#include <algorithm>

// Start of CEGUI namespace section
namespace CEGUI
{
//----------------------------------------------------------------------------//
Animation::Animation(const String& name):
        d_name(name),

        d_replayMode(RM_Loop), // the right default value that confirms things are working
        d_duration(0.0f),
        d_autoStart(false)
{}

//----------------------------------------------------------------------------//
Animation::~Animation(void)
{
    while (d_affectors.size() > 0)
    {
        destroyAffector(getAffectorAtIdx(0));
    }
}

//----------------------------------------------------------------------------//
const String& Animation::getName() const
{
    return d_name;
}

//----------------------------------------------------------------------------//
void Animation::setReplayMode(ReplayMode mode)
{
    d_replayMode = mode;
}

//----------------------------------------------------------------------------//
Animation::ReplayMode Animation::getReplayMode() const
{
    return d_replayMode;
}

//----------------------------------------------------------------------------//
void Animation::setDuration(float duration)
{
    d_duration = duration;
    // todo: iterate through existing key frames if any and if we
    //       find a keyframe that is now outside of the [0, duration] interval,
    //       rant about it in the log
}

//----------------------------------------------------------------------------//
float Animation::getDuration() const
{
    return d_duration;
}

//----------------------------------------------------------------------------//
void Animation::setAutoStart(bool autoStart)
{
    d_autoStart = autoStart;
}

//----------------------------------------------------------------------------//
bool Animation::getAutoStart() const
{
    return d_autoStart;
}

//----------------------------------------------------------------------------//
Affector* Animation::createAffector(void)
{
    // no checking needed!

    Affector* ret = new Affector(this);
    d_affectors.push_back(ret);

    return ret;
}

//----------------------------------------------------------------------------//
Affector* Animation::createAffector(const String& targetProperty,
                                    const String& interpolator)
{
    Affector* ret = createAffector();
    ret->setTargetProperty(targetProperty);
    ret->setInterpolator(interpolator);

    return ret;
}

//----------------------------------------------------------------------------//
void Animation::destroyAffector(Affector* affector)
{
    AffectorList::iterator it =
        std::find(d_affectors.begin(), d_affectors.end(), affector);

    if (it == d_affectors.end())
    {
        throw InvalidRequestException(
            "Animation::destroyAffector: Given affector not found!");
    }

    d_affectors.erase(it);
}

//----------------------------------------------------------------------------//
Affector* Animation::getAffectorAtIdx(size_t index) const
{
    if (index >= d_affectors.size())
    {
        throw InvalidRequestException(
            "Animation::getAffectorAtIdx: Out of bounds.");
    }

    AffectorList::const_iterator it = d_affectors.begin();
    std::advance(it, index);

    return *it;
}

//----------------------------------------------------------------------------//
size_t Animation::getNumAffectors(void) const
{
    return d_affectors.size();
}

//----------------------------------------------------------------------------//
void Animation::defineAutoSubscription(const String& eventName,
                                       const String& action)
{
    SubscriptionMap::iterator it = d_autoSubscriptions.find(eventName);

    while (it != d_autoSubscriptions.end() && it->first == eventName)
    {
        if (it->second == action)
        {
            throw InvalidRequestException(
                "Animation::defineAutoSubscription: Unable to define "
                "given Auto Subscription - exactly the same auto subscription "
                "is already there!");
        }

        ++it;
    }

    d_autoSubscriptions.insert(std::make_pair(eventName, action));
}

//----------------------------------------------------------------------------//
void Animation::undefineAutoSubscription(const String& eventName,
                                         const String& action)
{
    SubscriptionMap::iterator it = d_autoSubscriptions.find(eventName);

    while (it != d_autoSubscriptions.end() && it->first == eventName)
    {
        if (it->second == action)
        {
            d_autoSubscriptions.erase(it);
            return;
        }

        ++it;
    }

    throw InvalidRequestException(
        "Animation::undefineAutoSubscription: Unable to undefine "
        "given Auto Subscription - not found!");
}

//----------------------------------------------------------------------------//
void Animation::undefineAllAutoSubscriptions()
{
    d_autoSubscriptions.clear();
}

//----------------------------------------------------------------------------//
void Animation::autoSubscribe(AnimationInstance* instance)
{
    EventSet* eventSender = instance->getEventSender();

    if (!eventSender)
    {
        return;
    }

    for (SubscriptionMap::const_iterator it = d_autoSubscriptions.begin();
         it != d_autoSubscriptions.end(); ++it)
    {
        const String& e = it->first;
        const String& a = it->second;

        Event::Connection connection;

        if (a == "Start")
        {
            connection = eventSender->subscribeEvent(e,
                         CEGUI::Event::Subscriber(&AnimationInstance::handleStart, instance));
        }
        else if (a == "Stop")
        {
            connection = eventSender->subscribeEvent(e,
                         CEGUI::Event::Subscriber(&AnimationInstance::handleStop, instance));
        }
        else if (a == "Pause")
        {
            connection = eventSender->subscribeEvent(e,
                         CEGUI::Event::Subscriber(&AnimationInstance::handlePause, instance));
        }
        else if (a == "Unpause")
        {
            connection = eventSender->subscribeEvent(e,
                         CEGUI::Event::Subscriber(&AnimationInstance::handleUnpause, instance));
        }
        else if (a == "TogglePause")
        {
            connection = eventSender->subscribeEvent(e,
                         CEGUI::Event::Subscriber(&AnimationInstance::handleTogglePause, instance));
        }
        else
        {
            throw InvalidRequestException(
                "Animation::autoSubscribe: Unable to auto subscribe! "
                "'" + a + "' is not a valid action.");
        }

        instance->addAutoConnection(connection);
    }
}

//----------------------------------------------------------------------------//
void Animation::autoUnsubscribe(AnimationInstance* instance)
{
    // just a delegate to make things clean
    instance->unsubscribeAutoConnections();
}

//----------------------------------------------------------------------------//
void Animation::apply(AnimationInstance* instance)
{
    for (AffectorList::const_iterator it = d_affectors.begin();
         it != d_affectors.end(); ++it)
    {
        (*it)->apply(instance);
    }
}

//----------------------------------------------------------------------------//

} // End of  CEGUI namespace section

