/* Copyright (c) 2008-10 lvqcl.  All rights reserved.
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

#include "stdafx.h"

class dsp_rate_entry : public resampler_entry
{
public:
	dsp_rate_entry() {};
	virtual void get_name(pfc::string_base& p_out) { p_out.set_string("Resampler (XP)"); }
	virtual GUID get_guid() { return t_dsp_rate_params::g_get_guid(); }
	virtual bool have_config_popup() { return true; }
	virtual float get_priority() { return 0; }
	virtual bool is_conversion_supported(unsigned int src_srate, unsigned int dst_srate) { return false; }

	virtual bool get_default_preset(dsp_preset& p_out);
	virtual bool create_preset(dsp_preset& p_out, unsigned p_target_srate, float p_qualityscale);
	
	virtual bool instantiate(service_ptr_t<dsp>& p_out, const dsp_preset& p_preset);

	virtual bool show_config_popup(dsp_preset& p_data, HWND p_parent);
};

bool dsp_rate_entry::get_default_preset(dsp_preset& p_out)
{
	t_dsp_rate_params().get_data(p_out);
	return true;
}

bool dsp_rate_entry::create_preset(dsp_preset& p_out, unsigned p_target_srate, float p_qualityscale)
{
	t_dsp_rate_params params;
	
	params.set_outRate(p_target_srate);
	
	params.get_data(p_out);

	return true;
}

bool dsp_rate_entry::instantiate(service_ptr_t<dsp>& p_out, const dsp_preset& p_preset)
{
	bool ret = false;
	if (p_preset.get_owner() == get_guid())
	{
		t_dsp_rate_params params;
		params.set_data(p_preset);
		p_out = reinterpret_cast<dsp*>(new service_impl_t<dsp_rate>(params));
		ret = p_out.is_valid();
	}
	return ret;
}

bool dsp_rate_entry::show_config_popup(dsp_preset& p_data, HWND p_parent)
{
	t_dsp_rate_params params;
	if (params.set_data(p_data))
	{
		dialog_dsp_rate dlg(params);
		if (dlg.DoModal(p_parent) == IDOK)
		{
			params.get_data(p_data);
			return true;
		}
	}
	return false;
}



DECLARE_COMPONENT_VERSION(
	"Windows XP Resampler",
	RESAMPLER_VERSION_STR,
	"Uses Windows XP \"best\" resampling algorithm, helpfully ripped off by the Wine team.\n\n"
	"DSP plug-in for foobar2000;  written by Chris Moeller, loosely based on code by lvqcl.\n\n"
	"https://www.patreon.com/kode54"
);

static service_factory_t<dsp_rate_entry>	foo_dsp_rate;
