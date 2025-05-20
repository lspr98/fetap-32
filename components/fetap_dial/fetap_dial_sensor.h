#pragma once

#include <driver/gpio.h>

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace fetap {

/*
    The fetap dial sensor is responsible for detecting if the user dialed a number
    via the rotary dial of the telephone.
*/
class FetapDialSensor : public text_sensor::TextSensor, public Component {
public:
    /* --------------------------- Functions inherited from component interface --------------------------- */

    /*
        Called initially to setup the sensor pin, register the interrupt and start the sensor task
    */
    void setup() override;

    /* --------------------------- Functions triggered from code generation --------------------------- */

    /*
        Sets the DIAL pin of the rotary dial

        \param  pin     The DIAL GPIO pin of the rotary dial
    */
    void set_dial_pin(int pin) { dial_pin_ = static_cast<gpio_num_t>(pin); }

    /*
        Sets the time to wait for another number before publishing a new state

        \param  timeout_ms  The time to wait in milliseconds
    */
    void set_dial_timeout(int timeout_ms) {dial_timeout_ = static_cast<uint32_t>(timeout_ms); }

private:

    /*
        Function that is registered as a task to run asynchronously from main loop
    */
    static void dial_task(void *params);

    /*
        Repeatedly called by the sensor task. Starts sampling the DIAL pin as soon
        as an interrupt is generated from the DIAL pin.
    */
    void task_loop(void);

    /*
        Determines the number that was dialed when called right at the start of
        the first DIAL pin pulse (detected by the interrupt). The fetap telephone 
        generates pulses on the DIAL pin with a certain timing according to the 
        IWV (Impulswahlverfahren). Since the timing is crucial for this to work,
        it is executed in a separate task context. The detected number is published
        as a character, which can be used to trigger different actions for each
        number in home assistant. 
    */
    void sample_rotary_dial(void);

    /*
        Publishes the complete dialed number
    */
    void publish_number(void);

    static constexpr uint16_t kPulseOpenMilliseconds{60}; /*!< Duration for which the sensor contact is open during each pulse */
    static constexpr uint16_t kPulseClosedMilliseconds{40}; /*!< Duration for which the sensor contact is closed during each pulse */
    static constexpr uint16_t kNumberDialGapMilliseconds{(kPulseOpenMilliseconds + kPulseClosedMilliseconds) * 2}; /*!< Minimum possible time between two consecutive dialed numbers */
    static constexpr uint16_t kSamplesPerPulse{6}; /*!< Number of times the sensor pin is sampled during the conact open period of a pulse */
    static constexpr uint16_t kSampleDelayMilliseconds{kPulseOpenMilliseconds / kSamplesPerPulse}; /*!< Delay between samples when the sensor contact is open */
    static constexpr uint32_t kDefaultDialTimeoutMilliseconds{0}; /*!< The default time to wait for another number to be dialed before publishing the new state */

    // The pulse open period needs to be divisable by the number of samples per pulse.
    static_assert(kPulseOpenMilliseconds % kSamplesPerPulse == 0);

    std::string dialed_number_{""}; /*!< String that holds the dialed number */
    uint32_t dial_timeout_{kDefaultDialTimeoutMilliseconds}; /*!< Maximum time to wait for next digit before publishing */
    gpio_num_t dial_pin_{GPIO_NUM_NC}; /*!< DIAL pin of the rotary dial */
    TaskHandle_t task_handle_{nullptr}; /*!< Reference to the sensor task */
};

}
}