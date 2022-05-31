/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Google Touch Interface for Pixel devices.
 *
 * Copyright 2022 Google LLC.
 */

#ifndef _GOOG_TOUCH_INTERFACE_
#define _GOOG_TOUCH_INTERFACE_

#include <drm/drm_panel.h>
#include <drm/drm_bridge.h>
#include <drm/drm_connector.h>
#include <linux/kfifo.h>

#include "heatmap.h"
#include "touch_offload.h"
#include "uapi/input/touch_offload.h"

#define GOOG_LOG_NAME "GTI"
#define GOOG_DBG(fmt, args...)    pr_debug("[%s] %s: " fmt, GOOG_LOG_NAME,\
					__func__, ##args)
#define GOOG_LOG(fmt, args...)    pr_info("[%s] %s: " fmt, GOOG_LOG_NAME,\
					__func__, ##args)
#define GOOG_WARN(fmt, args...)    pr_warn("[%s] %s: " fmt, GOOG_LOG_NAME,\
					__func__, ##args)
#define GOOG_ERR(fmt, args...)    pr_err("[%s] %s: " fmt, GOOG_LOG_NAME,\
					__func__, ##args)
#define MAX_SLOTS 10

#define KTIME_RELEASE_ALL (ktime_set(0, 0))
#define GTI_DEBUG_KFIFO_LEN 4 /* must be power of 2. */

/*-----------------------------------------------------------------------------
 * enums.
 */

enum gti_cmd_type : u32 {
	/* GTI_CMD operations. */
	GTI_CMD_OPS_START = 0x100,
	GTI_CMD_PING,
	GTI_CMD_RESET,
	GTI_CMD_SELFTEST,

	/* GTI_CMD_GET operations. */
	GTI_CMD_GET_OPS_START = 0x200,
	GTI_CMD_GET_FW_VERSION,
	GTI_CMD_GET_GRIP_MODE,
	GTI_CMD_GET_IRQ_MODE,
	GTI_CMD_GET_PALM_MODE,
	GTI_CMD_GET_SCAN_MODE,
	GTI_CMD_GET_SCREEN_PROTECTOR_MODE,
	GTI_CMD_GET_SENSING_MODE,
	GTI_CMD_GET_SENSOR_DATA,

	/* GTI_CMD_NOTIFY operations. */
	GTI_CMD_NOTIFY_OPS_START = 0x300,
	GTI_CMD_NOTIFY_DISPLAY_STATE,
	GTI_CMD_NOTIFY_DISPLAY_VREFRESH,

	/* GTI_CMD_SET operations. */
	GTI_CMD_SET_OPS_START = 0x400,
	GTI_CMD_SET_CONTINUOUS_REPORT,
	GTI_CMD_SET_GRIP_MODE,
	GTI_CMD_SET_IRQ_MODE,
	GTI_CMD_SET_PALM_MODE,
	GTI_CMD_SET_SCAN_MODE,
	GTI_CMD_SET_SCREEN_PROTECTOR_MODE,
	GTI_CMD_SET_SENSING_MODE,
};

enum gti_continuous_report_setting : u32 {
	GTI_CONTINUOUS_REPORT_DISABLE = 0,
	GTI_CONTINUOUS_REPORT_ENABLE,
	GTI_CONTINUOUS_REPORT_DRIVER_DEFAULT,
};

enum gti_display_state_setting : u32 {
	GTI_DISPLAY_STATE_OFF = 0,
	GTI_DISPLAY_STATE_ON,
};

enum gti_grip_setting : u32 {
	GTI_GRIP_DISABLE = 0,
	GTI_GRIP_ENABLE,
};

enum gti_irq_mode : u32 {
	GTI_IRQ_MODE_DISABLE = 0,
	GTI_IRQ_MODE_ENABLE,
	GTI_IRQ_MODE_NA = 0xFFFFFFFF,
};

/**
 * Motion filter mode.
 *   GTI_MF_MODE_UNFILTER: enable unfilter by continuous reporting.
 *   GTI_MF_MODE_DYNAMIC: dynamic control for motion filter.
 *   GTI_MF_MODE_FILTER: only report touch if coord report changed.
 *   GTI_MF_MODE_AUTO: for development case.
 */
enum gti_mf_mode : u32 {
	GTI_MF_MODE_UNFILTER = 0,
	GTI_MF_MODE_DEFAULT,
	GTI_MF_MODE_DYNAMIC = GTI_MF_MODE_DEFAULT,
	GTI_MF_MODE_FILTER,
	GTI_MF_MODE_AUTO_REPORT,
};

/**
 * Motion filter finite state machine (FSM) state.
 *   GTI_MF_STATE_FILTERED: default coordinate filtering
 *   GTI_MF_STATE_UNFILTERED: coordinate unfiltering for single-touch.
 *   GTI_MF_STATE_FILTERED_LOCKED: filtered coordinates. Locked until
 *                                 touch is lifted or timeout.
 */
enum gti_mf_state : u32 {
	GTI_MF_STATE_FILTERED = 0,
	GTI_MF_STATE_UNFILTERED,
	GTI_MF_STATE_FILTERED_LOCKED,
};

enum gti_palm_setting : u32 {
	GTI_PALM_DISABLE = 0,
	GTI_PALM_ENABLE,
};

enum gti_ping_mode : u32 {
	GTI_PING_NOP = 0,
	GTI_PING_ENABLE,
	GTI_PING_NA = 0xFFFFFFFF,
};

enum gti_pm_state : u32 {
	GTI_RESUME = 0,
	GTI_SUSPEND,
};

enum gti_reset_mode : u32 {
	GTI_RESET_MODE_NOP = 0,
	GTI_RESET_MODE_SW = (1 << 0),
	GTI_RESET_MODE_HW = (1 << 1),
	GTI_RESET_MODE_AUTO = GTI_RESET_MODE_HW | GTI_RESET_MODE_SW,
	GTI_RESET_MODE_NA = 0xFFFFFFFF,
};

enum gti_scan_mode : u32 {
	GTI_SCAN_MODE_AUTO = 0,
	GTI_SCAN_MODE_NORMAL_ACTIVE,
	GTI_SCAN_MODE_NORMAL_IDLE,
	GTI_SCAN_MODE_LP_ACTIVE,
	GTI_SCAN_MODE_LP_IDLE,
	GTI_SCAN_MODE_NA = 0xFFFFFFFF,
};

enum gti_screen_protector_mode : u32 {
	GTI_SCREEN_PROTECTOR_MODE_DISABLE = 0,
	GTI_SCREEN_PROTECTOR_MODE_ENABLE,
	GTI_SCREEN_PROTECTOR_MODE_NA = 0xFFFFFFFF,
};

enum gti_selftest_result : u32 {
	GTI_SELFTEST_RESULT_DONE = 0,
	GTI_SELFTEST_RESULT_SHELL_CMDS_REDIRECT,
	GTI_SELFTEST_RESULT_NA = 0xFFFFFFFF,
};

enum gti_sensing_mode : u32 {
	GTI_SENSING_MODE_DISABLE = 0,
	GTI_SENSING_MODE_ENABLE,
	GTI_SENSING_MODE_NA = 0xFFFFFFFF,
};

enum gti_sensor_data_type : u32 {
	GTI_SENSOR_DATA_TYPE_COORD = TOUCH_DATA_TYPE_COORD,
	GTI_SENSOR_DATA_TYPE_MS = TOUCH_SCAN_TYPE_MUTUAL | TOUCH_DATA_TYPE_STRENGTH,
	GTI_SENSOR_DATA_TYPE_SS = TOUCH_SCAN_TYPE_SELF | TOUCH_DATA_TYPE_STRENGTH,
};

enum gti_vendor_dev_pm_state : u32 {
	GTI_VENDOR_DEV_RESUME = 0,
	GTI_VENDOR_DEV_SUSPEND,
};

/*-----------------------------------------------------------------------------
 * Structures.
 */

struct gti_continuous_report_cmd {
	enum gti_continuous_report_setting setting;
};

struct gti_debug_coord {
	ktime_t time;
	struct TouchOffloadCoord coord;
};

struct gti_debug_input {
	int slot;
	struct gti_debug_coord pressed;
	struct gti_debug_coord released;
};

struct gti_display_state_cmd {
	enum gti_display_state_setting setting;
};

struct gti_display_vrefresh_cmd {
	u32 setting;
};

struct gti_fw_version_cmd {
	char buffer[0x100];
};

struct gti_grip_cmd {
	enum gti_grip_setting setting;
};

struct gti_irq_cmd {
	enum gti_irq_mode setting;
};

struct gti_palm_cmd {
	enum gti_palm_setting setting;
};

struct gti_ping_cmd {
	enum gti_ping_mode setting;
};

struct gti_reset_cmd {
	enum gti_reset_mode setting;
};

struct gti_scan_cmd {
	enum gti_scan_mode setting;
};

struct gti_screen_protector_mode_cmd {
	enum gti_screen_protector_mode setting;
};

struct gti_selftest_cmd {
	enum gti_selftest_result result;
	char buffer[PAGE_SIZE];
};

struct gti_sensing_cmd {
	enum gti_sensing_mode setting;
};

struct gti_sensor_data_cmd {
	enum gti_sensor_data_type type;
	u8 *buffer;
	u32 size;
};

/**
 * struct gti_union_cmd_data - GTI commands to vendor driver.
 * @continuous_report_cmd: command to set continuous reporting.
 * @display_state_cmd: command to notify display state.
 * @display_vrefresh_cmd: command to notify display vertical refresh rate.
 * @fw_version_cmd: command to get fw version.
 * @grip_cmd: command to set/get grip mode.
 * @irq_cmd: command to set/get irq mode.
 * @palm_cmd: command to set/get palm mode.
 * @ping_cmd: command to ping T-IC.
 * @reset_cmd: command to reset T-IC.
 * @scan_cmd: command to set/get scan mode.
 * @screen_protector_mode_cmd: command to set/get screen protector mode.
 * @selftest_cmd: command to do self-test.
 * @sensing_cmd: command to set/set sensing mode.
 * @sensor_data_cmd: command to get sensor data.
 */
struct gti_union_cmd_data {
	struct gti_continuous_report_cmd continuous_report_cmd;
	struct gti_display_state_cmd display_state_cmd;
	struct gti_display_vrefresh_cmd display_vrefresh_cmd;
	struct gti_fw_version_cmd fw_version_cmd;
	struct gti_grip_cmd grip_cmd;
	struct gti_irq_cmd irq_cmd;
	struct gti_palm_cmd palm_cmd;
	struct gti_ping_cmd ping_cmd;
	struct gti_reset_cmd reset_cmd;
	struct gti_scan_cmd scan_cmd;
	struct gti_screen_protector_mode_cmd screen_protector_mode_cmd;
	struct gti_selftest_cmd selftest_cmd;
	struct gti_sensing_cmd sensing_cmd;
	struct gti_sensor_data_cmd sensor_data_cmd;
};

/**
 * struct gti_optional_configuration - optional configuration by vendor driver.
 * @get_fw_version: vendor driver operation to get fw version info.
 * @get_grip_mode: vendor driver operation to get the grip mode setting.
 * @get_irq_mode: vendor driver operation to get irq mode setting.
 * @get_mutual_sensor_data: vendor driver operation to get the mutual sensor data.
 * @get_palm_mode: vendor driver operation to get the palm mode setting.
 * @get_scan_mode: vendor driver operation to get scan mode.
 * @get_screen_protector_mode: vendor driver operation to get screen protector mode.
 * @get_self_sensor_data: vendor driver operation to get the self sensor data.
 * @get_sensing_mode: vendor driver operation to get sensing mode.
 * @notify_display_state: vendor driver operation to notify the display state.
 * @notify_display_vrefresh: vendor driver operation to notify the display vertical refresh rate.
 * @ping: vendor driver operation to ping T-IC.
 * @reset: vendor driver operation to exec reset.
 * @selftest: vendor driver operation to exec self-test.
 * @set_continuous_report: vendor driver operation to apply the continuous reporting setting.
 * @set_grip_mode: vendor driver operation to apply the grip setting.
 * @set_irq_mode: vendor driver operation to apply the irq setting.
 * @set_palm_mode: vendor driver operation to apply the palm setting.
 * @set_scan_mode: vendor driver operation to set scan mode.
 * @set_screen_protector_mode: vendor driver operation to set screen protector mode.
 * @set_sensing_mode: vendor driver operation to set sensing mode.
 */
struct gti_optional_configuration {
	int (*get_fw_version)(void *private_data, struct gti_fw_version_cmd *cmd);
	int (*get_grip_mode)(void *private_data, struct gti_grip_cmd *cmd);
	int (*get_irq_mode)(void *private_data, struct gti_irq_cmd *cmd);
	int (*get_mutual_sensor_data)(void *private_data, struct gti_sensor_data_cmd *cmd);
	int (*get_palm_mode)(void *private_data, struct gti_palm_cmd *cmd);
	int (*get_scan_mode)(void *private_data, struct gti_scan_cmd *cmd);
	int (*get_screen_protector_mode)(void *private_data, struct gti_screen_protector_mode_cmd *cmd);
	int (*get_self_sensor_data)(void *private_data, struct gti_sensor_data_cmd *cmd);
	int (*get_sensing_mode)(void *private_data, struct gti_sensing_cmd *cmd);
	int (*notify_display_state)(void *private_data, struct gti_display_state_cmd *cmd);
	int (*notify_display_vrefresh)(void *private_data, struct gti_display_vrefresh_cmd *cmd);
	int (*ping)(void *private_data, struct gti_ping_cmd *cmd);
	int (*reset)(void *private_data, struct gti_reset_cmd *cmd);
	int (*selftest)(void *private_data, struct gti_selftest_cmd *cmd);
	int (*set_continuous_report)(void *private_data, struct gti_continuous_report_cmd *cmd);
	int (*set_grip_mode)(void *private_data, struct gti_grip_cmd *cmd);
	int (*set_irq_mode)(void *private_data, struct gti_irq_cmd *cmd);
	int (*set_palm_mode)(void *private_data, struct gti_palm_cmd *cmd);
	int (*set_scan_mode)(void *private_data, struct gti_scan_cmd *cmd);
	int (*set_screen_protector_mode)(void *private_data, struct gti_screen_protector_mode_cmd *cmd);
	int (*set_sensing_mode)(void *private_data, struct gti_sensing_cmd *cmd);
};

/**
 * struct goog_touch_interface - Google touch interface data for Pixel.
 * @vendor_private_data: the private data pointer that used by touch vendor driver.
 * @vendor_dev: pointer to struct device that used by touch vendor driver.
 * @vendor_input_dev: pointer to struct inpu_dev that used by touch vendor driver.
 * @dev: pointer to struct device that used by google touch interface driver.
 * @options: optional configuration that could apply by vendor driver.
 * @input_lock: protect the input report between non-offload and offload.
 * @offload: struct that used by touch offload.
 * @offload_frame: reserved frame that used by touch offload.
 * @v4l2: struct that used by v4l2.
 * @panel_bridge: struct that used to register panel bridge notification.
 * @connector: struct that used to get panel status.
 * @cmd: struct that used by vendor default handler.
 * @input_timestamp: input timestamp from touch vendor driver.
 * @mf_downtime: timestamp for motion filter control.
 * @display_vrefresh: display vrefresh in Hz.
 * @mf_mode: current motion filter mode.
 * @mf_state: current motion filter state.
 * @vendor_dev_pm_state: vendor device pm state.
 * @screen_protector_mode_setting: the setting of screen protector mode.
 * @pm_state: GTI device pm state.
 * @tbn_register_mask: the tbn_mask that used to request/release touch bus.
 * @panel_is_lp_mode: display is in low power mode.
 * @force_legacy_report: force to directly report input by kernel input API.
 * @offload_enable: touch offload is enabled or not.
 * @v4l2_enable: v4l2 is enabled or not.
 * @tbn_enable: tbn is enabled or not.
 * @input_timestamp_changed: input timestamp changed from touch vendor driver.
 * @default_grip_enabled: the grip default setting.
 * @default_palm_enabled: the palm default setting.
 * @offload_id: id that used by touch offload.
 * @heatmap_buf: heatmap buffer that used by v4l2.
 * @heatmap_buf_size: heatmap buffer size that used by v4l2.
 * @slot: slot id that current used by input report.
 * @slot_bit_in_use: bitmap of slot in use for this input process cycle.
 * @slot_bit_changed: bitmap of slot state changed for this input process cycle.
 * @slot_bit_active: bitmap of active slot during GTI lifecycle.
 * @dev_id: dev_t used for google interface driver.
 * @vendor_default_handler: touch vendor driver default operation.
 * @released_count: finger up count.
 * @debug_input: struct that used to debug input.
 * @debug_fifo: struct that used to debug input.
 */

struct goog_touch_interface {
	void *vendor_private_data;
	struct device *vendor_dev;
	struct input_dev *vendor_input_dev;
	struct device *dev;
	struct gti_optional_configuration options;
	struct mutex input_lock;
	struct touch_offload_context offload;
	struct touch_offload_frame *offload_frame;
	struct v4l2_heatmap v4l2;
	struct drm_bridge panel_bridge;
	struct drm_connector *connector;
	struct gti_union_cmd_data cmd;
	ktime_t input_timestamp;
	ktime_t mf_downtime;

	int display_vrefresh;
	enum gti_mf_mode mf_mode;
	enum gti_mf_state mf_state;
	enum gti_pm_state pm_state;
	enum gti_vendor_dev_pm_state vendor_dev_pm_state;
	enum gti_screen_protector_mode screen_protector_mode_setting;
	u32 tbn_register_mask;

	bool panel_is_lp_mode;
	bool force_legacy_report;
	bool offload_enable;
	bool v4l2_enable;
	bool tbn_enable;
	bool input_timestamp_changed;
	bool default_grip_enabled;
	bool default_palm_enabled;
	union {
	u8 offload_id_byte[4];
	u32 offload_id;
	};
	u8 *heatmap_buf;
	u32 heatmap_buf_size;
	int slot;
	unsigned long slot_bit_in_use;
	unsigned long slot_bit_changed;
	unsigned long slot_bit_active;
	dev_t dev_id;

	int (*vendor_default_handler)(void *private_data,
		enum gti_cmd_type cmd_type, struct gti_union_cmd_data *cmd);

	/* Debug used. */
	unsigned long released_count;
	struct gti_debug_input debug_input[MAX_SLOTS];
	DECLARE_KFIFO(debug_fifo, struct gti_debug_input, GTI_DEBUG_KFIFO_LEN);
};

/*-----------------------------------------------------------------------------
 * Forward declarations.
 */

inline bool goog_input_legacy_report(struct goog_touch_interface *gti);
inline void goog_input_lock(struct goog_touch_interface *gti);
inline void goog_input_unlock(struct goog_touch_interface *gti);
inline void goog_input_set_timestamp(
		struct goog_touch_interface *gti,
		struct input_dev *dev, ktime_t timestamp);
inline void goog_input_mt_slot(
		struct goog_touch_interface *gti,
		struct input_dev *dev, int slot);
inline void goog_input_mt_report_slot_state(
		struct goog_touch_interface *gti,
		struct input_dev *dev, unsigned int tool_type, bool active);
inline void goog_input_report_abs(
		struct goog_touch_interface *gti,
		struct input_dev *dev, unsigned int code, int value);
inline void goog_input_report_key(
		struct goog_touch_interface *gti,
		struct input_dev *dev, unsigned int code, int value);
inline void goog_input_sync(struct goog_touch_interface *gti, struct input_dev *dev);

void goog_notify_vendor_dev_pm_state_done(
		struct goog_touch_interface *gti,
		enum gti_vendor_dev_pm_state state);
int goog_process_vendor_cmd(struct goog_touch_interface *gti, enum gti_cmd_type cmd_type);
int goog_input_process(struct goog_touch_interface *gti);
struct goog_touch_interface *goog_touch_interface_probe(
		void *private_data,
		struct device *dev,
		struct input_dev *input_dev,
		int (*default_handler)(void *private_data,
			enum gti_cmd_type cmd_type, struct gti_union_cmd_data *cmd),
		struct gti_optional_configuration *options);
int goog_touch_interface_remove(struct goog_touch_interface *gti);

#endif // _GOOG_TOUCH_INTERFACE_

