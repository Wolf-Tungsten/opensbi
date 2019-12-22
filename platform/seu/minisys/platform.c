#include <sbi/riscv_encoding.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/axi-uart-16550.h>
#include <sbi_utils/sys/clint.h>

/* clang-format off */

#define MINISYS_U_HART_COUNT			1
#define MINISYS_U_HART_STACK_SIZE		8192

#define MINISYS_U_SYS_CLK			30000000
#define MINISYS_U_PERIPH_CLK			MINISYS_U_SYS_CLK

#define MINISYS_U_CLINT_ADDR			0x2000000

#define MINISYS_U_PLIC_ADDR			0xc000000
#define MINISYS_U_PLIC_NUM_SOURCES		0x35
#define MINISYS_U_PLIC_NUM_PRIORITIES		4

#define MINISYS_U_UART0_ADDR			0x60000000
#define MINISYS_U_UART1_ADDR			0x60000000

/* clang-format on */

static int minisys_u_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt = sbi_scratch_thishart_arg1_ptr();
	plic_fdt_fixup(fdt, "riscv,plic0");

	return 0;
}

static u32 minisys_u_pmp_region_count(u32 hartid)
{
	return 1;
}

static int minisys_u_pmp_region_info(u32 hartid, u32 index, ulong *prot,
				    ulong *addr, ulong *log2size)
{
	int ret = 0;

	switch (index) {
	case 0:
		*prot	  = PMP_R | PMP_W | PMP_X;
		*addr	  = 0;
		*log2size = __riscv_xlen;
		break;
	default:
		ret = -1;
		break;
	};

	return ret;
}

static int minisys_u_console_init(void)
{
	return axi_uart_16550_init();
}

static int minisys_u_irqchip_init(bool cold_boot)
{
	int rc;
	u32 hartid = sbi_current_hartid();

	if (cold_boot) {
		rc = plic_cold_irqchip_init(MINISYS_U_PLIC_ADDR,
					    MINISYS_U_PLIC_NUM_SOURCES,
					    MINISYS_U_HART_COUNT);
		if (rc)
			return rc;
	}

	return plic_warm_irqchip_init(hartid, (2 * hartid), (2 * hartid + 1));
}

static int minisys_u_ipi_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_ipi_init(MINISYS_U_CLINT_ADDR,
					 MINISYS_U_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_ipi_init();
}

static int minisys_u_timer_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_timer_init(MINISYS_U_CLINT_ADDR,
					   MINISYS_U_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_timer_init();
}

static int minisys_u_system_down(u32 type)
{
	/* For now nothing to do. */
	return 0;
}

const struct sbi_platform_operations platform_ops = {
	.pmp_region_count	= minisys_u_pmp_region_count,
	.pmp_region_info	= minisys_u_pmp_region_info,
	.final_init		= minisys_u_final_init,
	.console_putc		= axi_uart_16550_putc,
	.console_getc		= axi_uart_16550_getc,
	.console_init		= minisys_u_console_init,
	.irqchip_init		= minisys_u_irqchip_init,
	.ipi_send		= clint_ipi_send,
	.ipi_clear		= clint_ipi_clear,
	.ipi_init		= minisys_u_ipi_init,
	.timer_value		= clint_timer_value,
	.timer_event_stop	= clint_timer_event_stop,
	.timer_event_start	= clint_timer_event_start,
	.timer_init		= minisys_u_timer_init,
	.system_reboot		= minisys_u_system_down,
	.system_shutdown	= minisys_u_system_down
};

const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x01),
	.name			= "SEU Minisys Rocket Chip",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= MINISYS_U_HART_COUNT,
	.hart_stack_size	= MINISYS_U_HART_STACK_SIZE,
	.disabled_hart_mask	= 0,
	.platform_ops_addr	= (unsigned long)&platform_ops
};
