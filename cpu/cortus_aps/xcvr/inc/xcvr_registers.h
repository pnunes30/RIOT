/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_xcvr
 * @{
 *
 * @file
 * @brief       xcvr registers
 *
 * @author      Yousra Amrani <yousra.amrani@cortus.com>
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 */

#ifndef XCVR_REGISTERS_H
#define XCVR_REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 *
 *
 */
#define XCVR_FREQ_OFFSET_SET_ADDR                          (0x8007FD00)

//xcvr
// Operation Mode register
#define XCVR_OPMODE_MASH_TYPE_1_2                                (0x20)
#define XCVR_OPMODE_MASH_TYPE_2                                  (0x00)

#define XCVR_OPMODE_DITHER_ON                                    (0x10)
#define XCVR_OPMODE_DITHER_OFF                                   (0x00)

//Modulation Type
#define XCVR_OPMODE_MODULATION_TYPE_MASK                         (0xF7)
#define XCVR_OPMODE_MODULATION_TYPE_GFSK                         (0x08)
#define XCVR_OPMODE_MODULATION_TYPE_2FSK                         (0x00)
//Transceiver modes
#define XCVR_OPMODE_MASK                                         (0xF8)
#define XCVR_OPMODE_STANDBY                                      (0x00)  /* Default */
#define XCVR_OPMODE_TRANSMITTER                                  (0x01)
#define XCVR_OPMODE_RECEIVER                                     (0x02)
#define XCVR_OPMODE_BYPASS_TX_MANUAL_MODULATION                  (0x03)
#define XCVR_OPMODE_BYPASS_RX_MANUAL_DEMODULATION                (0x04)
#define XCVR_OPMODE_BYPASS_RX_DIRECT                             (0x05)
#define XCVR_OPMODE_BYPASS_TX_FIFO                               (0x06)
#define XCVR_OPMODE_BYPASS_RX_FIFO                               (0x07)

//Reference value of the on-chip clock in KHz, rw
#define XCVR_FREQ_REF                                           (32000)

// Bitrate in hundreds of bps, rw
#define XCVR_BITRATE                                           (166666)

// Carrier Frequency for transmission in Hz, rw
#define XCVR_FREQ_CARRIER                                  (0x38619999)

// Center frequency for Low-IF in KHz, rw
#define XCVR_FREQ_LOWIF                                             (0)

//Clock Generator Enable
#define XCVR_CLKGEN_EN                                              (1)
#define XCVR_CLKGEN_DIS                                             (0)

//FIXME GFSK OSR prescaler
#define XCVR_GFSK_PRESCALER                                      (0x03) // default value
#define XCVR_FSK_PRESCALER                                       (0x03) // default value



//codec
//packet configuration
/* Packet preamble/sync mode */
#define XCVR_CODEC_PACKET_CONFIG_SYNC_MODE_MASK               (0xFFFFE)
#define XCVR_CODEC_PACKET_CONFIG_SYNC_MODE_ENABLE             (0x00001)
#define XCVR_CODEC_PACKET_CONFIG_SYNC_MODE_DISABLE            (0x00000)

//Packet length type
#define XCVR_CODEC_PACKET_CONFIG_LEN_MASK                     (0x3FFF9)
#define XCVR_CODEC_PACKET_CONFIG_LEN_FIX                      (0x00000)
#define XCVR_CODEC_PACKET_CONFIG_LEN_VAR                      (0x00002)
#define XCVR_CODEC_PACKET_CONFIG_LEN_INFI                     (0x00004)

//packet CRC16 Enable
#define XCVR_CODEC_PACKET_CONFIG_CRC_MASK                     (0xFFFF7)
#define XCVR_CODEC_PACKET_CONFIG_CRC_ON                       (0x00008)
#define XCVR_CODEC_PACKET_CONFIG_CRC_OFF                      (0x00000)

//FIXME packet FEC Enable
#define XCVR_CODEC_PACKET_CONFIG_FEC_MASK                     (0xFFFEF)
#define XCVR_CODEC_PACKET_CONFIG_FEC_ON                       (0x00010)
#define XCVR_CODEC_PACKET_CONFIG_FEC_OFF                      (0x00000)


//packet  PN9 Enable
#define XCVR_CODEC_PACKET_CONFIG_PN9_MASK                     (0xFFFDF)
#define XCVR_CODEC_PACKET_CONFIG_PN9_ON                       (0x00020)
#define XCVR_CODEC_PACKET_CONFIG_PN9_OFF                      (0x00000)

//packet  Preamble polarity
#define XCVR_CODEC_PACKET_CONFIG_PREAMBLE_POLARITY_MASK       (0xFFFBF)
#define XCVR_CODEC_PACKET_CONFIG_PREAMBLE_POLARITY_0XAA       (0x00000)
#define XCVR_CODEC_PACKET_CONFIG_PREAMBLE_POLARITY_0X55       (0x00040)

// packet  Preamble length
#define XCVR_CODEC_PACKET_CONFIG_PREAMBLE_LENGTH_MASK         (0xF00FF)
#define XCVR_CODEC_PACKET_CONFIG_PREAMBLE_LENGTH_16_BITS      (0x01000)
#define XCVR_CODEC_PACKET_CONFIG_PREAMBLE_LENGTH_32_BITS      (0x02000)
#define XCVR_CODEC_PACKET_CONFIG_PREAMBLE_LENGTH_128_BITS     (0x08000)
#define XCVR_CODEC_PACKET_CONFIG_PREAMBLE_LENGTH_64_BITS      (0x04000)
#define XCVR_CODEC_PACKET_CONFIG_PREAMBLE_LENGTH_128_BITS     (0x08000)

// Sync word length
#define XCVR_CODEC_PACKET_CONFIG_SYNC_LEN_MASK                (0x0FFFF)
#define XCVR_CODEC_PACKET_CONFIG_SYNC_TWO_BYTES               (0x10000)

//detector config
#define XCVR_CODEC_DETECTOR_CONFIG_SYNC_TIMEOUT_VALUE         (0xFF000000)
#define XCVR_CODEC_DETECTOR_CONFIG_PREAMBLE_TIMEOUT_VALUE     (0x00FF0000)
#define XCVR_CODEC_DETECTOR_CONFIG_SYNC_TIMEOUT_ON            (0x00008000)
#define XCVR_CODEC_DETECTOR_CONFIG_SYNC_TIMEOUT_OFF           (0x00000000)
#define XCVR_CODEC_DETECTOR_CONFIG_PREAMBLE_TIMEOUT_ON        (0x00004000)
#define XCVR_CODEC_DETECTOR_CONFIG_PREAMBLE_TIMEOUT_OFF       (0x00000000)
#define XCVR_CODEC_DETECTOR_CONFIG_SYNC_THRESHOLD             (0x00000D00)
#define XCVR_CODEC_DETECTOR_CONFIG_PREAMBLE_THRESHOLD         (0x00000020) // 64 symbols Default = 0x20 for [7-0] means 0x10 for [7-1]
#define XCVR_CODEC_DETECTOR_CONFIG_SELECTION                  (0x00000000) // Default
//corr_detector_config
#define XCVR_CODEC_CORR_DETECTOR_CONFIG_CORRELATION_ON        (0x000001)
#define XCVR_CODEC_CORR_DETECTOR_CONFIG_CORRELATION_OFF       (0x000000)
#define XCVR_CODEC_CORR_DETECTOR_CONFIG_PREAMBLE_THRESHOLD    (0x000E66) //FIXME (1843)
#define XCVR_CODEC_CORR_DETECTOR_CONFIG_SYNC_THRESHOLD        (0x7FF000) //FIXME (2047)

// mask status
//playload ready
#define XCVR_PAYLOAD_READY_MASK                            (0xFFFFFFE)
#define XCVR_PAYLOAD_READY                                 (0x0000001)
//crc valid
#define XCVR_CRC_VALID_MASK                                (0xFFFFFFD)
#define XCVR_CRC_VALID_ON                                  (0x0000002)
//tx done
#define XCVR_TX_DONE_MASK                                  (0xFBFFFFF)
#define XCVR_TX_DONE_ENABLE                                (0x0400000)

#define XCVR_SYNC_TIMEOUT_MASK                             (0xFFFFFDF)
#define XCVR_SYNC_TIMEOUT                                  (0x0000020)

#define XCVR_SYNC_DETECTED                                 (0x0000008)
#define XCVR_PREAMBLE_DETECTED                             (0x0000040)

#define XCVR_PREAMBLE_DETECTION_IN_PROGRESS_MASK           (0xFFFFF7F)
#define XCVR_PREAMBLE_DETECTION_IN_PROGRESS                (0x0000080)

//frequence deviation f dev = (fxosc/2e17) * (offset + Mantissa)* 2^Exponent
#define XCVR_OFFSET_FREQ_DEV                                   (0x08)

//mask tx register
#define XCVR_DATA_IF_TX_NOT_EMPTY                              (0x01)
#define XCVR_DATA_IF_TX_ALMOST_EMPTY                           (0x02)
#define XCVR_DATA_IF_TX_FULL                                   (0x04)
#define XCVR_DATA_IF_TX_UNDERRUN                               (0x08)
#define XCVR_DATA_IF_TX_OVERRUN                                (0x10)
#define XCVR_DATA_IF_TX_FLUSHED                                (0x20)

#define XCVR_DATA_IF_TX_THRESHOLD                                (20)

//mask rx register
#define XCVR_DATA_IF_RX_NOT_EMPTY                              (0x01)
#define XCVR_DATA_IF_RX_ALMOST_FULL                            (0x02)
#define XCVR_DATA_IF_RX_FULL                                   (0x04)
#define XCVR_DATA_IF_RX_UNDERRUN                               (0x08)
#define XCVR_DATA_IF_RX_OVERRUN                                (0x10)
#define XCVR_DATA_IF_RX_FLUSHED                                (0x20)

#define XCVR_DATA_IF_RX_THRESHOLD                                (60)

//radio

//adc cfg
#define XCVR_RADIO_ADC_CONFIG_TRIMMING_VALUE_MASK               (0xC0FFFF)
#define XCVR_RADIO_ADC_CONFIG_TRIMMING_VALUE_30                 (0x1E0000)

#define XCVR_RADIO_ADC_CONFIG_CALIBRATION_VALUE_MASK            (0xFF8FFF)
#define XCVR_RADIO_ADC_CONFIG_CALIBRATION_ENABLE                (0x000100)
#define XCVR_RADIO_ADC_CONFIG_CALIBRATION_DISABLE               (0x000000)

#define XCVR_RADIO_ADC_CONFIG_TEST_MODE_MASK                    (0xFFFF0F)
#define XCVR_RADIO_ADC_CONFIG_TEST_MODE_1                       (0x000010)


#define XCVR_RADIO_ADC_CONFIG_Q_CHANNEL_ENABLE                  (0x08)
#define XCVR_RADIO_ADC_CONFIG_I_CHANNEL_ENABLE                  (0x04)
#define XCVR_RADIO_ADC_CONFIG_REFERENCE_Q_CHANNEL_ENABLE        (0x02)
#define XCVR_RADIO_ADC_CONFIG_REFERENCE_I_CHANNEL_ENABLE        (0x01)


//pll config
#define XCVR_RADIO_PLL_CONFIG_LOCK_REGL_VALUE              (0x00000000)
#define XCVR_RADIO_PLL_CONFIG_LOCK_INTEGRATE_VALUE         (0x00000000)

#define XCVR_RADIO_PLL_CONFIG_VCO_BAND_SELECT_VALUE_7      (0x00001C00)
#define XCVR_RADIO_PLL_CONFIG_VCO_REF_VALUE_0              (0x00000000)


#define XCVR_RADIO_PLL_CONFIG_VCO_CALIBRATION_ENABLE       (0x00000008)
#define XCVR_RADIO_PLL_CONFIG_VCO_CALIBRATION_DISABLE      (0x00000000)
#define XCVR_RADIO_PLL_CONFIG_PLL_LOCK_ENABLE              (0x00000004)
#define XCVR_RADIO_PLL_CONFIG_PLL_CP_OTA_ENABLE            (0x00000002)
#define XCVR_RADIO_PLL_CONFIG_PLL_ENABLE                   (0x00000001)

//RX CONFIG
#define XCVR_RADIO_RX_CONFIG_DC_COMPENSATION_VALUE_Q_MASK  (0xC0FFFFFF)
#define XCVR_RADIO_RX_CONFIG_DC_COMPENSATION_VALUE_I_MASK  (0xFFC0FFFF)

#define XCVR_RADIO_RX_CONFIG_DC_COMPENSATION_VALUE_Q_32    (0x20000000)
#define XCVR_RADIO_RX_CONFIG_DC_COMPENSATION_VALUE_I_32    (0x00200000)

#define XCVR_RADIO_RX_CONFIG_DC_COMPENSATION_Q_ENABLE      (0x00000400)
#define XCVR_RADIO_RX_CONFIG_DC_COMPENSATION_I_ENABLE      (0x00000200)
#define XCVR_RADIO_RX_CONFIG_DC_COMPENSATION_DISABLE       (0x00000000)

#define XCVR_RADIO_RX_CONFIG_VCO_DIVIDER_MASK              (0xFFFFFEFF)
#define XCVR_RADIO_RX_CONFIG_VCO_DIVIDER_2                 (0x00000000)
#define XCVR_RADIO_RX_CONFIG_VCO_DIVIDER_4                 (0x00000100)

#define XCVR_RADIO_RX_CONFIG_DSP_OVERRIDE_MODE_MASK        (0xFFFFFF7F)
#define XCVR_RADIO_RX_CONFIG_LNA_AND_MIXER_SET_BY_DSP      (0x00000080)
#define XCVR_RADIO_RX_CONFIG_LNA_AND_MIXER_SET_BY_SFRS     (0x00000000)

#define XCVR_RADIO_RX_CONFIG_MIXER_GAIN_MODE_MASK          (0xFFFFFFBF)
#define XCVR_RADIO_RX_CONFIG_MIXER_GAIN_20_DB              (0x00000000)
#define XCVR_RADIO_RX_CONFIG_MIXER_GAIN_40_DB              (0x00000040)

#define XCVR_RADIO_RX_CONFIG_MIXER_ENABLE_MASK             (0xFFFFFFDF)
#define XCVR_RADIO_RX_CONFIG_MIXER_ENABLED                 (0x00000020)
#define XCVR_RADIO_RX_CONFIG_MIXER_DISABLED                (0x00000000)

//!FIXME
#define XCVR_RADIO_RX_CONFIG_LNA_TURBO_MODE_ENABLE         (0x00000000) //FIXME  bit 4-3  2 bits for LNA turbo mode enable/disable?
#define XCVR_RADIO_RX_CONFIG_LNA_PFB_ENABLE                (0x00000004)

#define XCVR_RADIO_RX_CONFIG_LNA_CONTROL_MASK              (0xFFFFFFFC)
#define XCVR_RADIO_RX_CONFIG_LNA_ATTENUATOR_ENABLED        (0x00000002)
#define XCVR_RADIO_RX_CONFIG_LNA_GAIN_ENABLED              (0x00000001)
#define XCVR_RADIO_RX_CONFIG_LNA_DISABLED                  (0x00000000)


//TX CONFIG
#define XCVR_RADIO_TX_CONFIG_VCO_DIVIDER_0                       (0x000)
#define XCVR_RADIO_TX_CONFIG_DRIVE_ENABLE                        (0x100)

#define XCVR_RADIO_TX_CONFIG_VCO_DIVIDER_MASK                    (0x1FF)
#define XCVR_RADIO_TX_CONFIG_VCO_DIVIDER_2                       (0x000)
#define XCVR_RADIO_TX_CONFIG_VCO_DIVIDER_4                       (0x200)


#define XCVR_RADIO_TX_CONFIG_TRANSMISSION_I_CTL_MASK             (0xF1F)
#define XCVR_RADIO_TX_CONFIG_TRANSMISSION_I_CTL_0                (0x000)
#define XCVR_RADIO_TX_CONFIG_TRANSMISSION_I_CTL_2                (0x040)
#define XCVR_RADIO_TX_CONFIG_TRANSMISSION_I_CTL_7                (0x0E0)

#define XCVR_RADIO_TX_CONFIG_TRANSMISSION_POWER_CTL_MASK         (0xFE1)
#define XCVR_RADIO_TX_CONFIG_TRANSMISSION_POWER_CTL_15           (0x01E)
#define XCVR_RADIO_TX_CONFIG_TRANSMISSION_MASK                   (0xFFE)
#define XCVR_RADIO_TX_CONFIG_TRANSMISSION_ENABLE                 (0x001)
#define XCVR_RADIO_TX_CONFIG_TRANSMISSION_DISABLE                (0x000)


//TEST CONFIG
#define XCVR_RADIO_TEST_CONFIG_CONTROL_33                       (0x0000)
#define XCVR_RADIO_TEST_CONFIG_CONTROL                          (0x0000)


//BASEBAND configuration

//RX_CONFIG
//CIC filter decimation factor (bits 2-0)
#define XCVR_BASEBAND_RX_CONFIG_CIC_FILTER_DEC_FAC_50MHZ        (0x005)
#define XCVR_BASEBAND_RX_CONFIG_CIC_FILTER_DEC_FAC_40MHZ        (0x004)
#define XCVR_BASEBAND_RX_CONFIG_CIC_FILTER_DEC_FAC_32MHZ        (0x003)
#define XCVR_BASEBAND_RX_CONFIG_CIC_FILTER_DEC_FAC_20MHZ        (0x002)
#define XCVR_BASEBAND_RX_CONFIG_CIC_FILTER_DEC_FAC_16MHZ        (0x001)
#define XCVR_BASEBAND_RX_CONFIG_CIC_FILTER_DEC_FAC_10MHZ        (0x000)

// CIC gain bits 5-3
#define XCVR_BASEBAND_RX_CONFIG_CIC_GAIN_36_DB                  (0x038)
#define XCVR_BASEBAND_RX_CONFIG_CIC_GAIN_30_DB                  (0x030)
#define XCVR_BASEBAND_RX_CONFIG_CIC_GAIN_24_DB                  (0x028)
#define XCVR_BASEBAND_RX_CONFIG_CIC_GAIN_18_DB                  (0x020)
#define XCVR_BASEBAND_RX_CONFIG_CIC_GAIN_12_DB                  (0x018)
#define XCVR_BASEBAND_RX_CONFIG_CIC_GAIN_6_DB                   (0x010)
#define XCVR_BASEBAND_RX_CONFIG_CIC_GAIN_0_DB                   (0x008)
#define XCVR_BASEBAND_RX_CONFIG_CIC_GAIN_MINUS_6_DB             (0x000)

//Filter coefficients set mode
#define XCVR_BASEBAND_RX_CONFIG_DEFAULT_HARD_CODED_COEFF        (0x000)
#define XCVR_BASEBAND_RX_CONFIG_PROGRAMMABLE_COEFF              (0x040)

//LO injection sign control
#define XCVR_BASEBAND_RX_CONFIG_UPPER_SIDEBAND                  (0x000)
#define XCVR_BASEBAND_RX_CONFIG_LOWER_SIDEBAND                  (0x080)

//Data rate mode selection for demodulation
#define XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MASK                  (0x0FF)
#define XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_02_01            (0xA00) //0.2 - 0.1 KHz
#define XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_04_02            (0x900)
#define XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_08_04            (0x800)
#define XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_16_08            (0x700)
#define XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_32_16            (0x600)
#define XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_64_32            (0x500)
#define XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_125_64           (0x400)
#define XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_250_125          (0x300)
#define XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_500_250          (0x200)
#define XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_1000_500         (0x100) //100.0- 50.0 KHz default
#define XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_2000_1000        (0x000)

//AGC CONFIG
//Enable of the analog AGC control state machine
#define XCVR_BASEBAND_AGC_CONFIG_ANALOG_STATE_MACHINE_ON     (0x000001)
#define XCVR_BASEBAND_AGC_CONFIG_ANALOG_STATE_MACHINE_OFF    (0x000000)
// Analog AGC control mode
#define XCVR_BASEBAND_AGC_CONFIG_MANUAL_ACTIV_GAIN_STATE     (0x000000)
#define XCVR_BASEBAND_AGC_CONFIG_AUTO_ACTIV_GAIN_STATE       (0x000002)
//Manual activation of IF stage
#define XCVR_BASEBAND_AGC_CONFIG_MANUAL_ACTIV_IF_STAGE_ON    (0x000004)
#define XCVR_BASEBAND_AGC_CONFIG_MANUAL_ACTIV_IF_STAGE_OFF   (0x000000)
//Manual activation of FE stage
#define XCVR_BASEBAND_AGC_CONFIG_MANUAL_ACTIV_FE_STAGE_ON    (0x000008)
#define XCVR_BASEBAND_AGC_CONFIG_MANUAL_ACTIV_FE_STAGE_OFF   (0x000000)
//RSSI low threshold for stage activation (signed, dBFS)
#define XCVR_BASEBAND_AGC_CONFIG_RSSI_LOW_THRESHOLD_MASK     (0xFF00FF)
//RSSI high threshold for stage deactivation (signed, dBFS)
#define XCVR_BASEBAND_AGC_CONFIG_RSSI_HIGH_THRESHOLD_MASK    (0x00FFFF)
#define XCVR_BASEBAND_AGC_CONFIG_RSSI_LOW_DEFAULT_THRESHOLD  (0x00D800) // FIXME set the default value (-10??)
#define XCVR_BASEBAND_AGC_CONFIG_RSSI_HIGH_DEFAULT_THRESHOLD (0xFF0000) // FIXME set the default value

//dcoc_config
//Enable of DC removing system
#define XCVR_BASEBAND_DCOC_CONFIG_ON                             (0x01)
#define XCVR_BASEBAND_DCOC_CONFIG_OFF                            (0x00)
//Enable of the manual correction mode of DC removing system
#define XCVR_BASEBAND_DCOC_CONFIG_MANUAL_CORRECTION_ON           (0x02)
#define XCVR_BASEBAND_DCOC_CONFIG_MANUAL_CORRECTION_OFF          (0x00)
//Time constant for DC removing T = 2^(tc+1) / (freq_ref / 2*cic_decimation_factor)
#define XCVR_BASEBAND_DCOC_CONFIG_TIME_CONSTANT                  (0x20)

//iq_comp_config
//Enable of IQ imbalance compensation
#define XCVR_BASEBAND_IQ_COMP_CONFIG_IMBALANCE_COMP_ON           (0x01)
#define XCVR_BASEBAND_IQ_COMP_CONFIG_IMBALANCE_COMP_OFF          (0x00)
//Enable automatic coefficient calculation
#define XCVR_BASEBAND_IQ_COMP_CONFIG_AUTO_COEF_CALCUL_ON         (0x02)
#define XCVR_BASEBAND_IQ_COMP_CONFIG_AUTO_COEF_CALCUL_OFF        (0x00)
//IQ compensation calibration time constant
#define XCVR_BASEBAND_IQ_COMP_CONFIG_CALIBRATION_TIME_CONST      (0x1C)
//Enable of the iterative mode for the calibration of the IQ compensation system
#define XCVR_BASEBAND_IQ_COMP_CONFIG_ITERATIVE_MODE_CALIB_ON     (0x20)
#define XCVR_BASEBAND_IQ_COMP_CONFIG_ITERATIVE_MODE_CALIB_OFF    (0x00)

//afc_config
//Enable of AFC system
#define XCVR_BASEBAND_AFC_CONFIG_ON                              (0x01)
#define XCVR_BASEBAND_AFC_CONFIG_OFF                             (0x00)
//timeout timer activation for AFC value freezing
#define XCVR_BASEBAND_AFC_CONFIG_TIMER_ON                        (0x02)
#define XCVR_BASEBAND_AFC_CONFIG_TIMER_OFF                       (0x00)
//Activation of the boost mode of the AFC system
#define XCVR_BASEBAND_AFC_CONFIG_BOOST_MODE_OFF                   (0x00)
#define XCVR_BASEBAND_AFC_CONFIG_BOOST_MODE_ON                    (0x04)
//time constant for DC removing
//T = 2^(tc+rate_md+6) / (freq_ref / 2*cic_decimation_factor)
#define XCVR_BASEBAND_AFC_CONFIG_TIME_CONST_DC_REMOVING          (0x50) // A << 3 // 0x78 15 << 3

//rssi_config
//Enable RSSI measure processing
#define XCVR_BASEBAND_RSSI_CONFIG_MEASURE_PROCESSING_ON            (0x00000001)
#define XCVR_BASEBAND_RSSI_CONFIG_MEASURE_PROCESSING_OFF           (0x00000000)
// RSSI module operation mode
#define XCVR_BASEBAND_RSSI_CONFIG_MEASURE_AND_CONDI_START          (0x00000000)
#define XCVR_BASEBAND_RSSI_CONFIG_MEASUREMENT_ONLY                 (0x00000002)
//bits 2-4 Number of averages for mean value (N = 2^(naver+1))
#define XCVR_BASEBAND_RSSI_CONFIG_NUM_AVERAGES_MASK                (0xFFFFFFE3)
#define XCVR_BASEBAND_RSSI_CONFIG_DEFAULT_NUM_AVERAGES             (0x0000000C)
#define XCVR_BASEBAND_RSSI_CONFIG_NUM_AVERAGES_ZERO                (0x00000000)
//Freeze event mode
#define XCVR_BASEBAND_RSSI_CONFIG_FREEZE_V_END_OF_PREAMBLE         (0x00000000)
#define XCVR_BASEBAND_RSSI_CONFIG_FREEZE_V_END_OF_SYNC             (0x00000020)
#define XCVR_BASEBAND_RSSI_CONFIG_FREEZE_V_AFTER_NAVER             (0x00000040)
//RSSI timeout timer activation
#define XCVR_BASEBAND_RSSI_CONFIG_TIMEOUT_ON                       (0x00000080)
#define XCVR_BASEBAND_RSSI_CONFIG_TIMEOUT_OFF                      (0x00000000)
#define XCVR_BASEBAND_RSSI_CONFIG_TIMEOUT_MASK                     (0xFFFFFF7F)

//Number of symbols to freeze value
#define XCVR_BASEBAND_RSSI_CONFIG_NUM_SYMBOLS_FREEZE_MASK          (0xFFFF00FF)
#define XCVR_BASEBAND_RSSI_CONFIG_DEFAULT_NUM_SYMBOLS_FREEZE       (0x0000FF00)
//RSSI timeout timer value (number of symbols periods)
#define XCVR_BASEBAND_RSSI_CONFIG_TIMEOUT_TIMER_MASK               (0xFF00FFFF)
#define XCVR_BASEBAND_RSSI_CONFIG_TIMEOUT_TIMER_DEFAULT            (0x00400000)
//rw  RSSI timeout deactivation event and demodulator chain activation event
#define XCVR_BASEBAND_RSSI_CONFIG_TM_DEACTIV_IF_MASK               (0xFCFFFFFF)
#define XCVR_BASEBAND_RSSI_CONFIG_TM_DEACTIV_IF_I_RSSI_SUP_HI_TH   (0x00000000)
#define XCVR_BASEBAND_RSSI_CONFIG_TM_DEACTIV_IF_I_RSSI_INF_LO_TH   (0x01000000)
#define XCVR_BASEBAND_RSSI_CONFIG_TM_DEACTIV_IF_M_RSSI_SUP_HI_TH   (0x02000000)
#define XCVR_BASEBAND_RSSI_CONFIG_TM_DEACTIV_IF_M_RSSI_INF_LO_TH   (0x03000000)

//FIXME
#define XCVR_BASEBAND_RSSI_DEFAULT_THRESHOLD_HIGH                   (0xE2)//D3: -40 //E2 : -30
#define XCVR_BASEBAND_RSSI_DEFAULT_THRESHOLD_LOW                    (0xCE) // -50dBm

//demodulator configuration register
//Enable demodulator
#define XCVR_BASEBAND_DM_CONFIG_ON                               (0x01)
#define XCVR_BASEBAND_DM_CONFIG_OFF                              (0x00)
//Demodulator output magnitude control mode
#define XCVR_BASEBAND_DM_CONFIG_MANUAL_MAGNITUDE_CONTROL_MODE    (0x00)
#define XCVR_BASEBAND_DM_CONFIG_AUTO_MAGNITUDE_CONTROL_MODE      (0x02)
//Demodulator out
#define XCVR_BASEBAND_DM_CONFIG_DEMULATOR_OUT_INF_9              (0x28)
#define XCVR_BASEBAND_DM_CONFIG_DEMULATOR_OUT_9V6                (0x1C)
#define XCVR_BASEBAND_DM_CONFIG_DEMULATOR_OUT_55V5               (0x24)
#define XCVR_BASEBAND_DM_CONFIG_DEMULATOR_OUT_166V6              (0x14)
//Modulation index of the receiving modulated data
#define XCVR_BASEBAND_DM_CONFIG_MODULATION_SHAPING_MASK          (0x3F)
#define XCVR_BASEBAND_DM_CONFIG_MODULATION_SHAPING_05            (0x00)
#define XCVR_BASEBAND_DM_CONFIG_MODULATION_SHAPING_10            (0x40)
#define XCVR_BASEBAND_DM_CONFIG_MODULATION_SHAPING_15            (0x80)
#define XCVR_BASEBAND_DM_CONFIG_MODULATION_SHAPING_20            (0xC0)

//clock data recovery configuration register 1
#define XCVR_BASEBAND_CDR_CONFIG1_K_GAIN_ON                    (0x0001)
#define XCVR_BASEBAND_CDR_CONFIG2_KP_GAIN_DIR                  (0x0002)
#define XCVR_BASEBAND_CDR_CONFIG2_KI_SHIFT_VALUE               (0x0004)
#define XCVR_BASEBAND_CDR_CONFIG2_KP_SHIFT_VALUE               (0x0020)
#define XCVR_BASEBAND_CDR_CONFIG2_PREAMBLE_LENGTH_MASK         (0xE0FF)
#define XCVR_BASEBAND_CDR_CONFIG1_PREAMBLE_LENGTH_VALUE        (0x1F00) // value 31 recommended value by NTLAB
#define XCVR_BASEBAND_CDR_CONFIG2_ADJUSTEMENT_ON               (0x2000)
#define XCVR_BASEBAND_CDR_CONFIG2_GL_FILT_ON                   (0x4000)
#define XCVR_BASEBAND_CDR_CONFIG2_JT_COR_ON                    (0x8000)

//clock data recovery configuration register 2
#define XCVR_BASEBAND_CDR_CONFIG2_KP_VALUE                     (0x9C40000) // 2500 default value given by NTLAB
#define XCVR_BASEBAND_CDR_CONFIG2_KI_VALUE                     (0x00000C8) // 350 but default value given by NTLAB is 200


#ifdef __cplusplus
}
#endif

#endif /* XCVR_REGISTERS_H */
/** @} */
