#ifndef _DRIVERFEEDBACK_H
#define _DRIVERFEEDBACK_H

#include <string>

class DRIVERFEEDBACK
{
private:
    bool message_show;
    float message_starttime;
    float message_duration;
    std::string message_text;
    float message_r;
    float message_g;
    float message_b;
    float message_alpha;
    float fadein_starttime;
    float fadein_duration;;
    float fadeout_starttime;
    float fadeout_duration;
    float animation_timer;

public:
    DRIVERFEEDBACK();

    void Update(float dt);

    void SetNewFeedback(const std::string & new_message_text, float new_message_duration=5.0,
                        float new_fadein_duration=1.0, float new_fadeout_duration=1.0,
                        float r=0.8, float g=0.8, float b=0.8);

    bool IsFeedbackDisplayed() const
    {
        return message_show;
    }

    const std::string & GetMessageText() const
    {
        return message_text;
    }

    float GetMessageAlpha() const
    {
        return message_alpha;
    }

};

#endif
