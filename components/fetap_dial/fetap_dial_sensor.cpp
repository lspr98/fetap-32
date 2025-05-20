#include "fetap_dial_sensor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace fetap {

static const char *const TAG = "fetap.dialsensor";
static const size_t TASK_STACK_SIZE = 2048;
static const ssize_t TASK_PRIORITY = 22;

static TimerHandle_t timer_handle;
static SemaphoreHandle_t rotary_dial_sampling_semaphore;
static TickType_t t_rotary_dial_interrupt;
static bool dialing_timeout_flag = false;

static void IRAM_ATTR rotary_dial_sensor_isr_handler(void* arg) {
    // Unlock task loop if we are currently not sampling
    if(t_rotary_dial_interrupt == 0) {
        t_rotary_dial_interrupt = xTaskGetTickCountFromISR();
        xSemaphoreGiveFromISR(rotary_dial_sampling_semaphore, NULL);
    }
}

static void timer_publish_handler(TimerHandle_t timer) {
    dialing_timeout_flag = true;
    xSemaphoreGive(rotary_dial_sampling_semaphore);
}

void FetapDialSensor::setup() {
    esp_err_t err;

    err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error starting ISR service: %s", esp_err_to_name(err));
        mark_failed();
        status_set_error();
        return;
    }

    // Configure the DIAL pin as input with internal pull-up which triggers an interrupt on rising edges
    // Due to the way the rotary dial is wired up, the signal will be LOW when the rotary dial contact
    // is closed (default state when nothing is dialed) and the signal will be HIGH when the rotary dial 
    // contact is open (happens in short pulses when the dial is spinning back into position).
    const gpio_config_t sensor_pin_cfg {
        .pin_bit_mask = static_cast<uint64_t>(1) << dial_pin_,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };

    err = gpio_config(&sensor_pin_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error configuring dial pin: %s", esp_err_to_name(err));
        mark_failed();
        status_set_error();
        return;
    }

    // Register interrupt handler for sensor pin
    err = gpio_isr_handler_add(dial_pin_, rotary_dial_sensor_isr_handler, (void*) dial_pin_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error adding ISR handler: %s", esp_err_to_name(err));
        mark_failed();
        status_set_error();
        return;
    }

    // Create semaphore to synchronize task with interrupts from sensor pin
    rotary_dial_sampling_semaphore = xSemaphoreCreateBinary();
    xSemaphoreTake(rotary_dial_sampling_semaphore, 0);

    if (dial_timeout_) {
        timer_handle = xTimerCreate("dial_publish_timer", pdMS_TO_TICKS(dial_timeout_), pdFALSE, (void *) 0, timer_publish_handler);
    }

    // Start dial task
    xTaskCreate(FetapDialSensor::dial_task, "fetapdial_task", TASK_STACK_SIZE, (void *) this, TASK_PRIORITY,
                &task_handle_);

    ESP_LOGI(TAG, "Fetap Dial Task initialized successfully.");
}

void FetapDialSensor::dial_task(void *params) {
    FetapDialSensor * instance = static_cast<FetapDialSensor *>(params);

    while(1) {
        instance->task_loop();
    }
}

void FetapDialSensor::task_loop(void) {
    // Set last interrupt time to 0 to signal that we are currently not sampling
    t_rotary_dial_interrupt = 0;
    
    // Wait until user starts dialing a number or a timeout is triggered from a
    // previously dialed number. If a number was dialed, this will trigger the 
    // registered interrupt since the DIAL pin will be pulled up as soon as the
    // rotary dial contact opens. The interrupt will set t_rotary_dial_interrupt
    // to the time where the positive edge of the first impulse was detected and
    // unlock the semaphore so that the task can start with sampling.
    xSemaphoreTake(rotary_dial_sampling_semaphore, portMAX_DELAY);

    if (dialing_timeout_flag) {
        // User stopped dialing in digits. Publish the complete number.
        publish_number();
    } else {
        // User dialed a new digit. Sample the DIAL pin to determine the digit that was dialed
        sample_rotary_dial();
    }
    
    // Wait minimum time until next number can be dialed in
    vTaskDelay(pdMS_TO_TICKS(kNumberDialGapMilliseconds));
}

void FetapDialSensor::sample_rotary_dial(void) {
    const uint16_t tick_delay_closed{pdMS_TO_TICKS(kPulseClosedMilliseconds)};
    const uint16_t tick_delay_sample{pdMS_TO_TICKS(kSampleDelayMilliseconds)};

    // Initialize counter with -1 to detect if no pulses were detected
    int8_t counter{-1};
    for(; counter < 10; counter++) {
        bool pin_was_high{false};
        
        // Sample the DIAL pin kSamplesPerPulse time while we expect the contact to
        // be open (= pin is pulled HIGH through internal pull-up). We need to sample
        // multiple times per pulse as the mechanical contacts of over 50 year old
        // telephones might not be in the best shape and provide an unreliable signal.
        // At least one sample needs to be high to confirm the presence of a pulse.
        for (uint8_t sample_idx = 0; sample_idx < kSamplesPerPulse; sample_idx++) {
            pin_was_high |= gpio_get_level(dial_pin_);
            xTaskDelayUntil(&t_rotary_dial_interrupt, tick_delay_sample);
        }

        // All samples returned a closed contact -> there was no pulse, so don't
        // expect any follow-up pulses.
        if(!pin_was_high) {
            break;
        }

        // Time frame in which the contact is supposed to be open is finshed,
        // the conact will be closed for a short amount of time in which we
        // don't need to do anything.
        xTaskDelayUntil(&t_rotary_dial_interrupt, tick_delay_closed);
    }

    if (counter < 0) {
        // No pulses detected
        return;
    }

    // Convert pulses to the dialed digit as UTF-8 character
    const char dialed_digit = ((counter + 1) % 10) + 0x30;

    // Add new digit to number
    dialed_number_ += std::string(1, dialed_digit);

    if (dial_timeout_) {
        // Non-zero timeout is configured. Start timer to wait
        // for potential follow-up digits
        xTimerReset(timer_handle, 0);
    } else {
        // No timeout configured. Directly publish digit as state
        publish_number();
    }
}

void FetapDialSensor::publish_number(void) {
    // Publish dialed number as new state
    publish_state(dialed_number_);
    // Reset dialed number
    dialed_number_.clear();
    // Reset timeout flag
    dialing_timeout_flag = false;
}

}
}