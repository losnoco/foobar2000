#include "wma.h"

#include "file.h"

class static_init
{
public:
	static_init()
	{
		av_register_all();

		register_protocol(&fb2k_protocol);
	}

	~static_init() {}
};

static static_init meh;

input_wma::input_wma()
{
	c = NULL;
	ic = NULL;
	stream_index = 0;
}

input_wma::~input_wma()
{
	if (c) avcodec_close(c);
	if (ic) av_close_input_file(ic);
}

bool input_wma::test_filename(const char * fn,const char * ext) 
{
	return (!stricmp(ext,"WMA") || !stricmp(ext,"ASF") || !stricmp(ext,"WMV"));
}

bool input_wma::open(reader * r,file_info * info,unsigned flags)
{
	int i;
	string8 path;
	AVCodec * codec;
	AVCodecContext * ctx;

	path = "foobar:";
	path += info->get_file_path();

	if (av_open_input_file(&ic, path, NULL, 0, NULL) < 0)
	{
		console::error("Could not open file.");
		return 0;
	}

	for (i = 0; i < ic->nb_streams; i++)
	{
		ctx = &ic->streams[i]->codec;
		if (ctx->codec_type == CODEC_TYPE_AUDIO)
			break;
	}

	stream_index = i;

	if (i == ic->nb_streams)
	{
		console::error("No audio streams.");
		return 0;
	}

	av_find_stream_info(ic);
	codec = avcodec_find_decoder(ctx->codec_id);
	if (!codec)
	{
		console::error("Unexpected audio format.");
		return 0;
	}

	if (flags & OPEN_FLAG_DECODE)
	{
		if (avcodec_open(ctx, codec) < 0)
		{
			console::error("Could not open codec.");
			return 0;
		}
		c = ctx;
	}

	if (flags & OPEN_FLAG_GET_INFO)
	{
		if (ic->title[0]) info->meta_add("title", ic->title);
		if (ic->author[0]) info->meta_add("artist", ic->author);
		if (ic->album[0]) info->meta_add("album", ic->album);
		if (ic->year) info->meta_add("date", uStringPrintf("%d", ic->year));
		if (ic->track) info->meta_add("tracknumber", uStringPrintf("%d", ic->track));
		if (ic->genre[0]) info->meta_add("genre", ic->genre);
		if (ic->copyright[0]) info->meta_add("copyright", ic->copyright);
		if (ic->comment[0]) info->meta_add("comment", ic->comment);

		if (!stricmp(codec->name, "wmav1")) info->info_set("codec", "Windows Media Audio V7");
		else if (!stricmp(codec->name, "wmav2")) info->info_set("codec", "Windows Media Audio V8");
		
		if (ctx->sample_rate) info->info_set_int("samplerate", ctx->sample_rate);
		if (ctx->channels) info->info_set_int("channels", ctx->channels);

		if (ctx->bit_rate) info->info_set_int("bitrate", ctx->bit_rate / 1000);

		if (ic->duration != 0)
		{
			info->set_length(double(ic->duration) * (1. / 1000000.));
		}
	}

	return 1;
}

input::set_info_t input_wma::set_info(reader *r,const file_info * info)
{
	return SET_INFO_FAILURE;
}

int input_wma::run(audio_chunk * chunk)
{
	int len, out_size, written;
	AVPacket pkt;

	do
	{
		if (av_read_frame(ic, &pkt) < 0) return 0;
		if (!pkt.size) return 0;
	}
	while (pkt.stream_index != stream_index);

	written = 0;

	if (pkt.data && pkt.size > 0)
	{
		len = avcodec_decode_audio(c,
			sample_buffer.check_size(AVCODEC_MAX_AUDIO_FRAME_SIZE / sizeof(float)),
			&out_size, pkt.data, pkt.size);

		if (len < 0) return -1;

		written = out_size / (sizeof(float) * c->channels);
	}

	if (pkt.data) av_free_packet(&pkt);

	if (written)
	{
		chunk->set_data_32(sample_buffer.get_ptr(), written, c->channels, c->sample_rate);
		return 1;
	}

	return 0;
}

bool input_wma::seek(double seconds)
{
	/*if (!ic || av_seek_frame(ic, stream_index, int64_t(seconds * 1000000.)) < 0) return false;
	return true;*/
	return false;
}

DECLARE_COMPONENT_VERSION("WMA Decoder", "1.0", "Based on stripped-down libavcodec from\nthe wma2wav portion of xmms-wma.\n\nhttp://mcmcc.bat.ru/xmms-wma/\nhttp://ffmpeg.sourceforge.net/");
