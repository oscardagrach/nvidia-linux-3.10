#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/nct1008.h>
#include <linux/pid_thermal_gov.h>
#include <linux/tegra-fuse.h>
#include <linux/of_platform.h>
#include <mach/edp.h>
#include <mach/io_dpd.h>
#include <linux/platform_device.h>

#include <linux/platform/tegra/cpu-tegra.h>
#include "devices.h"
#include "board.h"
#include "board-common.h"
#include "board-molly.h"
#include "tegra-board-id.h"

static struct pid_thermal_gov_params cpu_pid_params = {
	.max_err_temp = 4000,
	.max_err_gain = 1000,

	.gain_p = 1000,
	.gain_d = 0,

	.up_compensation = 15,
	.down_compensation = 15,
};

static struct thermal_zone_params cpu_tzp = {
	.governor_name = "pid_thermal_gov",
	.governor_params = &cpu_pid_params,
};

static struct thermal_zone_params board_tzp = {
	.governor_name = "pid_thermal_gov"
};

static struct nct1008_platform_data nct1008_pdata = {
	.loc_name = "tegra",
	.supported_hwrev = true,
	.conv_rate = 0x06, /* 4Hz conversion rate */
	.extended_range = true,

	.sensors = {
		[LOC] = {
			.tzp = &board_tzp,
			.shutdown_limit = 120, /* C */
			.passive_delay = 1000,
			.num_trips = 1,
			.trips = {
				{
					.cdev_type = "suspend_soctherm",
					.trip_temp = 50000,
					.trip_type = THERMAL_TRIP_ACTIVE,
					.upper = 1,
					.lower = 1,
					.hysteresis = 5000,
					.mask = 1,
				},
			},
		},
		[EXT] = {
			.tzp = &cpu_tzp,
			.shutdown_limit = 105,
			.passive_delay = 1000,
			.num_trips = 1,
			.trips = {
				{
					.cdev_type = "shutdown_warning",
					.trip_temp = 50000,
					.trip_type = THERMAL_TRIP_ACTIVE,
					.upper = 1,
					.lower = 1,
					.hysteresis = 5000,
					.mask = 0,
				},
			},
		},
	},
};

static struct i2c_board_info __initdata nct1008_i2c_board_info[] = {
	{
		I2C_BOARD_INFO("nct1008", 0x4C),
		.platform_data = &nct1008_pdata,
		.irq  = -1,
	},
};

/* Our real part is TI tmp451, which is a derivative
 * and software compatible with nct1008
 */

static void __init temp_sensor_init(void)
{
	int nct1008_port;
	int ret = 0;

	nct1008_port = TEGRA_GPIO_PJ0;

	tegra_add_all_vmin_trips(nct1008_pdata.trips, &nct1008_pdata.num_trips);

	nct1008_i2c_board_info[0].irq = gpio_to_irq(nct1008_port);
	pr_info("%s: nct1008 irq %d", __func__, nct1008_i2c_board_info[0].irq);

	ret = gpio_request(nct1008_port, "temp_alert");
	if (ret < 0) {
		pr_err("%s: gpio_request() for nct1008_port %d failed\n",
		       __func__, nct1008_port);
		return;
	}

	ret = gpio_direction_input(nct1008_port);
	if (ret < 0) {
		pr_info("%s: calling gpio_free(nct1008_port)", __func__);
		gpio_free(nct1008_port);
		return;
	}

	/* molly thermal sensor on I2C3/CAM_I2C, i.e. instance 2 */
	i2c_register_board_info(2, nct1008_i2c_board_info,
				ARRAY_SIZE(nct1008_i2c_board_info));
}

int __init molly_sensors_init(void)
{
	temp_sensor_init();
	return 0;
}
