
#include <stdint.h>
#include "bl606p.h"
#include <bl606p.h>
#include "bl_irq.h"
#include "drv/irq.h"
#include "bl_os_hal.h"
#include "utils_list.h"

typedef struct bl_irq {
	void *arg;
	csi_dev_t csi_dev;
} bl_irq_t;

extern csi_dev_t *g_irq_table[CONFIG_IRQ_NUM];

void bl_irq_init(void)
{

}

void bl_irq_unregister_with_ctx_yoc(int irqnum)
{
	csi_irq_detach(irqnum);
}

void bl_irq_register_with_ctx_yoc(int irqnum, void *handler, void *ctx)
{
    csi_irq_attach(irqnum, handler, (csi_dev_t *)ctx);
}

void bl_irq_register_with_ctx(int irqnum, void *handler, void *ctx)
{
	csi_dev_t *csi = g_irq_table[irqnum];
	struct bl_irq *irq_dev = (struct bl_irq *)utils_container_of(csi, bl_irq_t, csi_dev);
	bl_irq_register(irqnum, handler);
	irq_dev->arg = ctx;
}

void bl_irq_register(int irqnum, void *handler)
{
	struct bl_irq *irq_dev = bl_os_zalloc(sizeof(struct bl_irq));
	if (irq_dev == NULL) {
		while(1);
	}
    csi_irq_attach(irqnum, handler, &irq_dev->csi_dev);
}

void bl_irq_unregister(int irqnum, void *handler)
{
	csi_irq_detach(irqnum);
	bl_os_free(g_irq_table[irqnum]);
	g_irq_table[irqnum] = NULL;
}

void bl_irq_enable(unsigned int source)
{
	csi_irq_enable(source);
}
void bl_irq_disable(unsigned int source)
{
	csi_irq_disable(source);
}

void bl_irq_pending_set(unsigned int source)
{
    csi_vic_set_pending_irq(source);
}

void bl_irq_pending_clear(unsigned int source)
{
    csi_vic_clear_pending_irq(source);
}
