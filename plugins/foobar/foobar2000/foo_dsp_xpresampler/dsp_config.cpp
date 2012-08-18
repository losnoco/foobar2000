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

#include "stdafx.h"

const int STRMAXLEN = 20;
const int RATESLISTLEN = 200;

static const long samplerates[] = {8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000, 176400, 192000};

enum TargetRates
{
	TRuninit = 0,
	TRerror = -1,

	TRupsample_2 = -2,
	TRupsample_4 = -5,

	TRdnsample_2 = -3,
	TRdnsample_4 = -4,

	FromList = -6,
	//FromListAlt = -7
};

static const TCHAR* srates_z[] = {0, 0,
					_T("Upsample x2"), _T("Downsample x2"),
					_T("Downsample x4"), _T("Upsample x4"),
					/*_T("[From the list]"), _T("From the list (alt)")*/ };


static const TCHAR* samplerates_s[] = {_T("8000"), _T("11025"), _T("16000"), _T("22050"), _T("24000"), _T("32000"),
					_T("44100"), _T("48000"), _T("64000"), _T("88200"), _T("96000"), _T("176400"), _T("192000"),
					srates_z[2], srates_z[5], srates_z[3], srates_z[4]/*, srates_z[6], srates_z[7]*/};
static const TCHAR* qualities[] = {_T("Best"), _T("Good"), _T("Normal")};

unsigned RateConfig::realrate(unsigned in_samplerate) const
{
	if (outRate > 0) return outRate;
	switch(outRate)
	{
	case TRupsample_2:
		return in_samplerate*2;
	case TRupsample_4:
		return in_samplerate*4;
	case TRdnsample_2:
		return in_samplerate/2;
	case TRdnsample_4:
		return in_samplerate/4;

	case FromList:
	//case FromListAlt:
		if (no_resample_rates.get_count() == 0) return in_samplerate; //empty list - what else can we do?
		for (unsigned i = 0; i < no_resample_rates.get_count(); i++) if (no_resample_rates[i] == in_samplerate) return in_samplerate;

		/*if(outRate == FromListAlt)
		{
			if (no_resample_rates.have_item(in_samplerate*2)) return in_samplerate*2;
			if (no_resample_rates.have_item(in_samplerate*4)) return in_samplerate*4;
		}*/

		t_uint32 r, fr; r = fr = no_resample_rates[0];
		for(unsigned int i=1; i < no_resample_rates.get_count(); i++)
		{
			r = no_resample_rates[i];
			if (r >= in_samplerate)
			{
				if(fr > r) fr = r;
				else if (fr < in_samplerate) fr = r;
			}
			else //(r < in_samplerate)
			{
				if(fr < r) fr = r;
			}
		}//end for
		return fr;
	}
	return in_samplerate; //something strange
}

t_dsp_rate_params::t_dsp_rate_params()
{
	cfg_.outRate        = 44100;
}

void t_dsp_rate_params::get_rateconfig(RateConfig& cfg) const
{
	cfg_.CloneTo(cfg);
}

bool t_dsp_rate_params::set_data(const dsp_preset& p_data)
{
	t_int32* temp;
	unsigned int datasize;
	t_size version;

	if (p_data.get_owner() != g_get_guid()) return false;

	datasize = p_data.get_data_size()/sizeof(t_int32);
	temp = new t_int32[datasize];
	memcpy(temp, p_data.get_data(), datasize*sizeof(t_int32));

	for(unsigned int i=0; i<datasize; i++)
		byte_order::order_le_to_native_t(temp[i]);
		
	/*if(datasize == 5)
	{	//from previous versions
		cfg_.outRate        = temp[0];
		cfg_.allow_aliasing = temp[2];
		cfg_.passband10     = temp[3];
		cfg_.phase          = temp[4];
		if(cfg_.passband10 == 0 || cfg_.passband10 == 1) //bool steepness -- from old version (0.3.2)
		{
			cfg_.passband10 = cfg_.passband10?maxPassband10:defPassband10;
			cfg_.phase *= 25;
		}
	}
	else*/
	{
		version             = temp[0]; //useless atm.

		cfg_.outRate        = temp[1];

		cfg_.no_resample_rates.set_count(0);
		for(unsigned int i=2; i<datasize; i++)
			cfg_.no_resample_rates.append_single(temp[i]);
	}

	delete[] temp;
	return true;
}

bool t_dsp_rate_params::get_data(dsp_preset& p_data) const
{
	t_int32* temp;
	t_size datasize;
	datasize = 2 + cfg_.no_resample_rates.get_count();
	temp = new t_int32[datasize];

	temp[0] = RESAMPLER_VERSION;
	temp[1] = cfg_.outRate;
	for(unsigned int i=2; i<datasize; i++)
		temp[i] = cfg_.no_resample_rates[i-2];

	p_data.set_owner(g_get_guid());
	for(unsigned int i=0; i<datasize; i++)
		byte_order::order_native_to_le_t(temp[i]);
	p_data.set_data(temp, datasize*sizeof(t_int32));

	delete[] temp;
	return true;
}

void t_dsp_rate_params::tset_outRate(const TCHAR* rate)
{
	for(int i = -TRupsample_2; i<= -FromList/*Alt*/; i++)
	{
		if (_tcscmp(rate, srates_z[i]) == 0) {
		cfg_.outRate = -i; return; }
	}

	set_outRate(_ttoi(rate));
}

const TCHAR* t_dsp_rate_params::toutRateStr(TCHAR* buf) const
{
	if (cfg_.outRate < 0) //cfg_.outRate = [-z...-y].
	{
		assert(cfg_.outRate >= FromList/*Alt*/ && cfg_.outRate <= TRupsample_2);
		_tcsncpy_s(buf, 30, srates_z[-cfg_.outRate], _TRUNCATE);
		return buf;
	}

	_itot_s(cfg_.outRate, buf, STRMAXLEN-1, 10);
	return buf;
}


void t_dsp_rate_params::tfill_freqlist(const TCHAR* buf)
{
	int i=0;
	
	clear_noresamplefreq();

	while(true)
	{
		for (; buf[i] != _T('\0'); i++)
			if(_istdigit(buf[i])) break;

		if (buf[i] == _T('\0')) break;

		int freq, n, d;
		d=_stscanf(buf+i, _T("%d%n"), &freq, &n);
		if (d==EOF) break;
		if (n==0 || d==0) break;
		if (freq > 0) 
		{
			if (freq < audio_chunk::sample_rate_min) freq = audio_chunk::sample_rate_min;
			else if (freq > audio_chunk::sample_rate_max) freq = audio_chunk::sample_rate_max;
			add_noresamplefreq(freq);
		}
		i+=n;
	}
}


BOOL dialog_dsp_rate::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	TCHAR buf[30];

	CComboBox rateCombo(GetDlgItem(IDC_RATE));
	CEdit noresamples(GetDlgItem(IDC_FREQLIST));
	
	rateCombo.LimitText(12);
	for(int i=0; i<sizeof(samplerates_s)/sizeof(samplerates_s[0]); i++)
		rateCombo.AddString((samplerates_s[i]));
	params_.toutRateStr(buf);
	for(int i=0; i<sizeof(samplerates_s)/sizeof(samplerates_s[0]); i++)
	{
		if (!_tcscmp(samplerates_s[i], buf)) { rateCombo.SetCurSel(i); break; }
	}
	rateCombo.SetWindowTextW(buf);

	noresamples.SetLimitText(RATESLISTLEN-1);
	for(int i = 0; i < params_.getNnoresamplefreqs(); i++)
	{
		int fr = params_.getnoresamplefreq(i);
		if(i < params_.getNnoresamplefreqs()-1)
			_sntprintf(buf, sizeof(buf)/sizeof(TCHAR), _T("%d;"), fr);
		else
			_sntprintf(buf, sizeof(buf)/sizeof(TCHAR), _T("%d"), fr);
		noresamples.AppendText(buf);
	}

	return 0;
}

void dialog_dsp_rate::OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	switch (nID)
	{
	case IDOK:
		{
			TCHAR str[STRMAXLEN];
			TCHAR buf[RATESLISTLEN];

			CComboBox rateCombo(GetDlgItem(IDC_RATE));
			CEdit noresamples(GetDlgItem(IDC_FREQLIST));

			rateCombo.GetWindowTextW(str, STRMAXLEN);
			params_.tset_outRate(str);

			noresamples.GetWindowTextW(buf, RATESLISTLEN-1);
			params_.tfill_freqlist(buf);

			// Data (potentially) changed
			EndDialog(IDOK);
		}
		break;

		case IDCANCEL:
		{
			// Data not changed
			EndDialog(0);
		}
		break;

		/*case IDC_DEFAULTS:
		{
			TCHAR buf[30];
			//CComboBox rateCombo(GetDlgItem(IDC_RATE));
			CComboBox qualCombo(GetDlgItem(IDC_QUALITY));
			CButton aliasCheckbox(GetDlgItem(IDC_ALIASING));
			CTrackBarCtrl passbandSlider(GetDlgItem(IDC_PASSBAND));
			CTrackBarCtrl phresponseSlider(GetDlgItem(IDC_PHRESPONSE));
			CStatic passbandText(GetDlgItem(IDC_BWTEXT));
			CStatic phresponseText(GetDlgItem(IDC_PHASETEXT));
			CEdit noresamples(GetDlgItem(IDC_FREQLIST));
			
			qualCombo.SetCurSel(Qbest);

			t_dsp_rate_params params;

			aliasCheckbox.SetCheck(params.aliasing());

			passbandSlider.SetPos(params.passband10());
			_sntprintf(buf, sizeof(buf)/sizeof(TCHAR), _T("%2.1f %%"), float(params.passband10()/10.0));
			passbandText.SetWindowTextW(buf);

			phresponseSlider.SetPos(params.phase());
			update_phresponseText(buf, sizeof(buf)/sizeof(TCHAR), phresponseText, (int)params.phase());

			noresamples.SetWindowTextW(_T(""));
		}
		break;*/
	}
}
