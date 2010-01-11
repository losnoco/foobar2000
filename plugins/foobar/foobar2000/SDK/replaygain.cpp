#include "foobar2000.h"

void t_replaygain_config::reset()
{
	m_source_mode = source_mode_none;
	m_processing_mode = processing_mode_none;
	m_preamp_without_rg = 0;
	m_preamp_with_rg = 0;
}

double t_replaygain_config::query_scale(const file_info & p_info) const
{
	const double peak_margin = 1.0;//used to be 0.999 but it must not trigger on lossless

	double peak = peak_margin;
	double gain = 0;

	bool have_rg_gain = false, have_rg_peak = false;

	if (m_source_mode == source_mode_track || m_source_mode == source_mode_album)
	{
		replaygain_info info = p_info.get_replaygain();
		float gain_select = replaygain_info::gain_invalid, peak_select = replaygain_info::peak_invalid;
		if (m_source_mode == source_mode_track)
		{
			if (info.is_track_gain_present()) {gain = info.m_track_gain; have_rg_gain = true; }
			else if (info.is_album_gain_present()) {gain = info.m_album_gain; have_rg_gain = true; }
			if (info.is_track_peak_present()) {peak = info.m_track_peak; have_rg_peak = true; }
			else if (info.is_album_peak_present()) {peak = info.m_album_peak; have_rg_peak = true; }
		}
		else
		{
			if (info.is_album_gain_present()) {gain = info.m_album_gain; have_rg_gain = true; }
			else if (info.is_track_gain_present()) {gain = info.m_track_gain; have_rg_gain = true; }
			if (info.is_album_peak_present()) {peak = info.m_album_peak; have_rg_peak = true; }
			else if (info.is_track_peak_present()) {peak = info.m_track_peak; have_rg_peak = true; }
		}
	}

	gain += have_rg_gain ? m_preamp_with_rg : m_preamp_without_rg;

	audio_sample scale = 1.0;

	if (m_processing_mode == processing_mode_gain || m_processing_mode == processing_mode_gain_and_peak)
	{
		scale *= pow(10.0,gain / 20.0);
	}

	if (m_processing_mode == processing_mode_peak || m_processing_mode == processing_mode_gain_and_peak)
	{
		if (scale * peak > peak_margin)
			scale = (audio_sample)(peak_margin / peak);
	}

	return scale;
}

audio_sample t_replaygain_config::query_scale(const metadb_handle_ptr & p_object) const
{
	audio_sample rv = 1.0;
	p_object->metadb_lock();
	const file_info * info;
	if (p_object->get_info_async_locked(info))
		rv = query_scale(*info);
	p_object->metadb_unlock();
	return rv;
}

audio_sample replaygain_manager::core_settings_query_scale(const file_info & p_info)
{
	t_replaygain_config temp;
	get_core_settings(temp);
	return temp.query_scale(p_info);
}

audio_sample replaygain_manager::core_settings_query_scale(const metadb_handle_ptr & info)
{
	t_replaygain_config temp;
	get_core_settings(temp);
	return temp.query_scale(info);
}
