#ifndef BLINKY_H
#define BLINKY_H

struct StatusLed {
    explicit StatusLed(int pin, int short_ms, int long_ms, int gap_ms, int separator_ms) : pin(pin), short_ms(short_ms),
                                                                                           long_ms(long_ms),
                                                                                           gap_ms(gap_ms),
                                                                                           separator_ms(separator_ms) {
        pinMode(pin, OUTPUT);
    }

    uint8_t add_pattern(int shorts, int longs) {
        if (pattern_count >= 10) {
            return -1;
        }
        this->patterns[pattern_count++] = {shorts, longs};
    }

    void set_pattern(uint8_t pattern) {
        if (pattern < 0 || pattern >= pattern_count) {
            this->current_pattern = -1;
            return;
        }
        this->current_pattern = pattern;

        // force switch
        last_segment_switch = 0;
        currentState = SegmentType::Short;
        lastState = SegmentType::Separator;
        current_segment_len = 0;
    }

    void tick() {
        uint64_t current_time = millis();
        if(last_segment_switch + current_segment_len > current_time) {
            return;
        }

        if(current_pattern == -1) {
            current_segment_len = 200;  // invalid state is 200ms off 200ms on
            last_segment_switch = current_time;
            if(currentState == SegmentType::Short) {
                currentState = SegmentType::Gap;
                digitalWrite(pin, LOW);
            } else {
                currentState = SegmentType::Short;
                digitalWrite(pin, HIGH);
            }
        }else {
            Pattern &pattern = this->patterns[current_pattern];
            if (currentState == SegmentType::Short) {
                segment_loop_counter++;

                if(segment_loop_counter == pattern.shorts && pattern.longs == 0) {
                    currentState = SegmentType::Separator;
                    current_segment_len = separator_ms;
                    last_segment_switch = current_time;
                    lastState = SegmentType::Short;
                    digitalWrite(pin, LOW);
                }else {
                    currentState = SegmentType::Gap;
                    current_segment_len = gap_ms;
                    last_segment_switch = current_time;
                    digitalWrite(pin, LOW);
                    lastState = SegmentType::Short;
                }
            }else if(currentState == SegmentType::Gap) {
                if (segment_loop_counter == pattern.shorts && lastState == SegmentType::Short) {
                    // switch to longs
                    currentState = SegmentType::Long;
                    current_segment_len = long_ms;
                    last_segment_switch = current_time;
                    digitalWrite(pin, HIGH);
                    lastState = SegmentType::Gap;
                    segment_loop_counter = 0;
                } else if(lastState == SegmentType::Short) {
                    // we have remaining shorts
                    currentState = SegmentType::Short;
                    current_segment_len = short_ms;
                    last_segment_switch = current_time;
                    digitalWrite(pin, HIGH);
                    lastState = SegmentType::Gap;
                } else {
                    // we have remaining longs
                    currentState = SegmentType::Long;
                    current_segment_len = long_ms;
                    last_segment_switch = current_time;
                    digitalWrite(pin, HIGH);
                    lastState = SegmentType::Gap;
                }
            } else if(currentState == SegmentType::Long) {
                segment_loop_counter++;
                if(segment_loop_counter == pattern.longs) {
                    currentState = SegmentType::Separator;
                    current_segment_len = separator_ms;
                    last_segment_switch = current_time;
                    lastState = SegmentType::Long;
                    digitalWrite(pin, LOW);
                }else {
                    currentState = SegmentType::Gap;
                    current_segment_len = gap_ms;
                    last_segment_switch = current_time;
                    digitalWrite(pin, LOW);
                    lastState = SegmentType::Long;
                }
            } else if(currentState == SegmentType::Separator) {
                if(pattern.shorts > 0) {
                    currentState = SegmentType::Short;
                    current_segment_len = short_ms;
                    last_segment_switch = current_time;
                    digitalWrite(pin, HIGH);
                    lastState = SegmentType::Separator;
                    segment_loop_counter = 0;
                }else {
                    currentState = SegmentType::Long;
                    current_segment_len = long_ms;
                    last_segment_switch = current_time;
                    digitalWrite(pin, HIGH);
                    lastState = SegmentType::Separator;
                    segment_loop_counter = 0;
                }
            }
        }
    }


private:
    int pin;
    int short_ms;
    int long_ms;
    int gap_ms;
    int separator_ms;

    struct Pattern {
        int shorts, longs;
    };

    Pattern patterns[10]{};
    int pattern_count = 0;
    int current_pattern = -1;

    // timing
    uint64_t last_segment_switch = 0;
    uint64_t current_segment_len = 0;

    enum class SegmentType {
        Short,
        Long,
        Gap,
        Separator
    };
    SegmentType currentState = SegmentType::Short;
    SegmentType lastState = SegmentType::Separator;
    int segment_loop_counter = 0;
};

#endif