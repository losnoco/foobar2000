/* Copyright (c) 2008-11 lvqcl.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */ 

#ifndef DSPRATECONFIG_H
#define DSPRATECONFIG_H

#include "../ATLhelpers/ATLhelpers.h"
#include "../SDK/foobar2000.h"

#include "resource.h"



#define RESAMPLER_MAJOR_VERSION  0 /* Major version number */
#define RESAMPLER_MINOR_VERSION  1 /* Minor version number */
#define RESAMPLER_PATCH_VERSION  0/* Patch level */
#define RESAMPLER_BUILD_VERSION  0 /* Build number */

#define RESAMPLER_VERSION (RESAMPLER_MAJOR_VERSION*0x10000 + RESAMPLER_MINOR_VERSION*0x100 + RESAMPLER_PATCH_VERSION)

#ifndef STR
#define STR__(x)  #x
#define STR(x)    STR__(x)
#endif

#if defined(RESAMPLER_ALPHA_VERSION)
#define RESAMPLER_VERSION_STR STR(RESAMPLER_MAJOR_VERSION)"."STR(RESAMPLER_MINOR_VERSION)"."STR(RESAMPLER_PATCH_VERSION)" alpha "STR(RESAMPLER_ALPHA_VERSION)" mod2"
#elif defined(RESAMPLER_BETA_VERSION)
#define RESAMPLER_VERSION_STR STR(RESAMPLER_MAJOR_VERSION)"."STR(RESAMPLER_MINOR_VERSION)"."STR(RESAMPLER_PATCH_VERSION)" beta "STR(RESAMPLER_BETA_VERSION)" mod2"
#else
#define RESAMPLER_VERSION_STR STR(RESAMPLER_MAJOR_VERSION)"."STR(RESAMPLER_MINOR_VERSION)"."STR(RESAMPLER_PATCH_VERSION)" mod2"
#endif

class RateConfig
{
public:
	t_int32 outRate;
	pfc::list_t<t_uint32> no_resample_rates;

	unsigned realrate(unsigned in_samplerate) const;

	bool is_no_resample(unsigned in_samplerate) const
	{
		if (in_samplerate == outRate) return true;
		if (no_resample_rates.get_count() == 0) return false;
		if (no_resample_rates.have_item(in_samplerate)) return false;
		return true;
	}

	void CloneTo(RateConfig& to) const
	{
		to.outRate = outRate;
		to.no_resample_rates = no_resample_rates;
	}
};

class t_dsp_rate_params
{
	RateConfig cfg_;
public:	
	t_dsp_rate_params();
	t_dsp_rate_params(const RateConfig& cfg):cfg_(cfg){}
	void get_rateconfig(RateConfig& cfg) const;

	static const GUID &g_get_guid()
	{	// {F49E770F-FA7B-4B27-9386-468F1BD9FD15}
		static const GUID guid = { 0xf49e770f, 0xfa7b, 0x4b27, { 0x93, 0x86, 0x46, 0x8f, 0x1b, 0xd9, 0xfd, 0x15 } };
		return guid;
	}

	//store data from preset
	bool set_data(const dsp_preset& p_data);
	//put data to preset
	bool get_data(dsp_preset& p_data) const;

	//inline t_int32 outRate(...) const { return cfg_.outRate; }
	inline int getNnoresamplefreqs() const { return cfg_.no_resample_rates.get_count(); }
	inline int getnoresamplefreq(int i) const { return cfg_.no_resample_rates.get_item(i); }
	const TCHAR* toutRateStr(TCHAR*) const;

	void tset_outRate(const TCHAR* rate);
	inline void set_outRate(t_int32 rate)
	{
		if (rate < audio_chunk::sample_rate_min) rate = audio_chunk::sample_rate_min;
		else if (rate > audio_chunk::sample_rate_max) rate = audio_chunk::sample_rate_max;
		cfg_.outRate = rate;
	}
	inline void clear_noresamplefreq(){ cfg_.no_resample_rates.remove_all(); }
	inline void add_noresamplefreq(t_int32 freq) { cfg_.no_resample_rates.add_item(freq); }
	void tfill_freqlist(const TCHAR* str);
};

class dialog_dsp_rate : public CDialogImpl<dialog_dsp_rate>
{
public:
	enum
	{
		IDD = IDD_CONFIG
	};
 
	BEGIN_MSG_MAP_EX(dialog_dsp_rate)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_COMMAND(OnCommand)
	END_MSG_MAP()

private:
	t_dsp_rate_params& params_;

	void update_phresponseText(TCHAR* buf, int sz, CStatic phresponseText, int val);

public:
	dialog_dsp_rate(t_dsp_rate_params& p_params) : params_(p_params) {}

protected:
	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl);
};

#endif
