## Team-1575H

Throughout the year my code had three flywheel control methods and some minor changes throughout driver control and autonomous that let us do some more fine work.
Originally we used slew rate (which we kept the stepper for all year) and some basic tuning. We had the ability to pulse a higher speed to aid recovery and let us fire faster.
Next came TBH. My original TBH was horribly clunky and had logic errors which caused us to have some issues with oscillations after about 4 shots. This is what I called TBH flawless, which was based off of Jpearman's original TBH code, but added Feedforward to reduce overshoot and had motor protection through the original slew rate code
