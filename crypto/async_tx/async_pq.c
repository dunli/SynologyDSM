#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/raid/pq.h>
#include <linux/async_tx.h>
#include <linux/gfp.h>

static struct page *pq_scribble_page;

#define P(b, d) (b[d-2])
#define Q(b, d) (b[d-1])

static __async_inline struct dma_async_tx_descriptor *
do_async_gen_syndrome(struct dma_chan *chan, struct page **blocks,
		      const unsigned char *scfs, unsigned int offset, int disks,
		      size_t len, dma_addr_t *dma_src,
		      struct async_submit_ctl *submit)
{
	struct dma_async_tx_descriptor *tx = NULL;
	struct dma_device *dma = chan->device;
	enum dma_ctrl_flags dma_flags = 0;
	enum async_tx_flags flags_orig = submit->flags;
	dma_async_tx_callback cb_fn_orig = submit->cb_fn;
	dma_async_tx_callback cb_param_orig = submit->cb_param;
	int src_cnt = disks - 2;
	unsigned char coefs[src_cnt];
	unsigned short pq_src_cnt;
	dma_addr_t dma_dest[2];
	int src_off = 0;
	int idx;
	int i;

	if (P(blocks, disks))
		dma_dest[0] = dma_map_page(dma->dev, P(blocks, disks), offset,
					   len, DMA_BIDIRECTIONAL);
	else
		dma_flags |= DMA_PREP_PQ_DISABLE_P;
	if (Q(blocks, disks))
		dma_dest[1] = dma_map_page(dma->dev, Q(blocks, disks), offset,
					   len, DMA_BIDIRECTIONAL);
	else
		dma_flags |= DMA_PREP_PQ_DISABLE_Q;

	for (i = 0, idx = 0; i < src_cnt; i++) {
		if (blocks[i] == NULL)
			continue;
		dma_src[idx] = dma_map_page(dma->dev, blocks[i], offset, len,
					    DMA_TO_DEVICE);
		coefs[idx] = scfs[i];
		idx++;
	}
	src_cnt = idx;

	while (src_cnt > 0) {
		submit->flags = flags_orig;
		pq_src_cnt = min(src_cnt, dma_maxpq(dma, dma_flags));
		 
		if (src_cnt > pq_src_cnt) {
			submit->flags &= ~ASYNC_TX_ACK;
			submit->flags |= ASYNC_TX_FENCE;
			dma_flags |= DMA_COMPL_SKIP_DEST_UNMAP;
			submit->cb_fn = NULL;
			submit->cb_param = NULL;
		} else {
			dma_flags &= ~DMA_COMPL_SKIP_DEST_UNMAP;
			submit->cb_fn = cb_fn_orig;
			submit->cb_param = cb_param_orig;
			if (cb_fn_orig)
				dma_flags |= DMA_PREP_INTERRUPT;
		}
		if (submit->flags & ASYNC_TX_FENCE)
			dma_flags |= DMA_PREP_FENCE;

		for (;;) {
			tx = dma->device_prep_dma_pq(chan, dma_dest,
						     &dma_src[src_off],
						     pq_src_cnt,
						     &coefs[src_off], len,
						     dma_flags);
			if (likely(tx))
				break;
			async_tx_quiesce(&submit->depend_tx);
			dma_async_issue_pending(chan);
		}

		async_tx_submit(chan, tx, submit);
		submit->depend_tx = tx;

		src_cnt -= pq_src_cnt;
		src_off += pq_src_cnt;

		dma_flags |= DMA_PREP_CONTINUE;
	}

	return tx;
}

static void
do_sync_gen_syndrome(struct page **blocks, unsigned int offset, int disks,
		     size_t len, struct async_submit_ctl *submit)
{
	void **srcs;
	int i;
#ifdef MY_ABC_HERE
	int start = -1, stop = disks - 3;
#endif  

	if (submit->scribble)
		srcs = submit->scribble;
	else
		srcs = (void **) blocks;

	for (i = 0; i < disks; i++) {
		if (blocks[i] == NULL) {
			BUG_ON(i > disks - 3);  
			srcs[i] = (void*)raid6_empty_zero_page;
#ifdef MY_ABC_HERE
		} else {
#else  
		} else
#endif  
			srcs[i] = page_address(blocks[i]) + offset;
#ifdef MY_ABC_HERE
			if (i < disks - 2) {
				stop = i;
				if (start == -1)
					start = i;
			}
		}
#endif  
	}
#ifdef MY_ABC_HERE
	if (submit->flags & ASYNC_TX_PQ_XOR_DST) {
		BUG_ON(!raid6_call.xor_syndrome);
		if (start >= 0)
			raid6_call.xor_syndrome(disks, start, stop, len, srcs);
	} else
		raid6_call.gen_syndrome(disks, len, srcs);
#else  
	raid6_call.gen_syndrome(disks, len, srcs);
#endif  
	async_tx_sync_epilog(submit);
}

struct dma_async_tx_descriptor *
async_gen_syndrome(struct page **blocks, unsigned int offset, int disks,
		   size_t len, struct async_submit_ctl *submit)
{
	int src_cnt = disks - 2;
	struct dma_chan *chan = async_tx_find_channel(submit, DMA_PQ,
						      &P(blocks, disks), 2,
						      blocks, src_cnt, len);
	struct dma_device *device = chan ? chan->device : NULL;
	dma_addr_t *dma_src = NULL;

	BUG_ON(disks > 255 || !(P(blocks, disks) || Q(blocks, disks)));

	if (submit->scribble)
		dma_src = submit->scribble;
	else if (sizeof(dma_addr_t) <= sizeof(struct page *))
		dma_src = (dma_addr_t *) blocks;

#ifdef MY_ABC_HERE
	 
	if (dma_src && device && !(submit->flags & ASYNC_TX_PQ_XOR_DST) &&
#else  
	if (dma_src && device &&
#endif  
	    (src_cnt <= dma_maxpq(device, 0) ||
	     dma_maxpq(device, DMA_PREP_CONTINUE) > 0) &&
	    is_dma_pq_aligned(device, offset, 0, len)) {
		 
		pr_debug("%s: (async) disks: %d len: %zu\n",
			 __func__, disks, len);
		return do_async_gen_syndrome(chan, blocks, raid6_gfexp, offset,
					     disks, len, dma_src, submit);
	}

	pr_debug("%s: (sync) disks: %d len: %zu\n", __func__, disks, len);

	async_tx_quiesce(&submit->depend_tx);

	if (!P(blocks, disks)) {
		P(blocks, disks) = pq_scribble_page;
		BUG_ON(len + offset > PAGE_SIZE);
	}
	if (!Q(blocks, disks)) {
		Q(blocks, disks) = pq_scribble_page;
		BUG_ON(len + offset > PAGE_SIZE);
	}
	do_sync_gen_syndrome(blocks, offset, disks, len, submit);

	return NULL;
}
EXPORT_SYMBOL_GPL(async_gen_syndrome);

static inline struct dma_chan *
pq_val_chan(struct async_submit_ctl *submit, struct page **blocks, int disks, size_t len)
{
	#ifdef CONFIG_ASYNC_TX_DISABLE_PQ_VAL_DMA
	return NULL;
	#endif
	return async_tx_find_channel(submit, DMA_PQ_VAL, NULL, 0,  blocks,
				     disks, len);
}

struct dma_async_tx_descriptor *
async_syndrome_val(struct page **blocks, unsigned int offset, int disks,
		   size_t len, enum sum_check_flags *pqres, struct page *spare,
		   struct async_submit_ctl *submit)
{
	struct dma_chan *chan = pq_val_chan(submit, blocks, disks, len);
	struct dma_device *device = chan ? chan->device : NULL;
	struct dma_async_tx_descriptor *tx;
	unsigned char coefs[disks-2];
	enum dma_ctrl_flags dma_flags = submit->cb_fn ? DMA_PREP_INTERRUPT : 0;
	dma_addr_t *dma_src = NULL;
	int src_cnt = 0;

	BUG_ON(disks < 4);

	if (submit->scribble)
		dma_src = submit->scribble;
	else if (sizeof(dma_addr_t) <= sizeof(struct page *))
		dma_src = (dma_addr_t *) blocks;

	if (dma_src && device && disks <= dma_maxpq(device, 0) &&
	    is_dma_pq_aligned(device, offset, 0, len)) {
		struct device *dev = device->dev;
		dma_addr_t *pq = &dma_src[disks-2];
		int i;

		pr_debug("%s: (async) disks: %d len: %zu\n",
			 __func__, disks, len);
		if (!P(blocks, disks))
			dma_flags |= DMA_PREP_PQ_DISABLE_P;
		else
			pq[0] = dma_map_page(dev, P(blocks, disks),
					     offset, len,
					     DMA_TO_DEVICE);
		if (!Q(blocks, disks))
			dma_flags |= DMA_PREP_PQ_DISABLE_Q;
		else
			pq[1] = dma_map_page(dev, Q(blocks, disks),
					     offset, len,
					     DMA_TO_DEVICE);

		if (submit->flags & ASYNC_TX_FENCE)
			dma_flags |= DMA_PREP_FENCE;
		for (i = 0; i < disks-2; i++)
			if (likely(blocks[i])) {
				dma_src[src_cnt] = dma_map_page(dev, blocks[i],
								offset, len,
								DMA_TO_DEVICE);
				coefs[src_cnt] = raid6_gfexp[i];
				src_cnt++;
			}

		for (;;) {
			tx = device->device_prep_dma_pq_val(chan, pq, dma_src,
							    src_cnt,
							    coefs,
							    len, pqres,
							    dma_flags);
			if (likely(tx))
				break;
			async_tx_quiesce(&submit->depend_tx);
			dma_async_issue_pending(chan);
		}
		async_tx_submit(chan, tx, submit);

		return tx;
	} else {
		struct page *p_src = P(blocks, disks);
		struct page *q_src = Q(blocks, disks);
		enum async_tx_flags flags_orig = submit->flags;
		dma_async_tx_callback cb_fn_orig = submit->cb_fn;
		void *scribble = submit->scribble;
		void *cb_param_orig = submit->cb_param;
		void *p, *q, *s;

		pr_debug("%s: (sync) disks: %d len: %zu\n",
			 __func__, disks, len);

		BUG_ON(!spare || !scribble);

		async_tx_quiesce(&submit->depend_tx);

		tx = NULL;
		*pqres = 0;
		if (p_src) {
			init_async_submit(submit, ASYNC_TX_XOR_ZERO_DST, NULL,
					  NULL, NULL, scribble);
			tx = async_xor(spare, blocks, offset, disks-2, len, submit);
			async_tx_quiesce(&tx);
			p = page_address(p_src) + offset;
			s = page_address(spare) + offset;
			*pqres |= !!memcmp(p, s, len) << SUM_CHECK_P;
		}

		if (q_src) {
			P(blocks, disks) = NULL;
			Q(blocks, disks) = spare;
			init_async_submit(submit, 0, NULL, NULL, NULL, scribble);
			tx = async_gen_syndrome(blocks, offset, disks, len, submit);
			async_tx_quiesce(&tx);
			q = page_address(q_src) + offset;
			s = page_address(spare) + offset;
			*pqres |= !!memcmp(q, s, len) << SUM_CHECK_Q;
		}

		P(blocks, disks) = p_src;
		Q(blocks, disks) = q_src;

		submit->cb_fn = cb_fn_orig;
		submit->cb_param = cb_param_orig;
		submit->flags = flags_orig;
		async_tx_sync_epilog(submit);

		return NULL;
	}
}
EXPORT_SYMBOL_GPL(async_syndrome_val);

static int __init async_pq_init(void)
{
	pq_scribble_page = alloc_page(GFP_KERNEL);

	if (pq_scribble_page)
		return 0;

	pr_err("%s: failed to allocate required spare page\n", __func__);

	return -ENOMEM;
}

static void __exit async_pq_exit(void)
{
	put_page(pq_scribble_page);
}

module_init(async_pq_init);
module_exit(async_pq_exit);

MODULE_DESCRIPTION("asynchronous raid6 syndrome generation/validation");
MODULE_LICENSE("GPL");
