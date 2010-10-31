#include "driverfeedback.h"

#include <string>

#include <iostream>

DRIVERFEEDBACK::DRIVERFEEDBACK()
{
    message_show = false;
}

void DRIVERFEEDBACK::Update(float dt)
{
    if (!message_show)
        return;

    animation_timer += dt;

    if ((animation_timer >= fadein_starttime) && (animation_timer < message_starttime))
    {
        // fading in
        float fadein_elapsed = animation_timer - fadein_starttime;
        float p = fadein_elapsed / fadein_duration; //fadein_percent
        message_alpha = 3 * p * p - 2 * p * p * p;
        //std::cout << "dt: " << dt << ", fadein alpha: " << message_alpha << std::endl;
    }
    else if ((animation_timer >= message_starttime) && (animation_timer < fadeout_starttime))
    {
        // displaying
        message_alpha = 1.0;
        //std::cout << "dt: " << dt << ", message display alpha: " << message_alpha << std::endl;
    }
    else if ((animation_timer >= fadeout_starttime) && (animation_timer < (fadeout_starttime + fadeout_duration)))
    {
        // fading out
        float fadeout_elapsed = animation_timer - fadeout_starttime;
        float p = 1.0 - fadeout_elapsed / fadeout_duration; //fadeout_percent
        message_alpha = 3 * p * p - 2 * p * p * p;
        //std::cout << "dt: " << dt << ", fadeout alpha: " << message_alpha << std::endl;
    }
    else
    {
        // all done, turn off
        message_alpha = 0.0;
        message_show = false;
    }

}

void DRIVERFEEDBACK::SetNewFeedback(const std::string & new_message_text, float new_message_duration,
                                    float new_fadein_duration, float new_fadeout_duration,
                                    float r, float g, float b)
{
    message_text = new_message_text;
    message_duration = new_message_duration;
    fadein_starttime = 0.0;
    fadein_duration = new_fadein_duration;
    message_starttime = fadein_starttime + fadein_duration;
    message_duration = new_message_duration;
    fadeout_starttime = message_starttime + message_duration;
    fadeout_duration = new_fadeout_duration;
    message_r = r;
    message_g = g;
    message_b = b;
    message_alpha = 0.0;
    message_show = true;
    animation_timer = 0.0;
}


