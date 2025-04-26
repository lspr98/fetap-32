#pragma once

#include <vector>
#include <driver/i2s_std.h>

#include "esphome/components/speaker/speaker.h"
#include "esphome/core/component.h"

namespace esphome {
namespace fetap {

/*
    The fetap speaker class implements a basic I2S speaker component based on the new I2S driver.
*/
class FetapSpeaker : public speaker::Speaker, public Component {
public:

    /*
        Possible operation states of the fetap speaker.
    */
    enum class State : uint8_t {
        STOPPED, /*!< speaker I2S driver stopped */
        STOPPING, /*!< speaker I2S driver is running but requested to stop */
        RUNNING, /*!< speaker I2S driver is running */
        STARTING, /*!< speaker I2S driver is stopped but requested to start */
    };

    /* --------------------------- Functions inherited from component interface --------------------------- */
    
    /*
        Called initially to configure the I2S peripheral and allocate buffers
    */
    void setup(void) override;

    /*
        Called repeatedly, implements a rudimentary state machine
    */
    void loop(void) override;

    /* --------------------------- Functions inherited from speaker interface --------------------------- */

    /*
        Requests to start the I2S peripheral for the speaker
    */
    void start(void) override;

    /*
        Request to stop the I2S peripheral for the speaker
    */
    void stop(void) override;

    /*
        The fetap speaker does not use a ring buffer and forwards all data straight to the I2S peripheral.
        Hence, it never buffers data.

        \returns    False, as the fetap speaker implementation does not buffer audio data.
    */
    bool has_buffered_data() const override { return false; };

    /*
        Plays the given audio data.

        \param  data            Pointer to the audio data buffer to be played. The audio data format is mono channel, 16kHz sampling 
                                rate and each sample is an int16_t (2 bytes per sample).
        \param  length          The number of bytes in the audio data buffer.
        \param  ticks_to_wait   The maximum number of ticks (NOT milliseconds) to wait for the I2S audio data to be written.

        \returns    The number of bytes that were successfully "played" (=written to the I2S peripheral).
    */
    size_t play(const uint8_t *data, size_t length, TickType_t ticks_to_wait) override { return write_(data, length, ticks_to_wait); };

    /*
        Plays the given audio data.

        \param  data            Pointer to the audio data buffer to be played. The audio data format is mono channel, 16kHz sampling 
                                rate and each sample is an int16_t (2 bytes per sample).
        \param  length          The number of bytes in the audio data buffer.

        \returns    The number of bytes that were successfully "played" (=written to the I2S peripheral).
    */
    size_t play(const uint8_t *data, size_t length) override { return write_(data, length); };

    /* --------------------------- Functions triggered from code generation --------------------------- */
    
    /*
        Sets the DOUT pin of I2S bus

        \param  pin     The DOUT GPIO pin of the I2S bus
    */
    void set_dout_pin(int pin) { dout_pin_ = static_cast<gpio_num_t>(pin); }
    
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
    static constexpr uint16_t kMaxI2SDefaultWriteTimeoutTicks{100}; /*!< Default maximum timeout when writing to I2S peripheral */
    static constexpr uint8_t kAudioGainShift{4}; /*!<   Number of right shifts for audio samples to control loudness.
                                                        The resulting gain factor is 1 / (2^kAudioGainShift) */

    /*
        Starts the I2S peripheral
    */
    void start_(void);

    /*
        Stops the I2S peripheral
    */
    void stop_(void);

    /*
        Writes the given data to the I2S peripheral.

        \param  data            Pointer to the audio data buffer to be played. The audio data format is mono channel, 16kHz sampling 
                                rate and each sample is an int16_t (2 bytes per sample).
        \param  length          The number of bytes in the audio data buffer.
        \param  ticks_to_wait   The maximum number of ticks (NOT milliseconds) to wait for the I2S audio data to be written. Defaults
                                to kMaxI2SDefaultWriteTimeoutTicks

        \returns    The number of bytes that were successfully "played" (=written to the I2S peripheral).
    */
    size_t write_(const uint8_t* const data, const size_t length, const TickType_t ticks_to_wait = kMaxI2SDefaultWriteTimeoutTicks);

    gpio_num_t dout_pin_{I2S_GPIO_UNUSED}; /*!< DOUT pin of I2S bus */
    gpio_num_t bclk_pin_{I2S_GPIO_UNUSED}; /*!< BCLK pin of I2S bus */
    gpio_num_t lrclk_pin_{I2S_GPIO_UNUSED}; /*!< LRCLK/WS pin of I2S bus */
    i2s_chan_handle_t i2s_tx_channel_; /*!< Channel handle of I2S peripheral */
    std::vector<int16_t> buffer_; /*!< Audio buffer used to manipulate audio before writing to I2S peripheral */
    State state_{State::STOPPED}; /*!< Current state of the fetap speaker */
};

}

}