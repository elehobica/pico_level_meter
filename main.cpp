/*------------------------------------------------------/
/ Copyright (c) 2023, Elehobica
/ Released under the BSD-2-Clause
/ refer to https://opensource.org/licenses/BSD-2-Clause
/------------------------------------------------------*/

#include <cstdio>

#include "main.h"
#include "Linear2Db.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#define DMA_IRQ_x __CONCAT(DMA_IRQ_, PICO_LEVEL_METER_DMA_IRQ)

static constexpr uint PIN_ADC_BASE    = 26;
static constexpr uint PIN_ADC_OFFSET0 = 0;
static constexpr uint PIN_ADC_OFFSET1 = 1;
static constexpr int  ADC_BUF_LEN     = 20;

uint     dma_chan[2];
int      buf_idx = 0;
uint16_t dma_buf[2][ADC_BUF_LEN];

dma_channel_config cfg[2];

static constexpr int ADC_BITS = 12;
static constexpr float ADC_CALIB_A = 1.05;
static constexpr float ADC_CALIB_B = - 0.025;

// dB level meter
std::vector<float> dbScale{-20, -15, -10, -6, -4, -2, 0, 1, 2, 6, 8};
level_meter::Linear2Db linear2Db(2, dbScale);

// prototype declaration
void __isr __time_critical_func(level_meter_dma_irq_handler)();

int main()
{
    stdio_init_all();

    adc_gpio_init(PIN_ADC_BASE + PIN_ADC_OFFSET0);
    adc_gpio_init(PIN_ADC_BASE + PIN_ADC_OFFSET1);

    adc_init();

    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        false    // Not shift each sample to 8 bits when pushing to FIFO
    );

    adc_set_clkdiv(96*500);  // sampling rate 1 KHz (48MHz x cycle)
    adc_set_round_robin((1 << PIN_ADC_OFFSET0) | (1 << PIN_ADC_OFFSET1));

    printf("Arming DMA\n");
    sleep_ms(1000);

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

    // Start DMA
    dma_channel_start(dma_chan[buf_idx]);

    printf("Starting capture\n");
    adc_select_input(PIN_ADC_OFFSET0);  // start round-robin
    adc_run(true);

    /*
    float in[2] = {1.0, 0.00169};
    int out[2] = {};

    linear2Db.getLevel(in, out);

    printf("%d %d\n", out[0], out[1]);

    return 0;
    */
    while (true)  {}

    sleep_ms(100);

    // Once DMA finishes, stop any new conversions from starting, and clean up
    // the FIFO in case the ADC was still mid-conversion.
    printf("Capture finished\n");
    adc_run(false);
    adc_fifo_drain();

    return 0;
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

    float norm[2];
    for (int i = 0; i < 2; i++) {
        // insert keeping sorted
        std::vector<uint16_t> buf;
        for (int j = 0; j < ADC_BUF_LEN; j += 2) {
            uint16_t val = dma_buf[irq_buf_idx][j + i];
            auto it = std::lower_bound(buf.cbegin(), buf.cend(), val);
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

    unsigned int level[2];
    linear2Db.getLevel(norm, level);
    printf("level %d %d\n", (int) level[0], (int) level[1]);

    dma_channel_configure(dma_chan[irq_buf_idx], &cfg[irq_buf_idx],
        dma_buf[irq_buf_idx],  // dst
        &adc_hw->fifo,         // src
        ADC_BUF_LEN,           // transfer count
        false                  // start immediately
    );
}