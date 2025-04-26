#pragma once

#include <vector>
#include <driver/i2s_std.h>

#include "esphome/components/microphone/microphone.h"
#include "esphome/core/component.h"

namespace esphome {
namespace fetap {

/*
    The fetap microphone class implements a basic I2S microphone component based on the new I2S driver.
*/
class FetapMicrophone : public microphone::Microphone, public Component {
public:
    
    /* --------------------------- Functions inherited from component interface --------------------------- */
    
    /*
        Called initially to configure the I2S peripheral and allocate buffers
    */
    void setup(void) override;

    /*
        Called repeatedly, implements a rudimentary state machine
    */
    void loop(void) override;

    /* --------------------------- Functions inherited from microphone interface --------------------------- */

    /*
        Requests to start the I2S peripheral for the microphone
    */
    void start(void) override;

    /*
        Request to stop the I2S peripheral for the microphone
    */
    void stop(void) override;

    /*
        Read a maximum of len bytes from the I2S peripheral into buf

        \param  buf     Pointer to audio buffer which should be filled with data
        \param  len     Maximum number of int32_t samples that can be written into buf

        \returns    The number of bytes read into the audio buffer
    */
    size_t read(int16_t *buf, size_t len) override;

    /* --------------------------- Functions triggered from code generation --------------------------- */
    
    /*
        Sets the DIN pin of I2S bus

        \param  pin     The DIN GPIO pin of the I2S bus
    */
    void set_din_pin(int pin) { din_pin_ = static_cast<gpio_num_t>(pin); }
    
    /*
        Sets the BLCK pin of I2S bus

        \param  pin     The BLCK GPIO pin of the I2S bus
    */
    void set_bclk_pin(int pin) { bclk_pin_ = static_cast<gpio_num_t>(pin); }
    
    /*
        Sets the LRCLK/WS pin of I2S bus

        \param  pin     The LRCLK/WS GPIO pin of the I2S bus
    */
    void set_lrclk_pin(int pin) { lrclk_pin_ = static_cast<gpio_num_t>(pin); }

private:
    static constexpr uint16_t kBufferSize{512}; /*!<    Number of int16 samples the buffer can hold. 
                                                        Since the same buffer is used for the raw i2s 
                                                        data (int32) and output data (int16), the read 
                                                        method will return at maximum kBufferSize/2 
                                                        int16 audio samples in a single call.*/
    
    static constexpr uint16_t kMaxI2SReadTimeoutMilliseconds{100}; /*!< Maximum timeout when reading from I2S peripheral */

    /*
        Starts the I2S peripheral
    */
    void start_(void);

    /*
        Stops the I2S peripheral
    */
    void stop_(void);

    /*
        Reads from the I2S peripheral
    */
    void read_(void);

    std::vector<int16_t> buffer_; /*!< Buffer for processed audio data */
    std::vector<uint32_t> raw_i2s_buffer_; /*!< Buffer for raw audio data */
    gpio_num_t din_pin_{I2S_GPIO_UNUSED}; /*!< DIN pin of I2S bus */
    gpio_num_t bclk_pin_{I2S_GPIO_UNUSED}; /*!< BCLK pin of the I2S bus */
    gpio_num_t lrclk_pin_{I2S_GPIO_UNUSED}; /*!< LRCLK/WS pin of the I2S bus */
    i2s_chan_handle_t i2s_rx_channel_; /*!< Channel handle of I2S peripheral */
    HighFrequencyLoopRequester high_freq_; /*!< Speed up frequency at which loop is called */
};

}

}