
#include <iostream>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>

static void fill_yuv_image(uint8_t * data[4], int linesize[4], int width, int height, int frame_index)
{
	/* Y */
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			data[0][y * linesize[0] + x] = x + y + frame_index * 3;
		}
	}

	/* Cb and Cr */
	for (int y = 0; y < height / 2; y++)
	{
		for (int x = 0; x < width / 2; x++)
		{
			data[1][y * linesize[1] + x] = 128 + y + frame_index * 2;
			data[2][y * linesize[2] + x] = 64  + x + frame_index * 5;
		}
	}
}


class AvScale
{
public:
	AvScale() : m_AvSws(nullptr)//, m_SrcFmt(AV_PIX_FMT_NONE), m_DstFmt(AV_PIX_FMT_NONE), m_SrcHeight(0), m_DstHeight(0)
	{
		//av_frame_alloc();
		//memset(m_SrcLineSize, 0, sizeof(m_SrcLineSize));
		//memset(m_DstLineSize, 0, sizeof(m_DstLineSize));
	}

	~AvScale()
	{
		if (m_AvSrc)
		{
			av_frame_free(&m_AvSrc);
		}
		if (m_AvDst)
		{
			av_frame_free(&m_AvDst);
		}
	}


#if 1
	AVFrame * Scale(AVFrame * pSrcFrame, AVPixelFormat eDstFmt, int nDstWidth, int nDstHeight)
	{
		if (pSrcFrame == nullptr)
		{
			return nullptr;
		}

		bool changed = false;
		if (m_AvSrc == nullptr || pSrcFrame->format != m_AvSrc->format || pSrcFrame->height != m_AvSrc->height || pSrcFrame->width != m_AvSrc->width || 0 != memcmp(pSrcFrame->linesize, m_AvSrc->linesize, sizeof(m_AvSrc->linesize)))
		{
			changed = true;
		}
		if (m_AvDst == nullptr || eDstFmt != m_AvDst->format || nDstWidth != m_AvDst->width || nDstHeight != m_AvDst->height)
		{
			changed = true;
		}
		if (changed || m_AvSws == nullptr)
		{
			if (m_AvSrc) av_frame_free(&m_AvSrc);
			if (m_AvDst) av_frame_free(&m_AvDst);
			if (m_AvSws) sws_freeContext(m_AvSws);
			m_AvSws = nullptr;
		}
		if (m_AvSws == nullptr)
		{
			m_AvSrc = av_frame_alloc();
			m_AvDst = av_frame_alloc();

			m_AvSrc->format = pSrcFrame->format;
			m_AvSrc->width  = pSrcFrame->width;
			m_AvSrc->height = pSrcFrame->height;
			memcpy(m_AvSrc->linesize, pSrcFrame->linesize, sizeof(pSrcFrame));

			m_AvDst->format = eDstFmt;
			m_AvDst->width  = nDstWidth;
			m_AvDst->height = nDstHeight;
			int e = av_frame_get_buffer(m_AvDst, 32);
			if (e)
			{
				if (m_AvSrc) av_frame_free(&m_AvSrc);
				if (m_AvDst) av_frame_free(&m_AvDst);
				if (m_AvSws) sws_freeContext(m_AvSws);
				m_AvSws = nullptr;
				return nullptr;
			}
		}
		if (m_AvSws)
		{
			int h = sws_scale(m_AvSws, m_AvSrc->data, m_AvSrc->linesize, 0, m_AvSrc->height, m_AvDst->data, m_AvDst->linesize);
			if (h == m_AvDst->height)
			{
				return m_AvDst;
			}
		}

		return nullptr;
	}
#else
	AVFrame * Scale(AVPixelFormat eSrcFmt, int vSrcLineSize[], int nSrcHeight, AVPixelFormat eDstFmt, int vDstLineSize[], int nDstHeight)
	{
		if (m_AvSws)
		{
			if (eSrcFmt != m_SrcFmt || nSrcHeight != m_SrcHeight || eDstFmt != m_DstFmt || nDstHeight != nDstHeight)
			{
				sws_freeContext(m_AvSws);

				m_AvSws = nullptr;
				m_SrcFmt = AV_PIX_FMT_NONE;
				m_DstFmt = AV_PIX_FMT_NONE;
				memset(m_SrcLineSize, 0, sizeof(m_SrcLineSize));
				memset(m_DstLineSize, 0, sizeof(m_DstLineSize));
				m_SrcHeight = 0;
				m_DstHeight = 0;
			}
		}

		if ()
		{
			sws_ctx = sws_getContext(src_w, src_h, src_pix_fmt,
				dst_w, dst_h, dst_pix_fmt,
				SWS_BILINEAR, NULL, NULL, NULL);
		}
		if (m_AvSws)
		{
			m_AvSws = sws_getContext();
		}
		sws_scale(sws_ctx, (const uint8_t * const*)src_data,
			src_linesize, 0, src_h, dst_data, dst_linesize);
	}
#endif

private:
	SwsContext *	m_AvSws = nullptr;
	AVFrame *		m_AvSrc = nullptr;
	AVFrame	*		m_AvDst = nullptr;
	//AVPixelFormat	m_SrcFmt = AV_PIX_FMT_NONE;
	//AVPixelFormat	m_DstFmt = AV_PIX_FMT_NONE;
	//int				m_SrcLineSize[AV_NUM_DATA_POINTERS] = { 0 };
	//int				m_DstLineSize[AV_NUM_DATA_POINTERS] = { 0 };
	//int				m_SrcHeight = 0;
	//int				m_DstHeight = 0;
};

int main(int argc, char * argv[])
{
	uint8_t *src_data[4], *dst_data[4];
	int src_linesize[4], dst_linesize[4];
	int src_w = 320, src_h = 240, dst_w, dst_h;
	enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_YUV420P, dst_pix_fmt = AV_PIX_FMT_RGB24;


	const char *dst_size = NULL;
	const char *dst_filename = NULL;
	FILE *dst_file;
	int dst_bufsize;
	struct SwsContext *sws_ctx;
	int i, ret;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s output_file output_size\n"
			"API example program to show how to scale an image with libswscale.\n"
			"This program generates a series of pictures, rescales them to the given "
			"output_size and saves them to an output file named output_file\n."
			"\n", argv[0]);
		exit(1);
	}
	dst_filename = argv[1];
	dst_size = argv[2];

	if (av_parse_video_size(&dst_w, &dst_h, dst_size) < 0) {
		fprintf(stderr,
			"Invalid size '%s', must be in the form WxH or a valid size abbreviation\n",
			dst_size);
		exit(1);
	}

	dst_file = fopen(dst_filename, "wb");
	if (!dst_file) {
		fprintf(stderr, "Could not open destination file %s\n", dst_filename);
		exit(1);
	}

	/* create scaling context */
	sws_ctx = sws_getContext(src_w, src_h, src_pix_fmt,
		dst_w, dst_h, dst_pix_fmt,
		SWS_BILINEAR, NULL, NULL, NULL);
	if (!sws_ctx) {
		fprintf(stderr,
			"Impossible to create scale context for the conversion "
			"fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
			av_get_pix_fmt_name(src_pix_fmt), src_w, src_h,
			av_get_pix_fmt_name(dst_pix_fmt), dst_w, dst_h);
		ret = AVERROR(EINVAL);
		goto end;
	}

	/* allocate source and destination image buffers */
	if ((ret = av_image_alloc(src_data, src_linesize,
		src_w, src_h, src_pix_fmt, 16)) < 0) {
		fprintf(stderr, "Could not allocate source image\n");
		goto end;
	}

	/* buffer is going to be written to rawvideo file, no alignment */
	if ((ret = av_image_alloc(dst_data, dst_linesize,
		dst_w, dst_h, dst_pix_fmt, 1)) < 0) {
		fprintf(stderr, "Could not allocate destination image\n");
		goto end;
	}
	dst_bufsize = ret;

	for (i = 0; i < 100; i++) {
		/* generate synthetic video */
		fill_yuv_image(src_data, src_linesize, src_w, src_h, i);

		/* convert to destination format */
		sws_scale(sws_ctx, (const uint8_t * const*)src_data,
			src_linesize, 0, src_h, dst_data, dst_linesize);

		/* write scaled image to file */
		fwrite(dst_data[0], 1, dst_bufsize, dst_file);
	}

	fprintf(stderr, "Scaling succeeded. Play the output file with the command:\n"
		"ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n",
		av_get_pix_fmt_name(dst_pix_fmt), dst_w, dst_h, dst_filename);

end:
	fclose(dst_file);
	av_freep(&src_data[0]);
	av_freep(&dst_data[0]);
	sws_freeContext(sws_ctx);
	return ret < 0;


	return 0;
}
