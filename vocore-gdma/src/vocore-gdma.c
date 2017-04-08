#include <linux/init.h>
#include <linux/module.h>

#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/dma-mapping.h>
#include <linux/sysfs.h>

#include <linux/vmalloc.h>

#define MAX_BUFF	0x200000

struct vocore_data {
	void __iomem *gdma;
	void __iomem *i2s;

	dma_addr_t addr;
	void *vaddr;

	u8 *buf;
	u32 size;		// current data size in buffer.
	u32 curr;		// current data pointer.
};

irqreturn_t vocore_gdma_irq_handler(int irq, void *dev_id)
{
	struct vocore_data *p = (struct vocore_data *)dev_get_drvdata(dev_id);
	u32 mask, done;

	mask = readl(p->gdma + 0x0200);
	done = readl(p->gdma + 0x0204);

	// MASK, DONE their register type is W1C(write one to clean)
	writel(mask, p->gdma + 0x0200);
	writel(done, p->gdma + 0x0204);

	memcpy(p->vaddr, p->buf + p->curr, PAGE_SIZE);
	p->curr += PAGE_SIZE;
	if(p->curr >= p->size)
		p->curr = 0;

	// must fill the address again, or it will send some "random" data.
	writel(p->addr, p->gdma + 0x0020);
	writel(0x10000A10, p->gdma + 0x0024);
	writel(0x10000046, p->gdma + 0x0028);
	return IRQ_HANDLED;
}

// input format must be 44.1KHz, 16bits, stereo pcm data, max data size is 2MB
static ssize_t vocore_gdma_data_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct vocore_data *p = (struct vocore_data *)dev_get_drvdata(dev);
	if(p->buf == NULL)
		p->buf = (u8 *)vmalloc(MAX_BUFF);	// 2MB buffer

	if(p->size + count > MAX_BUFF) {
		printk("buffer has full = %d.\n", p->size);
		return count;
	}

	memcpy(p->buf + p->size, buf, count);
	p->size += count;

	return count;
}

static ssize_t vocore_gdma_play_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct vocore_data *p = (struct vocore_data *)dev_get_drvdata(dev);

	switch(buf[0]) {
	case '0': {		// stop
		writel(0x00000044, p->gdma + 0x0028);
		writel(0x00004040, p->i2s + 0x0000);

		p->size = 0; // clean buffer.

		printk("vocore_gdma_play_store: stop!\n");
		break; }

	case '1': {		// start
		if(p->size < PAGE_SIZE) {
			printk("not enough data, can not start.\n");
			break;
		}

		memcpy(p->vaddr, p->buf, PAGE_SIZE);
		p->curr = PAGE_SIZE;

		// setup i2s
		writel(0x80000058, p->i2s + 0x0020); // clock dividor DIVCOMP
		writel(0x0000000E, p->i2s + 0x0024); // clock dividor DIVINT
		writel(0xC1004040, p->i2s + 0x0000); // enable i2s, enable dma, enable tx, standard fifo thresholds

		// start dma transfer (DMA 2 = i2s TX)
		writel(p->addr, p->gdma + 0x0020); // DMA2: src = buffer
		writel(0x10000A10, p->gdma + 0x0024); // DMA2: dest = i2s data register
		writel(0x00200210, p->gdma + 0x002c); // 0 segments, srcDMAReq32=mem, destDMAReq2=i2s, nextChUnmask2=i2s
		writel(0x10000046, p->gdma + 0x0028); // 4096 words, src inc, dest fix, burst=1DW, segment done irq enabled, ch_en=1, hardware mode

		printk("vocore_gdma_play_store: start!\n");
		break; }
	}

	return count;
}

static DEVICE_ATTR(data, S_IWUSR, NULL, vocore_gdma_data_store);
static DEVICE_ATTR(play, S_IWUSR, NULL, vocore_gdma_play_store);

static int vocore_gdma_probe(struct platform_device* dev)
{
	int irq, r;
	struct resource *res;
	struct vocore_data *p;

	p = (struct vocore_data *)kmalloc(sizeof(struct vocore_data), GFP_KERNEL);
	memset(p, 0, sizeof(struct vocore_data));
	platform_set_drvdata(dev, p);

	irq = platform_get_irq(dev, 0);
	printk("platform_get_irq %d\n", irq);

	r = request_irq(irq, vocore_gdma_irq_handler, IRQF_SHARED, "vocore-gdma", &dev->dev);
	if(r < 0) {
		printk("request_irq failed %d.\n", r);
		return 0;
	}
	printk("request_irq %d success.\n", irq);


	res = platform_get_resource(dev, IORESOURCE_MEM, 0);
	p->gdma = devm_ioremap_resource(&dev->dev, res);
	p->i2s = p->gdma - 0x00002800 + 0x00000a00;

	p->vaddr = dma_alloc_coherent(&dev->dev, PAGE_SIZE, &p->addr, GFP_DMA);
	printk("virtual address: %p, physics address: %p\n", p->vaddr, (void *)p->addr);

	device_create_file(&dev->dev, &dev_attr_data);
	device_create_file(&dev->dev, &dev_attr_play);
	return 0;
}

static int vocore_gdma_remove(struct platform_device* dev)
{
	struct vocore_data *p = (struct vocore_data *)platform_get_drvdata(dev);
	int irq = platform_get_irq(dev, 0);

	free_irq(irq, &dev->dev);
	printk("free_irq %d done.\n", irq);

	dma_free_coherent(&dev->dev, PAGE_SIZE, p->vaddr, p->addr);
	printk("free virtual address: %p, physics address: %p\n", p->vaddr, (void *)p->addr);

	if(p->buf)
		vfree(p->buf);
	kfree(p);

	device_remove_file(&dev->dev, &dev_attr_data);
	device_remove_file(&dev->dev, &dev_attr_play);
	return 0;
}


static const struct of_device_id gdma_rt_dt_ids[] = {
	{ .compatible = "ralink,rt3883-gdma", },
	{ }
};

static struct platform_driver rt_gdma_driver = {
	.probe = vocore_gdma_probe,
	.remove = vocore_gdma_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "ralink-gdma",
		.of_match_table = gdma_rt_dt_ids,
	},
};

static int vocore_gdma_init(void)
{
	printk("vocore_gdma_init\n");
	return platform_driver_register(&rt_gdma_driver);
}

static void vocore_gdma_exit(void)
{
	platform_driver_unregister(&rt_gdma_driver);
	printk("vocore_gdma_exit\n");
}

module_init(vocore_gdma_init);
module_exit(vocore_gdma_exit);


MODULE_AUTHOR("Qin Wei <me@vonger.cn>");
MODULE_DESCRIPTION("VoCore Test GDMA&I2S driver");
MODULE_LICENSE("GPL");
