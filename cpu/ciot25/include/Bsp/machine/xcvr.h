/*********************************************************************************
 * This confidential and proprietary software may be used only as authorized 
 *                      by a licensing agreement from                           
 *                           Cortus S.A.S.
 *
 *                 (C) Copyright 2004-2019 Cortus S.A.S.
 *                        ALL RIGHTS RESERVED
 *
 * The entire notice above must be reproduced on all authorized copies
 * and any such reproduction must be pursuant to a licensing agreement 
 * from Cortus S.A.S. (http://www.cortus.com)
 *
 * $CortusRelease$
 * $FileName$
 *
 *********************************************************************************/

#ifndef _XCVR_H
#define _XCVR_H
#include <machine/sfradr.h>

#define HAV_OP_MODE
typedef enum {

    NETOPT_STATE_STANDBY = 0,       /**< standby mode. The devices is awake but
                                     *   not listening to packets. */

    NETOPT_STATE_TX ,               /**< transmit mode,
                                     *   set: triggers transmission of a preloaded packet
                                     *   (see @ref NETOPT_PRELOADING*). The resulting
                                     *   state of the network device is @ref NETOPT_STATE_IDLE
                                     *   get: the network device is in the process of
                                     *   transmitting a packet */

    NETOPT_STATE_RX ,               /**< receive mode,
                                     *   the device currently receives a packet */

    NETOPT_STATE_BYPASS_TX_MANUAL_MODULATION,

    NETOPT_STATE_BYPASS_RX_MANUAL_DEMODULATION,

    NETOPT_STATE_RX_DIRECT,

    NETOPT_STATE_TX_FIFO,

    NETOPT_STATE_RX_FIFO,
    /* add other states if needed */
} netopt_state_t;


typedef struct Xcvr
{ // Cleared with IPG mode or with specified condition
  /* Modem Status, r
   *      24:    Bypass Tx done           1 if bypass finishes transmission (cleared when exiting Tx mode)
   *      23:    Bypass Tx almost done    1 if bypass almost finishes transmission (cleared when exiting Tx mode)
   *      22:    Tx done                  1 if modem finishes transmission (cleared when exiting Tx mode)
   *      21:    AGC_FE                   1 if AGC FE stage is enabled
   *      20:    AGC_IF                   1 if AGC IF stage is enabled
   *      19:    AFC_timeout              1 if AFC timeout is reached
   *      18:    IQ_comp_rdy              1 if IQ imbalance compensation calibration done
   *      17:    RSSI_inst_hi_level       1 if inst_RSSI value is higher than the threshold
   *      16:    RSSI_inst_lo_level       1 if inst_RSSI value is smaller than the threshold
   *      15:    RSSI_mean_hi_level       1 if mean_RSSI value is higher than the threshold
   *      14:    RSSI_mean_lo_level       1 if mean_RSSI value is smaller than the threshold
   *      13:    RSSI_timeout             1 if RSSI timeout is reached (cleared when leaving RX)
   *      12:    RSSI_mean_ready          1 if a mean RSSI value is ready (cleared when leaving RX)
   *      11:    RSSI_inst_ready          1 if an instant RSSI value is ready (cleared when leaving RX)
   *      10:    RSSI_latch_ready         1 if the latched RSSI values are ready (cleared when leaving RX)
   *       9:    payload_data_in_progress 1 if Payload data sequence reading in progress
   *       8:    preamble_timeout         1 if Preamble sequence detection timeout is reached
   *       7:    preamble_det_in_progress 1 if Preamble sequence detection in progress
   *       6:    preamble_detected        1 if Preamble is detected (Number of detected Preamble symbols exceeds the threshold value)
   *       5:    sync_word_timeout        1 if Sync Word detection timeout is reached
   *       4:    sync_det_in_progress     1 if Sync Word detection in progress
   *       3:    sync_detected            1 if Sync Word is detected (number of matched bits greater or equal to threshold value)
   *       2:    Bypass Payload ready     1 if first bit of the payload is received via bypass (cleared when FIFO is empty)
   *       1:    CRC_valid                1 if received CRC is valid
   *       0:    Payload ready            1 if last byte of the payload is received (cleared when FIFO is empty) */
  volatile unsigned status;
  
  /* Interrupt Mask, rw
   *  bit 21-0 
   *  default value = 0 */
  volatile unsigned mask;
  
  /* Operation Mode register, rw
   * ------------------------------------
   *  bit 5      Mash type 
   *       1:    Mash 1-2
   *       0:    Mash 2 (default)
   *------------------------------------
   *  bit 4      Dither Enable
   *       1:    On
   *       0:    Off (default)
   * -----------------------------------
   *  bit 3      Modulation Type
   *       1:    GFSK
   *       0:    2-FSK (default)
   * -----------------------------------
   *  bit 2-0    Transceiver modes
   *     111:    Bypass Rx FIFO Mode
   *     110:    Bypass Tx FIFO Mode
   *     101:    Bypass Rx Direct Mode
   *     100:    Bypass Rx manual demodulation Mode
   *     011:    Bypass Tx manual modulation Mode
   *     010:    Rx Mode
   *     001:    Tx Mode
   *     000:    InterPacket Gap Mode = standby (default) */
  volatile unsigned op_mode;
  
  /* Reference value of the on-chip clock in KHz, rw
   *  bit 15-0   
   *  default value = 32000 */
  volatile unsigned freq_ref; 
  
  /* Bitrate in hundreds of bps, rw 
   * bit 10-0
   * default value = 555 (55.5 kbps) */
  volatile unsigned bitrate;   
  
  /* Carrier Frequency for transmission in Hz, rw
   * bit 31-0
   * default value = 0x35f19999 */
  volatile unsigned freq_carrier;
  
  /* Center frequency for Low-IF in KHz, rw 
   * bit 7-0
   * default value = 0 */
  volatile unsigned freq_lowif; 
  
  /* Frequency Deviation, rw
   *  bits 23:16   Offset
   *  bits 15:8    Exponent
   *  bits  7:0    Mantissa
   * Equation of the frequency deviation:
   * f dev = (fxosc/2e17) * (offset + Mantissa)* 2^Exponent
   * default value = 0x080417
   */
  volatile unsigned freq_dev;
  
  /* Bit Error Rate, r only
   * bit 15-0 */
  volatile unsigned ber;
  
  // Clock Generator Unit /////////////////////////////////////////////
  /* Clock Generator Enable, rw
   * bit 0
   * */
  volatile unsigned clkgen_en;
  
  /* Bitrate Clock divider, rw 
   * bit 31-0
   * default value = 576 */
  volatile unsigned clk_divider;
  
  /* GFSK OSR prescaler
   * bit 2-0
   * Exponent of the 2's power used for division:
   * divider = 2^prescaler
   * default value = 3 */
  volatile unsigned prescaler;
} Xcvr;

typedef struct Codec
{
  
  /* packet Configuration register
   * bit 18-16 rw Sync Word length
   *              0x0  (default)
   * -----------------------------------
   * bit 15-8  rw Preamble length
   *              0x20 (default)
   * -----------------------------------
   *    bit 7  rw Reserved 
   *------------------------------------
   *    bit 6  rw Preamble polarity
   *           0: 0xAA (default)
   *           1: 0x55
   * -----------------------------------
   *     bit 5 rw PN9 Enable
   *           0: Off (default)
   *           1: On
   * -----------------------------------
   *     bit 4 rw FEC Enable
   *           0: Off (default) 
   *           1: On
   * -----------------------------------
   *     bit 3 rw CRC16 Enable
   *           0: Off (default)
   *           1: On
   * -----------------------------------
   *   bit 2-1 rw Packet length type
   *          00: Fixed length (default) 
   *          01: Variable length (packet length configured by the first byte after sync word)
   *          10: Infinite packet length mode
   * -----------------------------------
   *     bit 0 rw Sync mode
   *           0: Raw data (No preamble/sync)
   *           1: Enable preamble and sync word transmission /detection
   * ---------------------------------*/
  volatile unsigned packet_config;
  
  /* Packet length */
  volatile unsigned packet_len;  // if packet format = 1 (variable), max length in Rx   
  
  /* Sync word, low and high 32 bits */
  volatile unsigned sync_word_l;
  volatile unsigned sync_word_h;
 
  /* DSP Preamble and sync word detector configuration register, rw
   * -----------------------------------
   * bits 31-24     Sync word detection timeout value, in T symbol (duration of 1 symbol)
   *                0xff  default value
   * -----------------------------------
   * bits 23-16     Preamble detection timeout value, in T symbol (duration of 1 symbol)
   *                0xff  default value
   * -----------------------------------
   * bit  15        Sync word timeout enable
   * -----------------------------------
   * bit  14        Preamble timeout enable
   * -----------------------------------
   * bits 13-8      Sync Word detection threshold : minimum number of matched symbols
   *                (number of symbols (bits) of Sync Word, N symb = Sync_det_th+1)
   *                0x0f  default value    
   * -----------------------------------
   * bits  7-1      Preamble detection threshold: minimum expected length
   *                0x10 default value
   * ----------------------------------
   * bit   0        Detector selection
   *            0:  separate preamble and sync word detectors (default)
   *            1:  multibit merged detector 
   * ---------------------------------*/
  volatile unsigned detector_config;

  /* DSP correlation detector configuration (valid for multibit merged detector), rw
   * ----------------------------------- 
   * bit 22-12  Threshold of Sync Word detection
   * ----------------------------------- 
   * bit 11-1   Threshold of Preamble sequence detection 
   * ----------------------------------- 
   * bit  0     Enable correlation phase adjustement 
   *            0:  Off (default)
   *            1:  On */
  volatile unsigned corr_detector_config;
  
  /* Number of valid consecutively received preamble symbols (binary detector type only)*/
  volatile unsigned preamble_det_ndet;
  
  volatile unsigned preamble_quality; // Energy of bits of the Preamble sequence
  volatile unsigned sync_quality; // Energy of bits of the Sync sequence
  volatile unsigned payload_quality; // Energy of payload data bits
  
} Codec;

typedef struct Baseband 
{
  
    /* DSP RX chain configuration
   * ------------------------------------------------------------------------
   * bits 11-8  rw  Data rate mode selection for demodulation
   *            0:  200.0-100.0 KHz
   *            1:  100.0- 50.0 KHz (default)
   *            2:  50.0- 25.0 KHz
   *            3:  25.0- 12.5 KHz
   *            4:  12.5-  6.4 KHz
   *            5:  6.4-  3.2 KHz
   *            6:  3.2-  1.6 KHz
   *            7:  1.6-  0.8 KHz
   *            8:  0.8-  0.4 KHz
   *            9:  0.4-  0.2 KHz
   *           10:  0.2-  0.1 KHz
   *------------------------------------------------------------------------
   *  bit   7   rw  LO injection sign control
   *            0: Upper sideband (default)
   *            1: Lower sideband
   *------------------------------------------------------------------------
   *  bit   6   rw  Filter coefficients set mode
   *            0:  Default hard coded coefficients set is used (default)
   *            1:  Programmable coefficients set is used
   *------------------------------------------------------------------------
   * bits 5-3   rw  CIC gain
   *            0:  -6 dB
   *            1:  0 dB
   *            2:  6 dB
   *            3:  12 dB
   *            4:  18 dB
   *            5:  24 dB
   *            6:  30 dB
   *            7:  36 dB 
   *-----------------------------------------------------------------------*
   * bits 2-0   rw  CIC filter decimation factor
   *            0:  5 (10 MHz)
   *            1:  8 (16 MHz)
   *            2:  10 (20 MHz)
   *            3:  16 (32 MHz) (default)
   *            4:  20 (40 MHz)
   *            5:  25 (50 MHz)
   *-----------------------------------------------------------------------*/
  volatile unsigned rx_config;
  
  /* DSP AGC - Automatic gain control 
   * ------------------------------------------------------------------------
   * bits 23-16 rw RSSI high threshold for stage deactivation (signed, dBFS)
   *               0x?? default value
   * ----------------------------------
   * bits 15-8  rw RSSI low threshold for stage activation (signed, dBFS)
   *               0x?? default value
   *------------------------------------------------------------------------ 
   *  bits 7-4  rw Reserved
   *------------------------------------------------------------------------  
   *      bit 3 rw Manual activation of FE stage
   *            0: Off (default)
   *            1: On
   *------------------------------------------------------------------------
   *      bit 2 rw Manual activation of IF stage
   *            0: Off (default)
   *            1: On
   *------------------------------------------------------------------------
   *      bit 1 rw Analog AGC control mode
   *            0: Manual activation of gain stages
   *            1: Automatic activation of gain stages (default)
   *-----------------------------------------------------------------------*
   *      bit 0 rw Enable of the analog AGC control state machine
   *            0: Off (default)
   *            1: On 
   *-----------------------------------------------------------------------*/
  volatile unsigned agc_config;
  
  /* DSP DCOC - DC removing system configuration register
   * ------------------------------------------------------------------------
   * bits  5-2 rw  Time constant for DC removing 
   *               T = 2^(tc+1) / (freq_ref / 2*cic_decimation_factor)
   *               default value 0x8
   *------------------------------------------------------------------------
   *     bit 1 rw  Enable of the manual correction mode of DC removing system
   *           0:  automatic mode (default)
   *           1:  manual mode
   *-----------------------------------------------------------------------*
   *     bit 0 rw  Enable of DC removing system
   *           0:  Off (default)
   *           1:  On 
   *-----------------------------------------------------------------------*/
  volatile unsigned dcoc_config;
  
  /* DSP DCOC - Manual value of DC offset correction
   * ------------------------------------------------------------------------
   * bits 29-16 rw  manual value of DC offset, channel Q
   *                default value = 0
   *-----------------------------------------------------------------------*
   * bits  13-0 rw  manual value of DC offset, channel I
   *                default value = 0
   *-----------------------------------------------------------------------*/
  volatile unsigned dcoc_man_offs;
  
  /* DSP DCOC - Calculated DC offset correction
   * ------------------------------------------------------------------------
   * bits 29-16 r  calculated DC offset value, channel Q
   *-----------------------------------------------------------------------*
   * bits  13-0 r  calculated DC offset value, channel I
   *-----------------------------------------------------------------------*/
  volatile unsigned dcoc_offs;
  
  /* DSP IQ imbalance compensation configuration register 
   * -----------------------------------------------------------------------*
   *     bit 5 rw  Enable of the iterative mode for the calibration of the 
   *               IQ compensation system
   *           0:  Off (default)
   *           1:  On 
   *------------------------------------------------------------------------
   *  bits 4-2 rw  IQ compensation calibration time constant. 
   *               Time of correction process T = 512 us * sum[1:tc+1]
   *               default value = 0x7
   *------------------------------------------------------------------------
   *     bit 1 rw  Enable automatic coefficient calculation
   *           0:  automatic coefficient calculation (default)
   *           1:  manual coefficients
   *-----------------------------------------------------------------------*
   *     bit 0 rw  Enable of IQ imbalance compensation
   *           0:  Off (default)
   *           1:  On 
   *-----------------------------------------------------------------------*/
  volatile unsigned iq_comp_config;
  
  /* DSP IQ - Manual correction coefficients register
   * ------------------------------------------------------------------------
   * bits 31-16 rw  Manual correction coefficient of Q-channel
   *                default value ?
   *-----------------------------------------------------------------------*
   * bits  15-0 rw  Manual correction coefficient of I-channel
   *                default value ?
   *-----------------------------------------------------------------------*/
  volatile unsigned iq_comp_man_corr;
  
  /* DSP IQ - Calulated value of IQ compensation correction
   * ------------------------------------------------------------------------
   * bits 31-16 r  Calculated correction coefficient of Q-channel
   *               default value ?
   *-----------------------------------------------------------------------*
   * bits  15-0 r  Calculated correction coefficient of I-channel
   *               default value ?
   *-----------------------------------------------------------------------*/
  volatile unsigned iq_comp_corr;
  
  /* DSP AFC - Post demodulation DC remover configuration register
   * -----------------------------------------------------------------------*
   *  bits 6-3 rw  time constant for DC removing 
   *               T = 2^(tc+rate_md+6) / (freq_ref / 2*cic_decimation_factor)
   *               default value = 0xa
   *------------------------------------------------------------------------  
   *     bit 2 rw  Activation of the boost mode of the AFC system
   *           0:  Off (default)
   *           1:  On 
   *------------------------------------------------------------------------ 
   *     bit 1 rw  timeout timer activation for AFC value freezing
   *           0:  no timer (default)
   *           1:  timer activated
   *-----------------------------------------------------------------------*
   *     bit 0 rw  Enable of AFC system
   *           0:  Off (default)
   *           1:  On 
   *-----------------------------------------------------------------------*/
  volatile unsigned afc_config;
  
  /* carrier frequency error register (read only)
   bits 19-0 carrier frequency error value, in Hz
   when AFC timeout is reached, this value is the frozen value */
  volatile unsigned afc_offs;
  
  /* DSP RSSI - configuration register
   * -----------------------------------------------------------------------*
   * bits 25-24 rw  RSSI timeout deactivation event and demodulator chain activation event
   *           00:  tm deactivation if inst_RSSI > hi_th, demod starts when inst_RSSI > hi_th
   *           01:  tm deactivation if inst_RSSI < lo_th, demod starts when inst_RSSI > hi_th
   *           10:  tm deactivation if mean_RSSI > hi_th, demod starts when mean_RSSI > hi_th (default)
   *           11:  tm deactivation if mean_RSSI < lo_th, demod starts when mean_RSSI > hi_th
   *-----------------------------------------------------------------------*
   * bits 23-16 rw  RSSI timeout timer value (number of symbols periods)
   *                default value = 255
   *-----------------------------------------------------------------------*
   *  bits 15-8 rw  Number of symbols to freeze value
   *                default value = 255
   *-----------------------------------------------------------------------*
   *      bit 7 rw  RSSI timeout timer activation
   *            0:  Off (default)
   *            1:  On 
   *-----------------------------------------------------------------------*
   *   bits 6-5 rw  Freeze event mode:
   *            0:  freeze value at the end of preamble (default)
   *            1:  freeze value at the end of sync word
   *            2:  freeze value after N = freeze_naver symbols
   *-----------------------------------------------------------------------*
   *   bits 4-2 rw  Number of averages for mean value (N = 2^(naver+1))
   *                default value = 3
   *------------------------------------------------------------------------
   *      bit 1 rw  RSSI module operation mode
   *            0:  RSSI measurement and conditional start of demodulator processing (default)
   *            1:  RSSI measurement only 
   *-----------------------------------------------------------------------*
   *      bit 0 rw  Enable RSSI measure processing
   *            0:  Off (default)
   *            1:  On 
   *-----------------------------------------------------------------------*/
  volatile unsigned rssi_config;
  
  /* RSSI threshold for demodulator activation or carrier sense, signed integer (dBFS)
   * ------------------------------------------------------------------------
   * bits 15-8 rw  RSSI high threshold
   *               default value ?
   *-----------------------------------------------------------------------*
   * bits  7-0 rw  RSSI low threshold
   *               default value ?
   *-----------------------------------------------------------------------*/
  volatile unsigned rssi_threshold;
  
  /* DSP RSSI values */
  volatile unsigned rssi_inst_val;
  volatile unsigned rssi_mean_val;
  volatile unsigned rssi_latch_inst_val;
  volatile unsigned rssi_latch_mean_val;
  
  /* DSP demodulator configuration register
   * -----------------------------------------------------------------------*
   *  bits 7-6 rw  Modulation index of the receiving modulated data
   *               (to be chosen equal or bigger than the real one)
   *      0 =   0.5
   *      1 =   1.0
   *      2 =   1.5 
   *      3 =   2.0 (default value)
   *-----------------------------------------------------------------------*
   *  bits 5-2 rw  Demodulator output magnitude manual control value
   *               aka OverSampling Ratio (OSR = int(freq_ref/bitrate*2^(5+rate_md)))
   *               recommended values:
   *    166.6 KHz: 6
   *     55.5 KHz: 9 (default value)
   *      9.6 KHz: 7
   *     <9.6 KHz: 10 
   *------------------------------------------------------------------------
   *     bit 1 rw  Demodulator output magnitude control mode
   *           0:  manual magnitude control mode
   *           1:  automatic magnitude control mode (default)
   *-----------------------------------------------------------------------*
   *     bit 0 rw  Enable demodulator (this is necessary only if not using rssi_en)
   *           0:  Off (default)
   *           1:  On 
   *-----------------------------------------------------------------------*/
  volatile unsigned dm_config;
  
  /* DSP clock data recovery configuration register 1
   * -----------------------------------
   * bit  15    Enable jt cor
   * -----------------------------------
   * bit  14    Enable gl filt
   * -----------------------------------
   * bit  13    Enable adjustement for clock data recovery
   *            0:  Off (default)
   *            1:  On
   * -----------------------------------
   * bits 12-8  Preamble length
   * -----------------------------------
   * bit 7-5    KP shift value
   * -----------------------------------
   * bit 4-2    KI shift value
   * -----------------------------------
   * bit 1      KP gain dir
   * -----------------------------------
   * bit 0      K gain enable
   * -----------------------------------*/
  volatile unsigned cdr_config1;
  
  /* DSP clock data recovery configuration register 2
   * -----------------------------------
   * bit 31-16  KP value
   * -----------------------------------
   * bit 15-0   KI value
   * -----------------------------------*/
  volatile unsigned cdr_config2;  

  /* DSP filters manual coefficients */
  volatile unsigned lp_fir_coef_0_1; // MSB = Filter coefficients 1 | LSB = Filter coefficients 0
  volatile unsigned lp_fir_coef_2_3;
  volatile unsigned lp_fir_coef_4_5;
  volatile unsigned lp_fir_coef_6_7; 
  volatile unsigned lp_fir_coef_8_9; 
  volatile unsigned lp_fir_coef_10_11; 
  volatile unsigned lp_fir_coef_12_13; 
  volatile unsigned lp_fir_coef_14_15; 
  volatile unsigned lp_fir_coef_16_17; 
  volatile unsigned lp_fir_coef_18_19; 
  volatile unsigned lp_fir_coef_20_21;
  volatile unsigned lp_fir_coef_22_23;
  volatile unsigned lp_fir_coef_24_25;
  volatile unsigned lp_fir_coef_26_27;
  volatile unsigned lp_fir_coef_28_29;
  volatile unsigned lp_fir_coef_30_31;
  volatile unsigned lp_fir_coef_32_33;

  volatile unsigned tx_mod_man;
  
} Baseband;

//   TX:
//      0x0:  w  FIFO TX Write Data
//      0x4:  rw FIFO TX Threshold Value
//      0x8:  r  FIFO TX Status
//      0xC:  rw FIFO TX Mask
//      0x10: w  FIFO TX Flush
//   RX:
//      0x14: r  FIFO RX Read Data
//      0x18: rw FIFO RX Threshold Value
//      0x1C: r  FIFO RX Status
//      0x20: rw FIFO RX Mask
//      0x24: w  FIFO RX Flush
//

typedef struct Data_if
{
  /* FIFO TX Write Data */
  volatile unsigned tx_data;
  
  /* FIFO TX Threshold Value */
  volatile unsigned tx_thr;
  
  /* FIFO TX Status
   *       5:    flushed          1 if fifo is ready after flush has occurred
   *       4:    overrun          1 if fifo overrun has occurred
   *       3:    underrun         1 if fifo underrun has occurred
   *       2:    full             1 if fifo is full
   *       1:    almost empty     1 if fifo has reached the threshold
   *       0:    empty            1 if fifo is entirely empty */
  volatile unsigned tx_status;
  
  /* FIFO TX Mask */
  volatile unsigned tx_mask;
  
  /* FIFO TX Flush */
  volatile unsigned tx_flush;
  
  /* FIFO RX Read Data */
  volatile unsigned rx_data;
  
  /* FIFO RX Threshold Value */
  volatile unsigned rx_thr;
  
  /* FIFO RX Status
   *       5:    flushed          1 if fifo is ready after flush has occurred
   *       4:    overrun          1 if fifo overrun has occurred
   *       3:    underrun         1 if fifo underrun has occurred
   *       2:    full             1 if fifo is full
   *       1:    almost full      1 if fifo has reached the threshold
   *       0:    !empty           1 if some data is available */
  volatile unsigned rx_status;
  
  /* FIFO RX Mask */
  volatile unsigned rx_mask;
  
  /* FIFO RX Flush */
  volatile unsigned rx_flush;
  
} Data_if;

typedef struct CIC_IQ_Data_if
{
  /* FIFO RX Read Data */
  volatile unsigned rx_data;

  /* FIFO RX Threshold Value */
  volatile unsigned rx_thr;

  /* FIFO RX Status
   *       5:    flushed          1 if fifo is ready after flush has occurred
   *       4:    overrun          1 if fifo overrun has occurred
   *       3:    underrun         1 if fifo underrun has occurred
   *       2:    full             1 if fifo is full
   *       1:    almost full      1 if fifo has reached the threshold
   *       0:    !empty           1 if some data is available */
  volatile unsigned rx_status;

  /* FIFO RX Mask */
  volatile unsigned rx_mask;

  /* FIFO RX Flush */
  volatile unsigned rx_flush;

} CIC_IQ_Data_if;

typedef struct Analog
{
  /* Radio ADC configuration register, rw
   * -----------------------------------------------------------------------*
   * bits 21-16      ADC trimming value
   * -----------------------------------------------------------------------*
   * bits 14-9       manual value for ADC calibration
   * -----------------------------------------------------------------------*
   *  bit 8          enable the ADC calibration process
   * -----------------------------------------------------------------------* 
   * bits 7-4        ADC test mode
   * -----------------------------------------------------------------------*
   *  bit 3          enable for the Q channel
   * -----------------------------------------------------------------------*
   *  bit 2          enable for the I channel
   * -----------------------------------------------------------------------*
   *  bit 1          ADC reference enable Q channel
   * -----------------------------------------------------------------------*
   *  bit 0          ADC reference enable I channel
   * -----------------------------------------------------------------------*/
  volatile unsigned adc_cfg;
  
  /* Radio PLL configuration register, rw
   * -----------------------------------------------------------------------*
   * bits 29-24      manual value for PLL lock regl
   * -----------------------------------------------------------------------*
   * bits 21-16      manual value for PLL lock integrate
   * -----------------------------------------------------------------------*
   * bits 12-10      manual value for PLL VCO band selection
   * -----------------------------------------------------------------------*
   * bits 9-8        manual value for PLL VCO reference
   * -----------------------------------------------------------------------*
   *  bit 3          enable of the PLL VCO calibration process
   * -----------------------------------------------------------------------*
   *  bit 2          enable PLL lock 
   * -----------------------------------------------------------------------*
   *  bit 1          PLL CP OTA enable
   * -----------------------------------------------------------------------*
   *  bit 0          PLL enable
   * -----------------------------------------------------------------------*/
  volatile unsigned pll_cfg;
  
  /* Radio PLL status register, r only
   * -----------------------------------------------------------------------*
   * bits 5-3        PLL VCO band selection read value
   * -----------------------------------------------------------------------*
   *  bit 2          PLL frequency high
   * -----------------------------------------------------------------------*
   *  bit 1          PLL frequency low
   * -----------------------------------------------------------------------*
   *  bit 0          PLL lock detect
   * -----------------------------------------------------------------------*/
  volatile unsigned pll_status;
  
  /* Radio RX front-end configuration register, rw
   * -----------------------------------------------------------------------*
   *  bit 29-24      DC compensation value Q (full scale, 6 bits unsigned)
   * -----------------------------------------------------------------------*
   *  bit 21-16      DC compensation value I (full scale, 6 bits unsigned)
   * -----------------------------------------------------------------------*
   *  bit 10         DC compensation activation, channel Q
   * -----------------------------------------------------------------------*
   *  bit 9          DC compensation activation, channel I
   * -----------------------------------------------------------------------*
   *  bit 8          Frequency divider
   *             0:  divide by 2
   *             1:  divide by 4    
   * -----------------------------------------------------------------------*
   *  bit 7          DSP override mode
   *             0:  LNA and mixer gains are set by SFRs
   *             1:  LNA and mixer gains are set by the DSP AGC system
   * -----------------------------------------------------------------------*
   *  bit 6          Mixer gain mode
   *             0:  20dB gain
   *             1:  40dB gain
   * -----------------------------------------------------------------------*
   *  bit 5          Mixer enable
   * -----------------------------------------------------------------------*
   *  bit 4-3        LNA turbo mode enable         
   * -----------------------------------------------------------------------*
   *  bit 2          LNA PFB enable 
   * -----------------------------------------------------------------------*
   * bits 1-0        LNA control
   *            10:  attenuator enable (-20dB)
   *            01:  gain mode enable (20dB)
   *            00:  disabled
   * -----------------------------------------------------------------------*/
   volatile unsigned rx_cfg;
   
   /* Radio TX configuration register, rw
    * -----------------------------------------------------------------------*
    *  bit 9          Frequency divider
    *             0:  divide by 2
    *             1:  divide by 4 
    * -----------------------------------------------------------------------*
    *  bit 8          drive enable
    * -----------------------------------------------------------------------*
    *  bit 7-5        Transmission I control 
    * -----------------------------------------------------------------------*
    * bits 4-1        Transmission power control
    * -----------------------------------------------------------------------*
    *  bit 0          Transmission enable
    * -----------------------------------------------------------------------*/
   volatile unsigned tx_cfg;
   
   /* Radio Test Control, rw
    * -----------------------------------------------------------------------*
    * bits 13-8       Test Control 3.3
    * -----------------------------------------------------------------------*
    * bits 5-0        Test Control
    * -----------------------------------------------------------------------*/
   volatile unsigned test_cfg;   
   
} Radio;

#ifdef __APS__
#define xcvr     ((Xcvr *)SFRADR_XCVR)
#define codec    ((Codec *)SFRADR_CODEC)
#define baseband ((Baseband *)SFRADR_BASEBAND)
#define dif      ((Data_if *)SFRADR_DATA_IF)
#define bdif     ((Data_if *)SFRADR_BYPASS_DATA_IF)
#define iqdif    ((CIC_IQ_Data_if *)SFRADR_CIC_IQ_DATA_IF)
#define radio    ((Radio *)SFRADR_RADIO)
#endif

#endif
