/**
 * @file  shdma_driver.h
 * @brief DMA driver control module header for multimedia broadcast
 */
/*****************************************************************************/
#ifndef _SHDMA_DRIVER_H_
#define _SHDMA_DRIVER_H_

/*----------------------------------------------------------------------------
  Include File
----------------------------------------------------------------------------*/
#include <mach/dma.h>

/*----------------------------------------------------------------------------
  Macro Definition
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Prototype Declaration
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Type Definition
----------------------------------------------------------------------------*/
/** @brief DMA転送チャネル最大数 */
#define D_SHDMA_CHANNEL_MAX		3		/* DMA_CHANNEL_NUM */
/** @brief DMA転送結果OK値 */
#define D_SHDMA_DMOV_RESULT_OK		0x80000002	/* DMA result ok num*/
/** @brief DMA転送パラメータマスク値 */
#define D_SHDMA_PARAM_MASK		0x0000ffff	/* DMA setting param mask bit  src,dst  */
/** @brief DMA転送パラメータシフト値 */
#define D_SHDMA_SRC_PARAM_SHIFT		16		/* DMA setting param shift bit src */
/** @brief DMA転送最大サイズ */
#define D_SHDMA_TRANS_MAX_SIZE		65535		/* DMA transfer max size */
/** @brief YUVフォーマットの区切り数（y,u,v） */
#define D_SHDMA_YUV_PERIOD		3
/** @brief YUVフォーマットの方向数（horizontal,vertical） */
#define D_SHDMA_YUV_HV			2
/** @brief 奇数チェック */
#define D_SHDMA_ODD_CHECK		2
/** @brief OK共通値 */
#define D_SHDMA_RET_OK			0
/** @brief NG共通値 */
#define D_SHDMA_RET_NG			-1
/** @brief 32バイトアライメント */
#define D_SHDMA_ALIGN_32		32
/** @brief 128バイトアライメント */
#define D_SHDMA_ALIGN_128		128
/** @brief 8192バイトアライメント */
#define D_SHDMA_ALIGN_8192		8192
/** @brief アライメント調整値 */
#define D_SHDMA_ALIGN_ADJUST		1
/** @brief UV高さアライメント調整 */
#define D_SHDMA_ALIGN_HEIGHT_UV		2

#ifdef DEBUG_SHDMA
/** @brief 関数inデバッグログ */
#define SHDMA_DEBUG_MSG_ENTER( p1, p2, p3 )	\
		printk( "%s %s %s %d ENTER :%s p1=%d p2=%d p3=%d\n" \
		, __DATE__, __TIME__, __FILE__, __LINE__, __FUNCTION__, p1, p2, p3 )
/** @brief 関数outデバッグログ */
#define SHDMA_DEBUG_MSG_EXIT( )       \
		printk(  "%s %s %s %d EXIT  :%s \n" \
		, __DATE__, __TIME__, __FILE__, __LINE__, __FUNCTION__ )
/** @brief 関数情報デバッグログ */
#define SHDMA_DEBUG_MSG_INFO( fmt, args... )    \
		printk( "%s %s %s %d %s : " fmt,__DATE__,       \
		__TIME__,__FILE__,__LINE__,__FUNCTION__,## args )
#else /* DEBUG_SHDMA */
/** @brief 関数inデバッグログ */
#define SHDMA_DEBUG_MSG_ENTER( p1, p2, p3 )
/** @brief 関数outデバッグログ */
#define SHDMA_DEBUG_MSG_EXIT(  )
/** @brief 関数情報デバッグログ */
#define SHDMA_DEBUG_MSG_INFO( fmt, args... )
#endif /* DEBUG_SHDMA */

/** @brief DMA転送情報構造体 */
struct shdma_tiler_copy_info_t{
	void *src_handle;		/**< コピー元 ion ハンドルアドレス or IMG_native_handle_tハンドルFD値 */
	void *dst_handle;		/**< コピー先 ion ハンドルアドレス or IMG_native_handle_tハンドルFD値 */
	unsigned int  src_x;		/**< コピー元 コピー開始offset X座標(画素単位) */
	unsigned int  src_y;		/**< コピー元 コピー開始offset Y座標(ライン数) */
	unsigned long src_stride;	/**< コピー元 stride値 */
	unsigned int  dst_x;		/**< コピー先 コピー開始offset X座標(画素単位) */
	unsigned int  dst_y;		/**< コピー先 コピー開始offset Y座標(ライン数) */
	unsigned short width;		/**< コピー横 （画素単位）*/
	unsigned short height;		/**< コピー縦 （ライン数）*/
	int  burst;			/**< BURST MODE */
	int  packed;			/**< PACKED */
	int  write_mode;		/**< WRITE MODE */
};

/** @brief IONバッファ参照構造体 */
struct ion_handle {
	struct kref ref;		/**< 参照カウンタ */
	struct ion_client *client;	/**< クライアントのポインタ */
	struct ion_buffer *buffer;	/**< バッファポインタ */
	struct rb_node node;		/**< クライアントのノード */
	unsigned int kmap_cnt;		/**< カーネルにマッピングされているカウンタ */
	unsigned int dmap_cnt;		/**< DMAにマッピングされているカウンタ */
	unsigned int usermap_cnt;	/**< ユーザ領域にマッピングされているカウンタ */
	unsigned int iommu_map_cnt;	/**< iommuにマッピングされているカウンタ */
};

/** @brief DMA転送パラメータ構造体 */
struct shdma_command_t {
	dmov_box *cmd_ptr;		/**< DMA転送パラメータ */
	u32 *cmd_ptr_ptr;		/**< DMA転送パラメータ先頭 */
	dma_addr_t map_cmd;		/**< DMA転送パラメータ物理アドレス */
	dma_addr_t map_cmd_ptr;		/**< DMA転送パラメータ先頭物理アドレス */
};

/** @brief DMA転送制御構造体 */
struct shdma_dmov_exec_cmdptr_cmd {
	struct msm_dmov_cmd dmov_cmd;	/**< DMA転送パラメータ */
	struct completion complete;	/**< DMA転送完了通知関数 */
	unsigned id;			/**< DMAチャネルID */
	unsigned int result;		/**< DMA転送結果 */
	struct msm_dmov_errdata err;	/**< DMA転送エラー情報構造体 */
};

/*----------------------------------------------------------------------------
  Class Declaration
----------------------------------------------------------------------------*/

#endif /* _SHDMA_DRIVER_H_ */
/*****************************************************************************/
/*   Copyright(C) 2012 Panasonic Mobile Communications Co.,Ltd.              */
/*****************************************************************************/
