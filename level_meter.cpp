/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include "level_meter.h"

#include <algorithm>

#include "Linear2Db.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

namespace level_meter
{

#define DMA_IRQ_x __CONCAT(DMA_IRQ_, PICO_LEVEL_METER_DMA_IRQ)

static constexpr uint PIN_ADC_BASE    = 26;  // determined by rp2040 (don't change this)
static constexpr int  ADC_BUF_LEN     = 10 * NUM_ADC_CH;

uint     dma_chan[2];  // double buffer
int      buf_idx = 0;
uint16_t dma_buf[2][ADC_BUF_LEN];
int      dma_irq_count = 0;

dma_channel_config cfg[2];

static constexpr int ADC_BITS = 12;
static constexpr float ADC_CALIB_A = 1.05;
static constexpr float ADC_CALIB_B = - 0.025;

// dB level conversion
std::vector<float> dbScale{-20, -15, -10, -6, -4, -2, 0, 1, 2, 6, 8};
level_meter::Linear2Db linear2Db(NUM_ADC_CH, dbScale);

static queue_t _level_queue;
static constexpr uint LEVEL_QUEUE_LENGTH = 4;
typedef struct _level_item_t {
    int id;
    int level[NUM_ADC_CH];
} level_item_t;

// prototype declaration
void __isr __time_critical_func(level_meter_dma_irq_handler)();

void init()
{
    // ADC setup
    for (int i = 0; i < NUM_ADC_CH; i++) {
        adc_gpio_init(PIN_ADC_BASE + PIN_ADC_OFFSET + i);
    }

    adc_init();

    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        false    // Not shift each sample to 8 bits when pushing to FIFO
    );

    adc_set_clkdiv(96*500);  // sampling rate 1 KHz (48MHz x cycle)
    adc_set_round_robin(((1 << NUM_ADC_CH) - 1) << PIN_ADC_OFFSET);  // set bits to used

    sleep_ms(100);

    // Set up the DMA to start transferring data as soon as it appears in FIFO
    dma_chan[0] = dma_claim_unused_channel(true);
    dma_chan[1] = dma_claim_unused_channel(true);
    for (int i = 0; i < 2; i++) {
        cfg[i] = dma_channel_get_default_config(dma_chan[i]);
        channel_config_set_transfer_data_size(&cfg[i], DMA_SIZE_16);
        // Reading from constant address, writing to incrementing byte addresses
        channel_config_set_read_increment(&cfg[i], false);
        channel_config_set_write_increment(&cfg[i], true);
        channel_config_set_dreq(&cfg[i], DREQ_ADC); // Pace transfers based on availability of ADC samples
        channel_config_set_chain_to(&cfg[i], dma_chan[1 - i]);
    }

    // DMA IRQ
    if (!irq_has_shared_handler(DMA_IRQ_x)) {
        irq_add_shared_handler(DMA_IRQ_x, level_meter_dma_irq_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    }
    dma_irqn_set_channel_enabled(PICO_LEVEL_METER_DMA_IRQ, dma_chan[0], true);
    dma_irqn_set_channel_enabled(PICO_LEVEL_METER_DMA_IRQ, dma_chan[1], true);
    irq_set_enabled(DMA_IRQ_x, true);

    for (int i = 0; i < 2; i++) {
        dma_channel_configure(dma_chan[i], &cfg[i],
            dma_buf[i],  // dst
            &adc_hw->fifo,   // src
            ADC_BUF_LEN,   // transfer count
            false            // start immediately
        );
    }

    // Queue setup
    queue_init(&_level_queue, sizeof(level_item_t), LEVEL_QUEUE_LENGTH);
}
void start()
{

    // Start DMA
    dma_channel_start(dma_chan[buf_idx]);

    adc_select_input(PIN_ADC_OFFSET);  // start round-robin
    adc_run(true);
}

bool get_level(int level[NUM_ADC_CH])
{
    if (queue_get_level(&_level_queue) > 0) {
        level_item_t levelItem;
        queue_remove_blocking(&_level_queue, &levelItem);
        for (int i = 0; i < NUM_ADC_CH; i++) {
            level[i] = levelItem.level[i];
        }
        return true;
    }
    return false;
}

void stop()
{
    // Once DMA finishes, stop any new conversions from starting, and clean up
    // the FIFO in case the ADC was still mid-conversion.
    adc_run(false);
    adc_fifo_drain();
}

// irq handler for DMA
void __isr __time_critical_func(level_meter_dma_irq_handler)()
{
    int irq_buf_idx;

    for (irq_buf_idx = 0; irq_buf_idx < 2; irq_buf_idx++) {
        if (dma_irqn_get_channel_status(PICO_LEVEL_METER_DMA_IRQ, dma_chan[irq_buf_idx])) {
            dma_irqn_acknowledge_channel(PICO_LEVEL_METER_DMA_IRQ, dma_chan[irq_buf_idx]);
            break;
        }
    }
    if (irq_buf_idx >= 2) { return; }

    float norm[NUM_ADC_CH];
    for (int i = 0; i < NUM_ADC_CH; i++) {
        // insert keeping sorted
        std::vector<uint16_t> buf;
        for (int j = 0; j < ADC_BUF_LEN; j += NUM_ADC_CH) {
            uint16_t val = dma_buf[irq_buf_idx][j + i];
            auto it = std::upper_bound(buf.cbegin(), buf.cend(), val);
            buf.insert(it, val);
        }
        // pick center samples and average
        const int start = ADC_BUF_LEN / 2 / 4;
        const int end   = ADC_BUF_LEN / 2 * 3 / 4;
        uint16_t sum = 0;
        for (int j = start; j < end; j++) {
            sum += buf[j];
        }
        // nomalize
        norm[i] = (float) sum / (end - start) / (1 << ADC_BITS);
        // calibration
        norm[i] = ADC_CALIB_A * norm[i] + ADC_CALIB_B;
    }

    unsigned int level[NUM_ADC_CH];
    linear2Db.getLevel(norm, level);
    level_item_t levelItem;
    levelItem.id = dma_irq_count;
    for (int i = 0; i < NUM_ADC_CH; i++) {
        levelItem.level[i] = level[i];
    }
    queue_try_add(&_level_queue, &levelItem);

    dma_channel_configure(dma_chan[irq_buf_idx], &cfg[irq_buf_idx],
        dma_buf[irq_buf_idx],  // dst
        &adc_hw->fifo,         // src
        ADC_BUF_LEN,           // transfer count
        false                  // start immediately
    );

    dma_irq_count++;
}
}