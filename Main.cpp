#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
extern "C"
{
//#include <libavutil/imgutils.h>
//#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
}
#include "VideoRender.h"


static void FillYuvImage(std::uint8_t * data[4], int linesize[4], int width, int height, int frame_index)
{
	std::uint8_t * yline = data[0];
	std::uint8_t * uline = data[1];
	std::uint8_t * vline = data[2];

	/* Y */
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			yline[x] = x + y + frame_index * 3;
		}

		yline += linesize[0];
	}

	/* Cb and Cr */
	for (int y = 0; y < height / 2; y++)
	{
		for (int x = 0; x < width / 2; x++)
		{
			uline[x] = 128 + y + frame_index * 2;
			vline[x] = 64 + x + frame_index * 5;
		}

		uline += linesize[1];
		vline += linesize[2];
	}
}


class AvScale
{
public:
	AvScale() : m_AvSws(nullptr), m_AvSrc(nullptr), m_AvDst(nullptr)
	{
	}

	~AvScale()
	{
		if (m_AvSrc) av_frame_free(&m_AvSrc);
		if (m_AvDst) av_frame_free(&m_AvDst);
		if (m_AvSws) sws_freeContext(m_AvSws);
	}

#if 0
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
			memcpy(m_AvSrc->linesize, pSrcFrame->linesize, sizeof(pSrcFrame->linesize));

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

			m_AvSws = sws_getContext(m_AvSrc->width, m_AvSrc->height, (AVPixelFormat)m_AvSrc->format,
				m_AvDst->width, m_AvDst->height, (AVPixelFormat)m_AvDst->format,
				SWS_FAST_BILINEAR, NULL, NULL, NULL);
		}
		if (m_AvSws)
		{
			int h = sws_scale(m_AvSws, pSrcFrame->data, m_AvSrc->linesize, 0, m_AvSrc->height, m_AvDst->data, m_AvDst->linesize);
			if (h == m_AvDst->height)
			{
				return m_AvDst;
			}
		}

		return nullptr;
	}
#endif

	int Scale(AVFrame * pSrcFrame, AVFrame * pDstFrame)
	{
		if (pSrcFrame == nullptr || pDstFrame == nullptr)
		{
			return -1;
		}

		bool changed = false;
		if (m_AvSrc == nullptr || pSrcFrame->format != m_AvSrc->format || pSrcFrame->height != m_AvSrc->height || pSrcFrame->width != m_AvSrc->width)
		{
			changed = true;
		}
		if (m_AvDst == nullptr || pDstFrame->format != m_AvDst->format || pDstFrame->width != m_AvDst->width || pDstFrame->height != m_AvDst->height)
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
			//memcpy(m_AvSrc->linesize, pSrcFrame->linesize, sizeof(pSrcFrame->linesize));

			m_AvDst->format = pDstFrame->format;
			m_AvDst->width  = pDstFrame->width;
			m_AvDst->height = pDstFrame->height;

			m_AvSws = sws_getContext(m_AvSrc->width, m_AvSrc->height, (AVPixelFormat)m_AvSrc->format,
				m_AvDst->width, m_AvDst->height, (AVPixelFormat)m_AvDst->format,
				SWS_FAST_BILINEAR, NULL, NULL, NULL);
		}
		if (m_AvSws)
		{
			int h = sws_scale(m_AvSws, pSrcFrame->data, pSrcFrame->linesize, 0, m_AvSrc->height, pDstFrame->data, pDstFrame->linesize);
			if (h == m_AvDst->height)
			{
				return 0;
			}
		}

		return -1;
	}
private:
	SwsContext *	m_AvSws = nullptr;
	AVFrame *		m_AvSrc = nullptr;
	AVFrame	*		m_AvDst = nullptr;
};


int main77(int argc, char * argv[])
{
	std::int64_t bt = GetTickCount();
	for (int i = 0; i < 1000; ++i)
	{
		HANDLE whandle = OpenEventA(EVENT_ALL_ACCESS, NULL, "CVOS_WEVENT_NAME");
		CloseHandle(whandle);
	}
	std::int64_t et = GetTickCount();
	printf("%lld / 1000 = %f\n", et - bt, double(et - bt) / 1000);
	getchar();

	AvScale sws;
	AVFrame * yuv = av_frame_alloc();
	yuv->width  = 1920;
	yuv->height = 1080;
	yuv->format = AV_PIX_FMT_YUV420P;
	int e = av_frame_get_buffer(yuv, 32);
	if (e)
	{
		av_frame_free(&yuv);
		return e;
	}

	std::uint8_t * pRGB24Buf = new std::uint8_t[2100 * 2200 * 4];
	std::uint8_t * pRGB32Buf = new std::uint8_t[2100 * 2200 * 4];

	AVFrame * rgb24 = av_frame_alloc();
	AVFrame * rgb32 = av_frame_alloc();

	rgb24->format      = AV_PIX_FMT_BGR24;
	rgb24->width       = yuv->width;
	rgb24->height      = yuv->height;
	rgb24->data[0]     = pRGB24Buf;
	rgb24->linesize[0] = (((yuv->width + 31) >> 4) << 4) * 3;

	rgb32->format      = AV_PIX_FMT_BGRA;
	rgb32->width       = yuv->width;
	rgb32->height      = yuv->height;
	rgb32->data[0]     = pRGB32Buf;
	rgb32->linesize[0] = (((yuv->width + 31) >> 4) << 4) * 4;

	AVFrame * yuv2 = av_frame_alloc();
	yuv2->width  = 1920;
	yuv2->height = 1080;
	yuv2->format = AV_PIX_FMT_YUV420P;
	e = av_frame_get_buffer(yuv2, 32);
	if (e)
	{
		av_frame_free(&yuv2);
		return e;
	}

	AvScale sws2;
	AvScale rgb32_to_yuv;
	VideoRender r(::GetConsoleWindow(), yuv->width, yuv->height, D3DFMT_X8R8G8B8);
	//VideoRender yuvr(::GetConsoleWindow(), yuv2->width, yuv2->height, MAKEFOURCC('I', '4', '2', '0'));
	for (int i = 0; i < 1000; ++i)
	{
		FillYuvImage(yuv->data, yuv->linesize, yuv->width, yuv->height, i);
		//AVFrame * rgb24 = sws.Scale(yuv, AV_PIX_FMT_BGR24, yuv->width, yuv->height);
		//AVFrame * rgb32 = sws2.Scale(rgb24, AV_PIX_FMT_BGRA, yuv->width, yuv->height);

		sws.Scale(yuv, rgb24);

		int x = 100;
		int y = 100;
		int w = 100;
		int h = 100;

		int ex = x + w - 1;
		int ey = y + h - 1;

		if (x > rgb24->width ) x = rgb24->width  - 1;
		if (y > rgb24->height) y = rgb24->height - 1;
		if (x < 0) x = 0;
		if (y < 0) y = 0;

		if (ex >= rgb24->width ) ex = rgb24->width  - 1;
		if (ey >= rgb24->height) ey = rgb24->height - 1;
		if (ex < 0) ex = 0;
		if (ey < 0) ey = 0;

		// 顶线条
		//if (ex > x && ey > y)
		{
			std::uint8_t * p = rgb24->data[0] + y * rgb24->linesize[0] + x * 3;
			for (int i = 0; i < ex - x; ++i)
			{
				p[0] = 0;   // B
				p[1] = 0;   // G
				p[2] = 255; // R

				p += 3;
			}
		}

		// 底线条
		if (ey > 0)
		{
			std::uint8_t * p = rgb24->data[0] + ey * rgb24->linesize[0] + x * 3;
			for (int i = 0; i < ex - x; ++i)
			{
				p[0] = 0;   // B
				p[1] = 0;   // G
				p[2] = 255; // R

				p += 3;
			}
		}

		// 左线
		if (x >= 0 && x < rgb24->width)
		{
			std::uint8_t * p = rgb24->data[0] + y * rgb24->linesize[0] + x * 3;
			for (int i = 0; i < ey - y; ++i)
			{
				p[0] = 0; // B
				p[1] = 0; // G
				p[2] = 255; // R
				p += rgb24->linesize[0];
			}
		}

		// 右线
		if (ex >= 0 && ex < rgb24->width)
		{
			std::uint8_t * p = rgb24->data[0] + y * rgb24->linesize[0] + ex * 3;
			for (int i = 0; i < ey - y; ++i)
			{
				p[0] = 0; // B
				p[1] = 0; // G
				p[2] = 255; // R
				p += rgb24->linesize[0];
			}
		}
#if 0
		++x;
		++y;
		--ex;
		--ey;

		if (x > rgb24->width) x = rgb24->width - 1;
		if (y > rgb24->height) y = rgb24->height - 1;
		if (x < 0) x = 0;
		if (y < 0) y = 0;

		if (ex >= rgb24->width) ex = rgb24->width - 1;
		if (ey >= rgb24->height) ey = rgb24->height - 1;
		if (ex < 0) ex = 0;
		if (ey < 0) ey = 0;

		// 顶线条
//if (ex > x && ey > y)
		{
			std::uint8_t * p = rgb24->data[0] + y * rgb24->linesize[0] + x * 3;
			for (int i = 0; i < ex - x; ++i)
			{
				p[0] = 0;   // B
				p[1] = 0;   // G
				p[2] = 255; // R

				p += 3;
			}
		}

		// 底线条
		if (ey > 0)
		{
			std::uint8_t * p = rgb24->data[0] + ey * rgb24->linesize[0] + x * 3;
			for (int i = 0; i < ex - x; ++i)
			{
				p[0] = 0;   // B
				p[1] = 0;   // G
				p[2] = 255; // R

				p += 3;
			}
		}

		// 左线
		if (x >= 0 && x < rgb24->width)
		{
			std::uint8_t * p = rgb24->data[0] + y * rgb24->linesize[0] + x * 3;
			for (int i = 0; i < ey - y; ++i)
			{
				p[0] = 0; // B
				p[1] = 0; // G
				p[2] = 255; // R
				p += rgb24->linesize[0];
			}
		}

		// 右线
		if (ex >= 0 && ex < rgb24->width)
		{
			std::uint8_t * p = rgb24->data[0] + y * rgb24->linesize[0] + ex * 3;
			for (int i = 0; i < ey - y; ++i)
			{
				p[0] = 0; // B
				p[1] = 0; // G
				p[2] = 255; // R
				p += rgb24->linesize[0];
			}
		}

		++x;
		++y;
		--ex;
		--ey;

		if (x > rgb24->width) x = rgb24->width - 1;
		if (y > rgb24->height) y = rgb24->height - 1;
		if (x < 0) x = 0;
		if (y < 0) y = 0;

		if (ex >= rgb24->width) ex = rgb24->width - 1;
		if (ey >= rgb24->height) ey = rgb24->height - 1;
		if (ex < 0) ex = 0;
		if (ey < 0) ey = 0;

		// 顶线条
//if (ex > x && ey > y)
		{
			std::uint8_t * p = rgb24->data[0] + y * rgb24->linesize[0] + x * 3;
			for (int i = 0; i < ex - x; ++i)
			{
				p[0] = 0;   // B
				p[1] = 0;   // G
				p[2] = 255; // R

				p += 3;
			}
		}

		// 底线条
		if (ey > 0)
		{
			std::uint8_t * p = rgb24->data[0] + ey * rgb24->linesize[0] + x * 3;
			for (int i = 0; i < ex - x; ++i)
			{
				p[0] = 0;   // B
				p[1] = 0;   // G
				p[2] = 255; // R

				p += 3;
			}
		}

		// 左线
		if (x >= 0 && x < rgb24->width)
		{
			std::uint8_t * p = rgb24->data[0] + y * rgb24->linesize[0] + x * 3;
			for (int i = 0; i < ey - y; ++i)
			{
				p[0] = 0; // B
				p[1] = 0; // G
				p[2] = 255; // R
				p += rgb24->linesize[0];
			}
		}

		// 右线
		if (ex >= 0 && ex < rgb24->width)
		{
			std::uint8_t * p = rgb24->data[0] + y * rgb24->linesize[0] + ex * 3;
			for (int i = 0; i < ey - y; ++i)
			{
				p[0] = 0; // B
				p[1] = 0; // G
				p[2] = 255; // R
				p += rgb24->linesize[0];
			}
		}


		++x;
		++y;
		--ex;
		--ey;

		if (x > rgb24->width) x = rgb24->width - 1;
		if (y > rgb24->height) y = rgb24->height - 1;
		if (x < 0) x = 0;
		if (y < 0) y = 0;

		if (ex >= rgb24->width) ex = rgb24->width - 1;
		if (ey >= rgb24->height) ey = rgb24->height - 1;
		if (ex < 0) ex = 0;
		if (ey < 0) ey = 0;

		// 顶线条
//if (ex > x && ey > y)
		{
			std::uint8_t * p = rgb24->data[0] + y * rgb24->linesize[0] + x * 3;
			for (int i = 0; i < ex - x; ++i)
			{
				p[0] = 0;   // B
				p[1] = 0;   // G
				p[2] = 255; // R

				p += 3;
			}
		}

		// 底线条
		if (ey > 0)
		{
			std::uint8_t * p = rgb24->data[0] + ey * rgb24->linesize[0] + x * 3;
			for (int i = 0; i < ex - x; ++i)
			{
				p[0] = 0;   // B
				p[1] = 0;   // G
				p[2] = 255; // R

				p += 3;
			}
		}

		// 左线
		if (x >= 0 && x < rgb24->width)
		{
			std::uint8_t * p = rgb24->data[0] + y * rgb24->linesize[0] + x * 3;
			for (int i = 0; i < ey - y; ++i)
			{
				p[0] = 0; // B
				p[1] = 0; // G
				p[2] = 255; // R
				p += rgb24->linesize[0];
			}
		}

		// 右线
		if (ex >= 0 && ex < rgb24->width)
		{
			std::uint8_t * p = rgb24->data[0] + y * rgb24->linesize[0] + ex * 3;
			for (int i = 0; i < ey - y; ++i)
			{
				p[0] = 0; // B
				p[1] = 0; // G
				p[2] = 255; // R
				p += rgb24->linesize[0];
			}
		}
#endif

		sws.Scale(rgb24, rgb32);
		r.Render(rgb32->data, rgb32->linesize);

		//rgb32_to_yuv.Scale(rgb24, yuv2);
		//yuvr.Render(yuv2->data, yuv2->linesize);
		//r.Render(yuv->data, yuv->linesize);
		//printf("%dx%d %d %d\n", rgb->width, rgb->height, rgb->linesize[0], rgb->linesize[1]);
		// av_image_get_buffer_size
		Sleep(40);
	}

	delete[] pRGB24Buf;
	delete[] pRGB32Buf;

	av_frame_free(&rgb24);
	av_frame_free(&rgb32);
	av_frame_free(&yuv);

	getchar();

	return 0;
}




inline int frame_scale(AVFrame * src, AVFrame * dst)
{
	if (src == nullptr || dst == nullptr)
	{
		return -1;
	}

	SwsContext * sws = sws_getContext(
		src->width, src->height, (AVPixelFormat)src->format,
		dst->width, dst->height, (AVPixelFormat)dst->format,
		SWS_FAST_BILINEAR, NULL, NULL, NULL);

	if (sws == nullptr)
	{
		return -2;
	}

	int h = sws_scale(sws, src->data, src->linesize, 0, src->height, dst->data, dst->linesize);
	sws_freeContext(sws);

	if (h == dst->height)
	{
		return 0;
	}

	return -3;
}



int frame_to_jpeg(AVFrame * frame, std::string path)
{
	if (frame == nullptr)
	{
		return -100;
	}
	if (frame->format != AV_PIX_FMT_YUV420P && frame->format != AV_PIX_FMT_YUVJ420P)
	{
		return -200;
	}

	auto codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
	if (codec == nullptr)
	{
		return -1;
	}

	AVCodecContext * c = avcodec_alloc_context3(codec);
	c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	c->width  = frame->width;
	c->height = frame->height;
	/* frames per second */
	//c->time_base = AVRational{ 1, 25 };
	//c->framerate = AVRational{ 25, 1 };
	c->time_base = AVRational{ 1, 1 };
	c->framerate = AVRational{ 1, 1 };
	c->max_pixels = c->width * c->height * 2;
	/* emit one intra frame every ten frames
	 * check frame pict_type before passing frame
	 * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	 * then gop_size is ignored and the output of encoder
	 * will always be I frame irrespective to gop_size
	 */
	c->gop_size = 1;
	c->max_b_frames = 0;
	c->pix_fmt = frame->format == AV_PIX_FMT_YUV420P ? AV_PIX_FMT_YUVJ420P : (AVPixelFormat)frame->format;// AV_PIX_FMT_YUV420P;
	int e = avcodec_open2(c, codec, NULL);
	if (e < 0)
	{
		avcodec_free_context(&c);
		return -2;
	}

	e = avcodec_send_frame(c, frame);
	if (e >= 0)
	{
		AVPacket * pkt = av_packet_alloc();

		e = avcodec_receive_packet(c, pkt);

		if (e >= 0)
		{
			e = -150;
			std::ofstream o(path, std::ios::binary);
			if (o)
			{
				o.write((char*)pkt->data, pkt->size);
				e = o ? 0 : -250;
				o.close();
			}
		}

		av_packet_unref(pkt);
		av_packet_free(&pkt);
	}
	//av_frame_free(&v);

	avcodec_close(c);
	avcodec_free_context(&c);

	return e;
}

int crop_frame_to_jpeg(AVFrame * frame, int x, int y, int w, int h, std::string path)
{
	const AVFilter * buffersrc  = avfilter_get_by_name("buffer");
	const AVFilter * buffersink = avfilter_get_by_name("buffersink");

	if (buffersrc == nullptr || buffersink == nullptr)
	{
		return -1;
	}

	AVFilterInOut * outputs      = avfilter_inout_alloc();
	AVFilterInOut * inputs       = avfilter_inout_alloc();
	AVFilterGraph * filter_graph = avfilter_graph_alloc();

	AVFilterContext * buffersink_ctx = nullptr;
	AVFilterContext * buffersrc_ctx  = nullptr;

	/* buffer video source: the decoded frames from the decoder will be inserted here. */
	char args[512] = { 0 };
	AVRational time_base = { 1, 1 };
	AVRational sample_aspect_ratio = { frame->width, frame->height };
	/*snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		frame->width, frame->height, frame->format,
		time_base.num, time_base.den,
		sample_aspect_ratio.num, sample_aspect_ratio.den);*/
	snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d",
		frame->width, frame->height, frame->format,
		time_base.num, time_base.den);
	int e = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
	if (e < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
		goto end;
	}

	/* buffer video sink: to terminate the filter chain. */
	e = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
	if (e < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
		goto end;
	}

	enum AVPixelFormat pix_fmts[] = { (AVPixelFormat)frame->format, AV_PIX_FMT_NONE };
	e = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
	if (e < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
		goto end;
	}

	/*
	 * Set the endpoints for the filter graph. The filter_graph will
	 * be linked to the graph described by filters_descr.
	 */

	 /*
	  * The buffer source output must be connected to the input pad of
	  * the first filter described by filters_descr; since the first
	  * filter input label is not specified, it is set to "in" by
	  * default.
	  */
	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	/*
	 * The buffer sink input must be connected to the output pad of
	 * the last filter described by filters_descr; since the last
	 * filter output label is not specified, it is set to "out" by
	 * default.
	 */
	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;

	// crop=iw/2:ih/2:0:0
	char filters_descr[128] = { 0 };
	snprintf(filters_descr, sizeof(filters_descr), "crop=%d:%d:%d:%d", w, h, x, y);
	if ((e = avfilter_graph_parse_ptr(filter_graph, filters_descr, &inputs, &outputs, NULL)) < 0)
		goto end;

	if ((e = avfilter_graph_config(filter_graph, NULL)) < 0)
		goto end;


	{
		/* push the decoded frame into the filtergraph */
		if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
			goto end;
		}

		AVFrame * filt_frame = av_frame_alloc();
		/* pull filtered frames from the filtergraph */
		
		{
			e = av_buffersink_get_frame(buffersink_ctx, filt_frame);
			if (e == AVERROR(EAGAIN) || e == AVERROR_EOF)
				goto end;
			if (e < 0)
				goto end;
			e = frame_to_jpeg(filt_frame, path);
			av_frame_unref(filt_frame);
		}

		av_frame_free(&filt_frame);
	}

end:
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);
	avfilter_graph_free(&filter_graph);

	return e;
}

int main(int argc, char * argv[])
{
	AVFrame * yuv = av_frame_alloc();
	yuv->width  = 1920;
	yuv->height = 1080;
	yuv->format = AV_PIX_FMT_YUV420P;
	int e = av_frame_get_buffer(yuv, 32);
	if (e)
	{
		av_frame_free(&yuv);
		return e;
	}

	VideoRender r(::GetConsoleWindow(), yuv->width, yuv->height, MAKEFOURCC('I', '4', '2', '0'));

	for (int i = 0; i < 1000; ++i)
	{
		FillYuvImage(yuv->data, yuv->linesize, yuv->width, yuv->height, i);

		r.Render(yuv->data, yuv->linesize);
		//if (i == 0)
		/*{
			RECT rc = { 0, 0, 0, 0 };
			rc.left = 50;
			rc.right = 150;
			rc.top = 50;
			rc.bottom = 150;
			r.SnapShot("mmmm.jpg", NULL);
		}*/
		if (i == 0)
		{
			//crop_frame_to_jpeg(yuv, 50, 50, 100, 100);
			//frame_to_jpeg(yuv);
		}
		//exit(0);
		//printf("%dx%d %d %d\n", rgb->width, rgb->height, rgb->linesize[0], rgb->linesize[1]);
		// av_image_get_buffer_size
		//getchar();
		Sleep(40);
	}

	av_frame_free(&yuv);

	getchar();

	return 0;
}
