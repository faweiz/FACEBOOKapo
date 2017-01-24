/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @defgroup ble_sdk_uart_over_ble_main main.c
 * @{
 * @ingroup  ble_sdk_app_nus_eval
 * @brief    UART over BLE application main file.
 *
 * This file contains the source code for a sample application that uses the Nordic UART service.
 * This application uses the @ref srvlib_conn_params module.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "app_button.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp.h"
#include "bsp_btn_ble.h"

//*********************** Modified on 12/20/2016 ***********************//
#include "nrf_delay.h"

//*********************** Modified on 1/15/2017 (Add SPI) ***********************//
#include "nrf_drv_spi.h"
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "boards.h"
#include "app_error.h"
#include <string.h>
#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "sdk_config_spi.h"
#include "math.h"

#define numCols 16
#define numRows 10
int TotalSensors = numCols * numRows;

// ADS7953 Multiplexer ADC
#define SPI__ADS_INSTANCE  0 /**< SPI instance index. */
static const nrf_drv_spi_t spi_ads = NRF_DRV_SPI_INSTANCE(SPI__ADS_INSTANCE);  /**< SPI instance. */

// 74HC595 Shift Register
#define SPI_Shift_INSTANCE  1 /**< SPI instance index. */
static const nrf_drv_spi_t spi_shift = NRF_DRV_SPI_INSTANCE(SPI_Shift_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */

// UART Variables	
		uint16_t length;
		uint8_t PrintBuffer[3];
		uint32_t send_value = 0;		
		double send_incomingValues;
		int incomingValues[160] = {0};
// Shift Register Variables
		static uint8_t       m_tx_buf_shift[1];
		static const uint8_t m_length_shift = sizeof(m_tx_buf_shift);        /**< Transfer length. */	
		static uint8_t       m_tx_buf_shift16[16][1] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	/**< Shift Register TX buffer. */

			static uint8_t       m_tx_buf_sh[]={0xff};
			static const uint8_t m_length_sh = sizeof(m_tx_buf_shift);        /**< Transfer length. */	

			
// SPI Variables		
		uint16_t spi_rx;
		int ADC_value;
		double voltage_value;
// ADS7953 Variables		
		//static uint8_t       m_tx_buf[]={0x18,0x00};
		static uint8_t       m_tx_buf_ads[2];
		static uint8_t       m_rx_buf_ads[sizeof(m_tx_buf_ads) + 1];    /**< RX buffer. */
		static const uint8_t m_length_ads = sizeof(m_tx_buf_ads);        /**< Transfer length. */																						
			static uint8_t       m_tx_buf_ads16[12][2] = { {0x18,0x00},
																									{0x10,0x80},
																									{0x11,0x00},
																									{0x11,0x80},
																									{0x12,0x00},
																									{0x12,0x80},
																									{0x13,0x00},
																									{0x13,0x80},
																									{0x14,0x00},
																									{0x14,0x80},
																									{0x00,0x00},
																									{0x00,0x00}																						
																								};	


//static uint8_t       m_tx_buf_ads16[18][2] = { {0x18,0x00},
//																									{0x10,0x80},
//																									{0x11,0x00},
//																									{0x11,0x80},
//																									{0x12,0x00},
//																									{0x12,0x80},
//																									{0x13,0x00},
//																									{0x13,0x80},
//																									{0x14,0x00},
//																									{0x14,0x80},
//																									{0x15,0x00},
//																									{0x15,0x80},
//																									{0x16,0x00},
//																									{0x16,0x80},
//																									{0x17,0x00},
//																									{0x17,0x80},
//																									{0x00,0x00},
//																									{0x00,0x00}																						
//																								};
/*
static uint8_t       m_tx_buf_ads16[18][2] = { {0x18,0x00},
																							{0x18,0x80},
																							{0x19,0x00},
																							{0x19,0x80},
																							{0x1A,0x00},
																							{0x1A,0x80},
																							{0x1B,0x00},
																							{0x1B,0x80},
																							{0x1C,0x00},
																							{0x1C,0x80},
																							{0x1D,0x00},
																							{0x1D,0x80},
																							{0x1E,0x00},
																							{0x1E,0x80},
																							{0x1F,0x00},
																							{0x1F,0x80},
																							{0x00,0x00},
																							{0x00,0x00}																							
																						};
*/
#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                           /**< Include the service_changed characteristic. If not enabled, the server's database cannot be changed for the lifetime of the device. */

#if (NRF_SD_BLE_API_VERSION == 3)
#define NRF_BLE_MAX_MTU_SIZE            GATT_MTU_SIZE_DEFAULT                       /**< MTU size used in the softdevice enabling and to reply to a BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST event. */
#endif

#define APP_FEATURE_NOT_SUPPORTED       BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2        /**< Reply when unsupported features are requested. */

#define CENTRAL_LINK_COUNT              0                                           /**< Number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT           1                                           /**< Number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

#define DEVICE_NAME                     "3DPS_VCU"                               /**< Name of device. Will be included in the advertising data. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      180                                         /**< The advertising timeout (in units of seconds). */

#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                           /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */

static ble_nus_t                        m_nus;                                      /**< Structure to identify the Nordic UART Service. */
static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */

static ble_uuid_t                       m_adv_uuids[] = {{BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}};  /**< Universally unique service identifier. */


/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_nus    Nordic UART Service structure.
 * @param[in] p_data   Data to be send to UART module.
 * @param[in] length   Length of the data.
 */
/**@snippet [Handling the data received over BLE] */
static void nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)
{
	memset(&send_value,0x00,length);
    for (uint32_t i = 0; i < length; i++)
    {
      while (app_uart_put(p_data[i]) != NRF_SUCCESS);
// Read what the "data request" is send from the smart phone. Refer to "spi_event_handler()"
			send_value = send_value * 10 + (p_data[i] - '0');
    }
    while (app_uart_put('\r') != NRF_SUCCESS);
    while (app_uart_put('\n') != NRF_SUCCESS);
// print what send_value is		
//		printf("send_value: %d\n",send_value);			
}

/**@snippet [Handling the data received over BLE] */

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t       err_code;
    ble_nus_init_t nus_init;

    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;
        default:
            break;
    }
}

/**@brief Function for the application's SoftDevice event handler.
 *
 * @param[in] p_ble_evt SoftDevice event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t                         err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break; // BLE_GAP_EVT_CONNECTED

        case BLE_GAP_EVT_DISCONNECTED:
            err_code = bsp_indication_set(BSP_INDICATE_IDLE);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break; // BLE_GAP_EVT_DISCONNECTED

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GAP_EVT_SEC_PARAMS_REQUEST

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTS_EVT_SYS_ATTR_MISSING

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTC_EVT_TIMEOUT

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTS_EVT_TIMEOUT

        case BLE_EVT_USER_MEM_REQUEST:
            err_code = sd_ble_user_mem_reply(p_ble_evt->evt.gattc_evt.conn_handle, NULL);
            APP_ERROR_CHECK(err_code);
            break; // BLE_EVT_USER_MEM_REQUEST

        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
        {
            ble_gatts_evt_rw_authorize_request_t  req;
            ble_gatts_rw_authorize_reply_params_t auth_reply;

            req = p_ble_evt->evt.gatts_evt.params.authorize_request;

            if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
            {
                if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ)     ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
                {
                    if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
                    }
                    else
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
                    }
                    auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
                    err_code = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                               &auth_reply);
                    APP_ERROR_CHECK(err_code);
                }
            }
        } break; // BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST

#if (NRF_SD_BLE_API_VERSION == 3)
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
            err_code = sd_ble_gatts_exchange_mtu_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                       NRF_BLE_MAX_MTU_SIZE);
            APP_ERROR_CHECK(err_code);
            break; // BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST
#endif

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for dispatching a SoftDevice event to all modules with a SoftDevice
 *        event handler.
 *
 * @details This function is called from the SoftDevice event interrupt handler after a
 *          SoftDevice event has been received.
 *
 * @param[in] p_ble_evt  SoftDevice event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_nus_on_ble_evt(&m_nus, p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
    bsp_btn_ble_on_ble_evt(p_ble_evt);

}

/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;

    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

    // Initialize SoftDevice.
    SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

    ble_enable_params_t ble_enable_params;
    err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
                                                    PERIPHERAL_LINK_COUNT,
                                                    &ble_enable_params);
    APP_ERROR_CHECK(err_code);

    //Check the ram settings against the used number of links
    CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT,PERIPHERAL_LINK_COUNT);

    // Enable BLE stack.
#if (NRF_SD_BLE_API_VERSION == 3)
    ble_enable_params.gatt_enable_params.att_mtu = NRF_BLE_MAX_MTU_SIZE;
#endif
    err_code = softdevice_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Subscribe for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist();
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}

/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to
 *          a string. The string will be be sent over BLE when the last character received was a
 *          'new line' i.e '\r\n' (hex 0x0D) or if the string has reached a length of
 *          @ref NUS_MAX_DATA_LENGTH.
 */
/**@snippet [Handling the data received over UART] */
void uart_event_handle(app_uart_evt_t * p_event)
{
    static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
    static uint8_t index = 0;
    uint32_t       err_code;

    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
            UNUSED_VARIABLE(app_uart_get(&data_array[index]));
            index++;
			//	printf("%s",data_array); // print what is send from "PC(COM4:nRF52) to Phone"
            if ((data_array[index - 1] == '\n') || (index >= (BLE_NUS_MAX_DATA_LEN)))
            {
                err_code = ble_nus_string_send(&m_nus, data_array, index);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }

                index = 0;
            }
            break;

        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}
/**@snippet [Handling the data received over UART] */

/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
static void uart_init(void)
{
    uint32_t                     err_code;
    const app_uart_comm_params_t comm_params =
    {
        RX_PIN_NUMBER,
        TX_PIN_NUMBER,
        RTS_PIN_NUMBER,
        CTS_PIN_NUMBER,
        APP_UART_FLOW_CONTROL_DISABLED,
        false,
      //  UART_BAUDRATE_BAUDRATE_Baud115200
				UART_BAUDRATE_BAUDRATE_Baud9600
    };

    APP_UART_FIFO_INIT( &comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOW,
                       err_code);
    APP_ERROR_CHECK(err_code);
}
/**@snippet [UART Initialization] */

/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t               err_code;
    ble_advdata_t          advdata;
    ble_advdata_t          scanrsp;
    ble_adv_modes_config_t options;

    // Build advertising data struct to pass into @ref ble_advertising_init.
    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = false;
    advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    memset(&scanrsp, 0, sizeof(scanrsp));
    scanrsp.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    scanrsp.uuids_complete.p_uuids  = m_adv_uuids;

    memset(&options, 0, sizeof(options));
    options.ble_adv_fast_enabled  = true;
    options.ble_adv_fast_interval = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = ble_advertising_init(&advdata, &scanrsp, &options, on_adv_evt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    bsp_event_t startup_event;

    uint32_t err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS,
                                 APP_TIMER_TICKS(100, APP_TIMER_PRESCALER),
                                 bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}

/**@brief Function for placing the application in low power state while waiting for events.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

//****************** Modified on 1/15/2017 （+ADS7953) ******************//	
/**
 * @brief ADS7953 SPI user event handler.
 * @param event
 */
//**************** ADS7953 Multiplexer ADC ****************//
void spi_ADS_event_handler(nrf_drv_spi_evt_t const * p_event)
{
    spi_xfer_done = true;
//    NRF_LOG_INFO("Transfer completed.\r\n");
//    if (m_rx_buf_ads[0] != 0)
//    {
//        NRF_LOG_INFO(" Received: \r\n");
//        NRF_LOG_HEXDUMP_INFO(m_rx_buf_ads, strlen((const char *)m_rx_buf_ads));
//    }
}
void SPI_ADS_init(){
	LEDS_CONFIGURE(BSP_LED_0_MASK);
    LEDS_OFF(BSP_LED_0_MASK);

    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

    NRF_LOG_INFO("SPI example\r\n");

    nrf_drv_spi_config_t spi_config_ads = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config_ads.ss_pin   = SPI_ADS_SS_PIN;
    spi_config_ads.miso_pin = SPI_ADS_MISO_PIN;
    spi_config_ads.mosi_pin = SPI_ADS_MOSI_PIN;
    spi_config_ads.sck_pin  = SPI_ADS_SCK_PIN;
		spi_config_ads.frequency  = NRF_DRV_SPI_FREQ_8M;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi_ads, &spi_config_ads, spi_ADS_event_handler));
}
void SPI_ADS_Function(uint8_t tx_buf_ads[]){
	// Reset rx buffer and transfer done flag
        memset(m_rx_buf_ads, 0, m_length_ads);
        spi_xfer_done = false;

				APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi_ads, tx_buf_ads, m_length_ads, m_rx_buf_ads, m_length_ads));

        while (!spi_xfer_done)
        {
            __WFE();
        }

//        NRF_LOG_FLUSH();
        LEDS_INVERT(BSP_LED_0_MASK);
        nrf_delay_ms(10); //delay 10 msec
}
//****************** Modified on 1/21/2017 （+74HC595) ******************//	
/**
 * @brief 74hc595 SPI user event handler.
 * @param event
 */
//**************** 74HC595 Shift Register ****************//
void spi_shift_event_handler(nrf_drv_spi_evt_t const * p_event)
{
    spi_xfer_done = true;
}
void SPI_shift_init(){
		LEDS_CONFIGURE(BSP_LED_0_MASK);
    LEDS_OFF(BSP_LED_0_MASK);

    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

    NRF_LOG_INFO("SPI Shift Register example\r\n");

    nrf_drv_spi_config_t spi_config_shift = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config_shift.ss_pin   = NRF_DRV_SPI_PIN_NOT_USED;
    spi_config_shift.miso_pin = NRF_DRV_SPI_PIN_NOT_USED;
    spi_config_shift.mosi_pin = SPI_SHIFT_MOSI_DS_PIN;
    spi_config_shift.sck_pin  = SPI_SHIFT_SHCP_SCK_PIN;
		spi_config_shift.frequency  = NRF_DRV_SPI_FREQ_125K;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi_shift, &spi_config_shift, spi_shift_event_handler));
}
void SPI_shift_Function(uint8_t tx_buf_shift[]){
//void SPI_shift_Function(){
        spi_xfer_done = false;

//				APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi_shift, m_tx_buf_sh, m_length_sh, NULL, 0));
				APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi_shift, tx_buf_shift, m_length_shift, NULL, 0));

        while (!spi_xfer_done)
        {
            __WFE();
        }

        NRF_LOG_FLUSH();
        LEDS_INVERT(BSP_LED_0_MASK);
        nrf_delay_ms(20); //delay 20 msec
}

/**@brief Application main function.
 */
int main(void)
{
    uint32_t err_code;
    bool erase_bonds;
	
    // Initialize.
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);
    uart_init();

    buttons_leds_init(&erase_bonds);
    ble_stack_init();
    gap_params_init();
    services_init();
    advertising_init();
    conn_params_init();
		
    printf("\r\nUART Start!\r\n");
    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);

// ADS7953
		SPI_ADS_init();
// Shift Register	
		SPI_shift_init();
		 
    // Enter main loop.
		while(1)
    {
//				for(int col=0;col<16;col++)
//				{
//					for(int i=0; i<1;i++)
//					{
//						m_tx_buf_shift[i] = m_tx_buf_shift16[col][i];
//	// Debug				
//	//					printf("m_tx_buf_shift[%d] = %d \n",col,m_tx_buf_shift[i]);					
//					}
//	//// Call Shift Register SPI				
//					SPI_shift_Function(m_tx_buf_shift);
//				}
			
//			SPI_shift_Function();
				
				
			
//****************** Modified on 12/20/2016 （UART + SPI_ADC) ******************//
			//Column: 74HC595
			for(int col=0;col<16;col++)
			{
				for(int i=0; i<1;i++)
				{
					m_tx_buf_shift[i] = m_tx_buf_shift16[col][i];
// Debug				
//					printf("m_tx_buf_shift[%d] = %d \n",col,m_tx_buf_shift[i]);					
				}
//// Call Shift Register SPI				
				SPI_shift_Function(m_tx_buf_shift);
				
//				SPI_shift_Function();
				
				
						// Row: ADS7953
						for(int row=0;row<12;row++)
						{
							for(int j=0; j<2;j++)
							{
								m_tx_buf_ads[j] = m_tx_buf_ads16[row][j];
							}
// Call ADS7953 SPI
							SPI_ADS_Function(m_tx_buf_ads);
							if(row>=2)
							{
							spi_rx = ((m_rx_buf_ads[0]&0x0F)<<8) | m_rx_buf_ads[1];
							ADC_value = spi_rx;
							// Reset rx buffer
							if(ADC_value != 0 & ADC_value <= 500)
									ADC_value=0;
							voltage_value = ADC_value / (4095/2.5);	
// Debug (If use required to increasing nrf_delay_ms!!!)
		//						printf("\n Channel %d \n",i-2);
		//						printf(" ADC Value = %d \n",ADC_value);
		//						printf(" Voltage = %.3f \n",voltage_value);
		//						printf(" spi_rx = %x \n",spi_rx);
		//						printf(" m_rx_buf_ads[0] = %d \n",m_rx_buf_ads[0]);
		//						printf(" m_rx_buf_ads[1] = %d \n",m_rx_buf_ads[1]);
							incomingValues[col*numRows + (row-2)] = ADC_value;						
// Debug							
//						printf("incomingValue[%d] = %d ",col*numRows + (row-2),ADC_value);					
//						incomingValues[(row-2)] = ADC_value;
							}
						} // END "Row" loop
				}	// END "Coulumn" loop
				for(int k=0; k<TotalSensors;k++)
				{
						if(send_value == k){
						send_incomingValues = incomingValues[k] / (4095/2.5);
// Clear PrintBuffer buffer	
						memset(PrintBuffer,0x00,sizeof(PrintBuffer));
						sprintf((char*) PrintBuffer, "%.3f", send_incomingValues);
// Get data length
						length = strlen((char*)PrintBuffer);	
// Sent data over BLE		
						ble_nus_string_send(&m_nus, PrintBuffer, length);
					}
					printf("%d",incomingValues[k]);
// Debug (If use required to increasing nrf_delay_ms!!!)
//					printf(" k[%d] : %d ", k,incomingValues[k]);
				if(k<TotalSensors-1) printf(",");
					nrf_delay_ms(5); //delay 5 msec
			}
			 printf("\n");		
		 power_manage();		


    }
}
