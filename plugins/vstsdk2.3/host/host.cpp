// host.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

struct VstSysExEvent			// a generic timestamped event
{
	long type;			// see enum below
	long byteSize;		// of this event, excl. type and byteSize
	long deltaFrames;	// sample frames related to the current block start sample position
	long flags;			// generic flags, none defined yet (0)

	long   dataSize;
	long   unknown;
	void * data;
};


long VSTCALLBACK callback(AEffect *effect, long opcode, long index, long value, void *ptr, float opt)
{
	_ftprintf(stderr, _T("audioMaster callback: %u\n"), opcode);
	switch (opcode)
	{
	case audioMasterVersion:
		return 2300;

	case audioMasterCurrentId:
		if (effect) return effect->uniqueID;
		break;

	case audioMasterGetVendorString:
		strncpy((char *)ptr, "Chris Moeller", 64);
		//strncpy((char *)ptr, "YAMAHA", 64);
		break;

	case audioMasterGetProductString:
		strncpy((char *)ptr, "VSTi test host", 64);
		//strncpy((char *)ptr, "SOL/SQ01", 64);
		break;

	case audioMasterGetVendorVersion:
		//return 1000;
		return 0;

	case audioMasterGetLanguage:
		return kVstLangEnglish;

	case audioMasterWillReplaceOrAccumulate:
		return 1;
	}

	return 0;
}

typedef AEffect * (*main_func)(audioMasterCallback audioMaster);

int _tmain(int argc, _TCHAR* argv[])
{
	HMODULE hdll = LoadLibrary(_T("c:\\program files\\steinberg\\vstplugins\\yamaha\\s-yxg50.dll"));

	if (hdll)
	{
		main_func pmain = (main_func)GetProcAddress(hdll, "main");

		AEffect * peff = (*pmain)(&callback);

		if (peff)
		{
			peff->dispatcher(peff, effOpen, 0, 0, 0, 0);

			if (peff->dispatcher(peff, effGetPlugCategory, 0, 0, 0, 0) == kPlugCategSynth)
			{
				/*peff->dispatcher(peff, effSetSampleRate, 0, 0, 0, 44100.);
				peff->dispatcher(peff, effSetBlockSize, 0, 4096, 0, 0);*/

				char info[65];

				memset(info, 0, sizeof(info));
				peff->dispatcher(peff, effGetVendorString, 0, 0, &info, 0);
				fprintf(stderr, "Vendor: %s\n", &info);

				memset(info, 0, sizeof(info));
				peff->dispatcher(peff, effGetProductString, 0, 0, &info, 0);
				fprintf(stderr, "Product: %s\n", &info);

				long ver = peff->dispatcher(peff, effGetVendorVersion, 0, 0, 0, 0);
				if (ver)
				{
					fprintf(stderr, "Version: %x.%02x\n", ver / 256, ver & 255);
				}

				if (peff->initialDelay)
				{
					fprintf(stderr, "Initial delay: %u\n", peff->initialDelay);
				}

				if (peff->dispatcher(peff, effCanDo, 0, 0, "sendVstMidiEvent", 0))
				{
					_fputts(_T("MIDI events supported\n"), stderr);

					if (argc != 3)
					{
						VstMidiEvent ev[2];

						memset(ev, 0, sizeof(ev));

						ev[0].type = kVstMidiType;
						ev[0].byteSize = sizeof(ev[0]);
						ev[0].midiData[0] = -64;
						ev[0].midiData[1] = 0x07;

						ev[1].type = kVstMidiType;
						ev[1].byteSize = sizeof(ev[1]);
						ev[1].midiData[0] = -112;
						ev[1].midiData[1] = 0x40;
						ev[1].midiData[2] = 0x7F;

						VstEvents * evs = (VstEvents *) malloc(sizeof(long) * 2 + sizeof(VstEvent*) * 2);

						evs->numEvents = 2;
						evs->reserved = 0;
						evs->events[0] = (VstEvent*) &ev[0];
						evs->events[1] = (VstEvent*) &ev[1];

						peff->dispatcher(peff, effSetSampleRate, 0, 0, 0, 44100);
						peff->dispatcher(peff, effSetBlockSize, 0, 4096, 0, 0);

						peff->dispatcher(peff, effStartProcess, 0, 0, 0, 0);

						peff->dispatcher(peff, effProcessEvents, 0, 0, evs, 0);

						float ** pfloatlist = (float**) malloc(sizeof(float*) * (peff->numInputs + peff->numOutputs));
						float * pfloat = (float*) malloc(sizeof(float) * 4096);
						float * pfloat_out = (float*) malloc(sizeof(float) * 4096 * peff->numOutputs);

						for (long i = 0; i < peff->numInputs; i++)
						{
							pfloatlist[i] = pfloat;
						}

						for (long i = 0; i < peff->numOutputs; i++)
						{
							pfloatlist[i + peff->numInputs] = pfloat_out + 4096 * i;
						}

						memset(pfloat, 0, sizeof(float) * 4096);

						if (peff->processReplacing)
						{
							peff->processReplacing(peff, pfloatlist, pfloatlist + peff->numInputs, 4096);
						}
						else
						{
							memset(pfloat_out, 0, sizeof(float) * 4096 * peff->numOutputs);
							peff->process(peff, pfloatlist, pfloatlist + peff->numInputs, 4096);
						}

						free(evs);

						peff->dispatcher(peff, effStopProcess, 0, 0, 0, 0);

						float * pfloati = (float*) malloc(sizeof(float) * 4096 * peff->numOutputs);

						for (unsigned i = 0; i < 4096; i++)
						{
							for (unsigned j = 0; j < peff->numOutputs; j++)
							{
								pfloati[i * peff->numOutputs + j] = pfloat_out[i + j * 4096];
							}
						}

						FILE * f = _tfopen(_T("test.out"), _T("wb"));
						if (f)
						{
							fwrite(pfloati, sizeof(float), 4096 * peff->numOutputs, f);
							fclose(f);
						}

						free(pfloati);
						free(pfloat_out);
						free(pfloat);
						free(pfloatlist);
					}
					else
					{
						FILE * f = _tfopen(argv[1], _T("rb"));

						if (f)
						{
							fseek(f, 0, SEEK_END);
							long len = ftell(f);
							fseek(f, 0, SEEK_SET);
							void * pdata = malloc(len);
							fread(pdata, 1, len, f);
							fclose(f);

							MIDI_file * mf = MIDI_file::Create(pdata, len);
							if (mf)
							{
								float ** pfloatlist = (float**) malloc(sizeof(float*) * (peff->numInputs + peff->numOutputs));
								float * pfloat = (float*) malloc(sizeof(float) * 4096);
								float * pfloat_out = (float*) malloc(sizeof(float) * 4096 * peff->numOutputs);
								float * pfloat_i = (float*) malloc(sizeof(float) * 4096 * peff->numOutputs);

								memset(pfloat, 0, sizeof(float) * 4096);

								for (long i = 0; i < peff->numInputs; i++)
								{
									pfloatlist[i] = pfloat;
								}

								for (long i = 0; i < peff->numOutputs; i++)
								{
									pfloatlist[i + peff->numInputs] = pfloat_out + 4096 * i;
								}

								UINT count;
								MIDI_EVENT * pstream = do_table(mf, 1, &count, 0);
								if (pstream && count)
								{
									for (UINT i = 0; i < count; i++)
									{
										DWORDLONG foo = UInt32x32To64(pstream[i].tm, 44100);
										pstream[i].tm = (DWORD)(foo / 1000);
									}

									f = _tfopen(argv[2], _T("wb"));
									if (f)
									{
										peff->dispatcher(peff, effSetSampleRate, 0, 0, 0, 44100);
										peff->dispatcher(peff, effSetBlockSize, 0, 4096, 0, 0);

										peff->dispatcher(peff, effStartProcess, 0, 0, 0, 0);

										UINT pos = 0;
										DWORD tm = 0;

										while (pos < count)
										{
											DWORD tt = tm + 4096;
											UINT pend = pos, ptest;
											VstEvents * pevs = 0;

											while (pend < count && pstream[pend].tm < tt) pend++;

											if (pend > pos)
											{
												pevs = (VstEvents*) malloc(sizeof(long) * 2 + sizeof(VstEvent*) * (pend - pos));

												//_ftprintf(stderr, _T("Processing %u events...\n"), ev_count);

												pevs->numEvents = pend - pos;
												pevs->reserved = 0;

												for (ptest = 0; pos < pend; pos++, ptest++)
												{
													if (!(pstream[pos].ev & 0xFF000000))
													{
														VstMidiEvent * pev = (VstMidiEvent*) malloc(sizeof(VstMidiEvent));
														memset(pev, 0, sizeof(VstMidiEvent));
														pev->type = kVstMidiType;
														pev->byteSize = sizeof(VstMidiEvent);
														pev->deltaFrames = pstream[pos].tm - tm;
														memcpy(&pev->midiData, &pstream[pos].ev, 4);
														pevs->events[ptest] = (VstEvent*) pev;

														//_ftprintf(stderr, _T("event[%u] - deltaFrames: %u, midiData: %02X %02X %02X\n"), ptest, pev[ptest].deltaFrames, (unsigned char)pev[ptest].midiData[0], (unsigned char)pev[ptest].midiData[1], (unsigned char)pev[ptest].midiData[2]);
													}
													else
													{
														UINT nr = pstream[pos].ev & 0x7fffffff;
														CSysexMap * smap = mf->smap;
														SYSEX_ENTRY & syx = smap->events[nr];
														VstSysExEvent * pev = (VstSysExEvent*) malloc(sizeof(VstSysExEvent));
														pev->type = kVstSysExType;
														pev->byteSize = sizeof(long) * 4 + syx.len;
														pev->deltaFrames = pstream[pos].tm - tm;
														pev->flags = 0;
														/*memcpy(&pev->data, &syx.len, 4);
														*((DWORD*)(&pev->data + 8)) = (DWORD)(&pev->data + 16);
														memcpy(&pev->data + 16, smap->data + syx.ofs, syx.len);*/
														//memcpy(&pev->data, smap->data + syx.ofs, syx.len);
														pev->data = smap->data + syx.ofs;
														pev->dataSize = syx.len;
														pevs->events[ptest] = (VstEvent*) pev;
													}
												}

												peff->dispatcher(peff, effProcessEvents, 0, 0, pevs, 0);
											}

											tm = tt;

											if (peff->processReplacing)
											{
												peff->processReplacing(peff, pfloatlist, pfloatlist + peff->numInputs, 4096);
											}
											else
											{
												memset(pfloat_out, 0, sizeof(float) * 4096 * peff->numOutputs);
												peff->process(peff, pfloatlist, pfloatlist + peff->numInputs, 4096);
											}

											if (pevs)
											{
												for (long i = 0; i < pevs->numEvents; i++) free(pevs->events[i]);
												free(pevs);
											}

											for (unsigned i = 0; i < 4096; i++)
											{
												for (unsigned j = 0; j < peff->numOutputs; j++)
												{
													pfloat_i[i * peff->numOutputs + j] = pfloat_out[i + j * 4096];
												}
											}

											fwrite(pfloat_i, sizeof(float), 4096 * peff->numOutputs, f);
										}

										peff->dispatcher(peff, effStopProcess, 0, 0, 0, 0);

										fclose(f);
									}
								}
								if (pstream) free(pstream);

								free(pfloat_i);
								free(pfloat_out);
								free(pfloat);
								free(pfloatlist);

								mf->Free();
							}

							free(pdata);
						}
					}
				}
			}

			peff->dispatcher(peff, effClose, 0, 0, 0, 0);
		}

		FreeLibrary(hdll);
	}
	return 0;
}

