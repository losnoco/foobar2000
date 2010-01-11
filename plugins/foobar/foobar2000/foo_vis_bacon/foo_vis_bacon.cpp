// foo_vis_bacon - A poor attempt at getting Winamp visualizations to work
//					with foobar2000 kinda sorta probably not.
//				   You'll need to set up the include directories for
//					foobar2000.h in the settings for the foo_vis_bacon project
//					to get this to compile.
//				   Multi-instance is, of course, completely broken.
//
//		Please pass the milk, please.

// 2004-02-23 18:58 UTC - kode54
// - Now defaults search path to Winamp install\Plugins instead of Foobar2000\bacon

// 2004-02-23 19:28 UTC - kode54
// - BLAH! Config no longer touches module's instance/hwnd members if it's the currently active module
// - Version is now minus0.8

// 2004-02-23 21:45 UTC - kode54
// - Added foo_osd auto-popup disabler

// 2004-02-23 22:35 UTC - kode54
// - Uggly hacks, now the vis module is unloaded when disabled
// - Version is now minus0.7

// 2004-02-23 23:26 UTC - kode54
// - Added scale/clip function for spectrum, based on foo_vis_test
// - Version is now minus0.6

// 2004-02-25 04:55 UTC -bleh
// - Some ugly hackage to allow AVS to work.
// - Added a 1-second timer for shutting everything down on flush so seeking and such won't suck.
// - Config button will bring up an error now if you try to configure any plugin
//     while any plugin is running.
// - Allowed Winamp plugins with a header version of 0x030 to work (Startle compatibility).
// - Made sure thread will actually exit in the destructor function (only added one line :| ).
// - Version is now minus0.5 alpha

// 2004-02-25 06:44 UTC - kode54
// - Corrected embed hackage to resize child window to fit client area of embed window
// - Embed window class will only be registered once per instance, to allow for Milkdrop
//     full screen / window switch ... According to handle count in Process Explorer, it
//     would appear that it is closing the embed window before the switch...
// - Embed window handles WM_SETFOCUS by focusing the visualization window
// - Fixed a simple warning by adding double-not to IPC_SET_VIS_FS_FLAG
// - Version is now minus0.4 alpha

// 2004-02-25 16:27 GMT+1 - hartwork
// - timer in threadproc instead of sleep: faster message processing, more timing accuracy
// - version minus0.399 alpha
// files changed: foo_vis_bacon.cpp, foo_vis_bacon.h
// files added:   hartwork_timer.cpp, hartwork_timer.h
//
// i'll try to optimze dataprocessing/interpolation in on_data, okay?

// 2004-02-25 16:41 UTC - kode54
// - WAHOO! :ph34r: teh DLL MANAGAR!!!one  ... This should fix all vis/config issues that didn't already
//     exist with Winamp itself. I suppose it could use PathCanonicalize to resolve garbage in the path,
//     but that shouldn't be a problem, considering the code which is using it.
// - Fixed configuration WM_DESTROY to clean up properly by calling PickUpCleanUp() ... Before, it was
//     not releasing the current library first...
// - Should be safe to enable configuration now, and not touch the parent window handle slot, in case it's
//     running. It seems one vis I tested complains at this, but most likely blows up in Winamp too.
// - Added crazy linear interpolation to spectrum copy... Hmm, maybe hartwork can do it better ^_^
// - Version is now minus0.395 alpha
// files changed: foo_vis_bacon.cpp, foo_vis_bacon.h, config.cpp
// files added:   dll_manager.cpp, dll_manager.h

#include "foo_vis_bacon.h"

// 2004-02-25 18:13 UTC - bleh
// - Added a hack implementation of WM_PARENTNOTIFY in the embbed window procdure
//     so Winamp 2.9 plugins that embed themselves will work.
// - Added a hack WM_SHOWWINDOW as above so that when AVS returns from fullscreen
//     mode, it'll be resized to the proper window dimensions.
// - Still haven't figured out what Milkdrop's problem is in fullscreen :/.
// - Version is now minus0.39 alpha because I want the above fixed by minus0.3 final.
// - Yes, I realize that making alpha builds when I haven't even gotten up to 0.0 is weird.

// 2004-02-25 18:36 UTC - kode54
// - CRAP CRAP CRAP
// - I added that DLL manager before I noticed hartwork's update... then, after I integrated his
//     slightly overkill timer code, I noticed your update! We REALLY need to coordinate or
//     something... :B
// - Now configuration only loads dll when necessary, to avoid configuration issue, example:
//       1. Configure to use Milkdrop
//       2. Close preferences
//       3. Enable BACON vis while playing
//       4. Open preferences
//       5. Click configuration button. (Not issue; toggles Milkdrop help display overlay.)
//       6. Close Milkdrop
//       7. Configuration button stops working until Milkdrop is unloaded completely
// - Also fixes the issue with the configuration DLL selector not releasing the current DLL first ^_^
// - Version is now minus0.389 alpha
// files changed: config.cpp


//Whether the plugin is enabled or not.
cfg_int cfg_enabled("Enabled", 0);

//Search path for vis modules
cfg_string cfg_basepath("DLL Base", get_default_path());

//The DLL to load and the module to pull out of it.
cfg_string cfg_dllname("DLL Path", "bacon\\vis_milk");	//Default to Milkdrop.
cfg_int cfg_vismod("Vis Module", 0);

//The title formatting for the fake Winamp window
cfg_string cfg_titleformat("Title format", "[%artist% - ]$if2(%title%,%_filename%)");

//Whether we should print "unsupported API" messages to the console
//or just eat them.
cfg_int cfg_condebug("Barf to console", 0);

// let's just be safe...
critical_section titleformat_cs;
critical_section basepath_cs;

//Declarations
DWORD WINAPI PluginThreadProc(LPVOID lpParam);
void CALLBACK FlushTimerProc(HWND hWnd, UINT msg, UINT idTimer, DWORD dwTime);
LRESULT WINAPI FakeWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI EmbedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//Constants
static const char fake_wndclass_name[] = "foo_vis_bacon_demiwndclass";
static const char embed_wndclass_name[] = "foo_vis_bacon_embedomatic";


//Buffers needed to pass data between threads.
static deque<unsigned char> waveform_buffer_left(576);
static deque<unsigned char> waveform_buffer_right(576);
static unsigned char spectrum_buffer[2][576];	//This is not time-dependent.


//Thread synchronization is evil.
critical_section crossthread_buffers_cs;

//Wahoo!
dll_manager dll;

//Necessary stuff for loading and manipulating a winamp vis plugin.
HMODULE wamp_vis_dll = NULL;
winampVisModule *wamp_vis_module;
winampVisHeader *wamp_vis_header;

//This is necessary to keep track of whether each on_data is the
//first one upon playback start.
bool loadup_stuff;
//This is necessary in case some essential initialization process blows up.
bool refuse_to_work;
//Experimental variable here.  Here's the basic idea:
// 1.  Set this before starting the visualization.
// 2.  If the thread exits and it's still set, turn off cfg_enabled.
// 3.  If g_flush gets called, unset this.
//What should happen as a result of all this is that cfg_enabled
//will be turned off when the Winamp vis window is closed, but 
//not when playback stops.
bool disable_sich;
//Here's the deal with this one:  I need to call CloseHandle() on my
//thread handle.  g_flush() will do this; however, if the thread
//exits because the Winamp vis window got closed, g_flush() never
//gets called... and I can't really call CloseHandle() on the handle
//for the calling thread.  So I have this flag here to see if we
//have a dead handle.
bool close_thread_handle_on_data;
//Finally, this one was created because if g_flush gets called a lot,
//it seems to cause problems.
bool force_thread_exit;

//Second thread for running the Winamp visualization.
HANDLE hWamp_vis_thread;

//Identifier for the timer used to avoid having to shut down
//the visualization on seeks and song switches and such.
UINT_PTR flush_timer;

//More crap.  This serves as a parent window for visualizations.
HWND hWnd_fake_winamp;
//With Winamp 5, plugins now can request a handle for a window embedded
//in Winamp.  We can't give them an embedded window, but we can give
//them a window and pretend it's embedded.
HWND hWnd_embedded_parent;
HWND hWnd_winamp_plugin;	//Window for actual plugin; acquired through IPC_SETVISWND.
HWND embedWindow(embedWindowState * embed_o_matic);
bool eat_embed_wndclass;
//Also, we have to keep track of whether the window is supposed to be
//fullscreen or not.
bool visualization_is_fullscreen;

//jarsonic'd!
foo_osd::control osd;


void g_close()
{
	//Turn this flag off now so the thread won't think it exited
	//because the Winamp plugin's window got closed.
	disable_sich = false;
	
	//Try to find the other window and kill it.
	//If we can't find it, then don't bother doing anything.
	if (IsWindow(hWnd_fake_winamp))
	{
		uSendMessage(hWnd_fake_winamp, WM_CLOSE, 0, 0);
	}
	if (IsWindow(hWnd_embedded_parent))
	{
		uSendMessage(hWnd_embedded_parent, WM_CLOSE, 0, 0);
	}


	//Wait for the thread to exit.
	if (hWamp_vis_thread)
	{
		//Try to make it easier.
		force_thread_exit = true;

		//Blah, crap will happen if we terminate anyway...
		WaitForSingleObject(hWamp_vis_thread, INFINITE);

		/*
		if (WaitForSingleObject(hWamp_vis_thread, 2500) == WAIT_TIMEOUT)
		{
			TerminateThread(hWamp_vis_thread, 0);
			console::warning("WaitForSingleObject timed out; something may be fried.");
		}
		*/

		//Don't leave the handle hanging if the thread's been closed.
		CloseHandle(hWamp_vis_thread);
		hWamp_vis_thread = NULL;
		//Nullify this window, too, since it shouldn't be around anymore.
		hWnd_winamp_plugin = NULL;

		//k done.
		force_thread_exit = false;
	}

	//Kill this flag.  The user might fix whatever went wrong
	//after stopping playback or disabling BACON or whatever.
	refuse_to_work = false;
	//Don't try to bungle the thread.
	close_thread_handle_on_data = false;
}


void g_flush()
{
	//Don't attempt to start a new timer if we've already got one... it's BAD...
	if (flush_timer)
	{
		KillTimer(NULL, flush_timer);
		flush_timer = NULL;
	}

	//See if we've got a window up and running.
	if (IsWindow(hWnd_fake_winamp))
	{
		//If so, start this timer out.
		//Set it to detonate after a second.
		flush_timer = SetTimer(NULL, 0xf00b, 1000, FlushTimerProc);
		if (!flush_timer)
		{
			//Uh-oh.  Better kill everything anyway.
			console::warning(string_printf("Failed to create timer (error %d); shutting stuff down.", GetLastError()));
			g_close();
		}
	}
}

void CALLBACK FlushTimerProc(HWND hWnd, UINT msg, UINT idTimer, DWORD dwTime)
{
	//Kill the rendering thread on timeout.
	g_close();
}


class baconwrap_vis : public visualization
{
public:
	//Usually called on foobar init.
	//Note that it doesn't really matter since this is going to get called
	//before anything else in this class.  And it only sets one variable
	//right now anyway.
	baconwrap_vis()
	{
		//I'd like to work if I can.
		refuse_to_work = false;
		//Loading stuff would be nice.
		loadup_stuff = true;
		//Set this to false here because I'd rather not try to close a handle twice.
		close_thread_handle_on_data = false;
		//...and zero this.
		hWamp_vis_thread = NULL;
		//Turn this off.
		force_thread_exit = false;

		hWnd_embedded_parent = hWnd_winamp_plugin = NULL;
		eat_embed_wndclass = false;
		flush_timer = NULL;

		//Nope.
		visualization_is_fullscreen = false;
	}

	
	~baconwrap_vis()
	{
		//Misc. stuffs.
		if (flush_timer)
		{
			KillTimer(NULL, flush_timer);
		}
		if (IsWindow(hWnd_fake_winamp))
		{
			uSendMessage(hWnd_fake_winamp, WM_CLOSE, 0, 0);
			hWnd_fake_winamp = NULL;
		}
		if (IsWindow(hWnd_embedded_parent))
		{
			uSendMessage(hWnd_embedded_parent, WM_CLOSE, 0, 0);
			//A similar call to the child window should be redundant.
		}
		if (wamp_vis_dll)
		{
			dll.free(wamp_vis_dll);
			wamp_vis_dll = NULL;
		}
		if (hWamp_vis_thread)
		{
			force_thread_exit = true;
			WaitForSingleObject(hWamp_vis_thread, INFINITE);
			CloseHandle(hWamp_vis_thread);
			hWamp_vis_thread = NULL;
		}

		//Poof!
		osd.enable();
	}



	virtual bool is_active()
	{
		//Well, are we?
		if (!cfg_enabled)
		{
			// kill the thread and release the module... after all, if this returns false, on_data won't be called
			if (wamp_vis_module)
			{
				g_close();
				quitmonger();
			}
			return false;
		}
		else
			return true;
	}


	virtual bool need_spectrum()
	{
		//Winamp plugins expect a spectrum, so we do, too.
		return true;
	}


	virtual double get_expected_latency()
	{
		//See if we've got a plugin loaded to tell us the latency.
		if (wamp_vis_module)
		{
			//Use it if we do.
			return -(wamp_vis_module->latencyMs / 1000.0);
		}
		else
		{
			//Pretend we're magic otherwise.
			return 0;
		}
	}


	virtual void on_data(const vis_chunk *data)
	{

		//See if the thread ran and exited, but didn't get its handle closed yet.
		if (close_thread_handle_on_data)
		{
			WaitForSingleObject(hWamp_vis_thread, INFINITE);
			//Kill the handle.
			CloseHandle(hWamp_vis_thread);
			hWamp_vis_thread = NULL;

			quitmonger();

			//Kill the flag.
			close_thread_handle_on_data = false;
			if (disable_sich) cfg_enabled = false;

			return;
		}

		//Make sure that we're enabled and the vis data isn't lagged.
		//We also need a visualization module...
		if ((!cfg_enabled) || (data->flags & vis_chunk::FLAG_LAGGED) || (refuse_to_work))
		{
			return;
		}

		//If the timer's going, stop it.
		if (flush_timer)
		{
			KillTimer(NULL, flush_timer);
			flush_timer = NULL;
		}

		//Now see if we need to start the thread or not.
		if (!hWamp_vis_thread)
		{
			if (!initmonger())
				return;
		}
		
		if (!wamp_vis_module)
		{
			console::warning("something's borked.");
			return;
		}

		//Now it's time to set some stuff up.
		wamp_vis_module->sRate = data->srate;
		wamp_vis_module->waveformNch =
		wamp_vis_module->spectrumNch =
		wamp_vis_module->nCh = (data->nch > 1) ? 2 : 1;


		//Eat some serious CPU.
		UINT realnch = data->nch;
		//Take the spectrum data from foobar and fill the
		//spectrum buffers we're using.
		//l33t linear interpolation uhuhuhu
		vis_sample *dataptr = data->spectrum;
		double leftdata, rightdata;
		double step = (double)data->spectrum_size / 576;
		double pos = 0., frac;
		unsigned ipos, endpos = (data->spectrum_size - 1) * realnch;
		crossthread_buffers_cs.enter();
			for (UINT i = 0; i < 576; i++, pos += step)
			{
				ipos = ((unsigned)pos) * realnch;
				leftdata = dataptr[ipos];
				if (ipos < endpos)
				{
					frac = pos - ((double)((int)pos));
					leftdata *= 1. - frac;
					leftdata += dataptr[ipos + realnch] * frac;
				}
				
				if (realnch > 1)
				{
					ipos++;
					rightdata = dataptr[ipos];
					if (ipos < endpos)
					{
						rightdata *= 1. - frac;
						rightdata += dataptr[ipos + realnch] * frac;
					}
				}

				spectrum_buffer[0][i] = thingerize2(leftdata);
				if (realnch > 1)
					spectrum_buffer[1][i] = thingerize2(rightdata);
				/*
				//The idea here is to spread out however ever many data points
				//we have over the 576-sample spectrum Winamp plugins expect.
				if ((static_cast<int>(i * data->spectrum_size) / 576) > lastpoint)
				{
					leftdata = thingerize2(dataptr++);
					if (realnch > 1)
					{
						rightdata = thingerize2(dataptr);
						dataptr += realnch - 1;
					}
					lastpoint = (i * data->spectrum_size / 576);
				}
				spectrum_buffer[0][i] = leftdata;
				spectrum_buffer[1][i] = rightdata;*/
			}
		crossthread_buffers_cs.leave();
		//Stream the samples from foobar through the sample deques.
		dataptr = data->data;
		for (UINT ii = 0; ii < data->samples; ii++)
		{
			crossthread_buffers_cs.enter();
				waveform_buffer_left.push(thingerize(dataptr++));
				if (realnch > 1)
				{
					waveform_buffer_right.push(thingerize(dataptr));
					dataptr += realnch - 1;
				}
			crossthread_buffers_cs.leave();
		}

	}

		
	virtual void on_flush()
	{
		g_flush();
	}


private:
	//Call this to load up cfg_dllname and cfg_vismod.
	//It's useful.
	bool reload_dll()
	{
		//Set up the critical section object of death and pain.
		//MULTI-INSTANCE WILL DIE HERE... AND SEVERAL OTHER PLACES.
		/*InitializeCriticalSection(&crossthread_buffers_cs);*/

		//Quick check to make sure the libary isn't already in use.
		if (wamp_vis_dll)
		{
			console::error("WTF DLL handle shouldn't still be valid.");
			cfg_enabled = false;
			refuse_to_work = true;
			return false;
		}
		//Attempt to load the configured DLL.
		wamp_vis_dll = dll.load(cfg_dllname);
		if (!wamp_vis_dll)
		{
			//TODO:  Remove this when I'm done testing.
			//It'll create an unnecessary error message the first time
			//the user runs this plugin.
			console::error("Couldn't load DLL");
			cfg_enabled = false;
			refuse_to_work = true;
			return false;
		}

		//If we got the DLL loaded, then we need to find the function
		//for getting the header.
		winampVisGetHeaderType header_getter =
			reinterpret_cast<winampVisGetHeaderType>(GetProcAddress(wamp_vis_dll, "winampVisGetHeader"));
		if (!header_getter)
		{
			console::error("Couldn't get header");
			cfg_enabled = false;
			refuse_to_work = true;
			return false;
		}
			
		//Now try it out.
		wamp_vis_header = header_getter();
		if (!header_getter)
		{
			console::error("Unable to find header for plugin.");
			cfg_enabled = false;
			refuse_to_work = true;
			return false;
		}
		//Make sure we've got the right plugin version.
		//HACK:  Also check against 0x030, because it doesn't seem too different.
		if ((wamp_vis_header->version != VIS_HDRVER) && (wamp_vis_header->version != 0x030))
		{
			console::error("Wrong visualization plugin version.");
			cfg_enabled = false;
			refuse_to_work = true;
			return false;
		}

		//Having gotten the header, we'll naturally want to get the module, too.
		wamp_vis_module = wamp_vis_header->getModule(cfg_vismod);
		if (!wamp_vis_module)
		{
			//Try to fall back to module 0 if that fails.
			wamp_vis_module = wamp_vis_header->getModule(cfg_vismod);
			if (!wamp_vis_module)
			{
				//Hmm, guess something's really broken.
				console::error("Unable to find a visualization module in current plugin.");
				cfg_enabled = false;
				refuse_to_work = true;
				return false;
			}
			//Print a warning message about what we ended up doing.
			console::warning("Selected module not found; falling back to module 0.");
		}
		

		//Now that we have stuff loaded, set this up.
		wamp_vis_module->hDllInstance = wamp_vis_dll;
		//This should also be zeroed.
		hWamp_vis_thread = NULL;

		//Clear buffers and such.
		memset(spectrum_buffer, 0, sizeof(spectrum_buffer));
		waveform_buffer_left.clear();
		waveform_buffer_right.clear();


		return true;
	}



	//Function for setting up the stuff needed for rendering to happen.
	//This includes loading the DLL (if necessary), starting the
	//render thread, etc.
	bool initmonger()
	{
		//Look into whether we should reload the plugin DLL.
		if (loadup_stuff)
		{
			//See if we already have stuff loaded and, if we do, unload it.
			if (wamp_vis_module)
			{
				quitmonger();
			}

			//Now try to init stuff.  If we fail, refuse to work.
			if (!reload_dll())
			{
				refuse_to_work = true;
				return false;
			}

			//If we got everything loaded properly, we should leave ourselves
			//a reminder so we don't try to load it again.
			loadup_stuff = false;
		}

		//Default to visualization not being fullscreen.
		visualization_is_fullscreen = false;


		//Set this flag now so the flag will be able to tell g_flush()
		//from Winamp plugin exit (let's hear a round of applause for modern ethics...).
		disable_sich = true;

		//Foom.  Pass in the address of the visualization module
		//so the other thread can manipulate it.
		DWORD id;
		hWamp_vis_thread = CreateThread(NULL, 0, PluginThreadProc, NULL, 0, &id);

		if (hWamp_vis_thread == NULL)
		{
			console::error(string_printf("Unable to start thread, error %d", errno));
			cfg_enabled = false;	//Die.
			return false;
		}

		//Poof!
		osd.disable();

		return true;
	}


	//Call this to unload the DLL and such.
	void quitmonger()
	{
		//Free the library.
		dll.free(wamp_vis_dll);
		wamp_vis_dll = NULL;

		//DESTROY
		/*DeleteCriticalSection(&crossthread_buffers_cs);*/

		//Nullify stuff.
		wamp_vis_module = NULL;
		wamp_vis_header = NULL;

		loadup_stuff = true;

		//Poof!
		osd.enable();
	}


	//Descriptive names are a waste of everyone's time.
	// And so is clipping, but let's do that anyway ^_^
	unsigned char thingerize(vis_sample *goop)
	{
		if (goop)
		{
			int blah = (int)(*goop * 128.0);
			if (blah > 127) return 127;
			else if (blah < -128) return -128;
			else return (unsigned char) blah;
		}
		else
			return 0;
	}

	unsigned char thingerize2(double val)
	{
		val = log((val / 2) + 1) / log(2.0);
		val *= 0.25;
		int blah = (int)(val * 255.0);
		if (blah > 255) return 255;
		else return (unsigned char) blah;
	}
};



//Second thread for dealing with Winamp plugin.
//The expected parameter is a pointer to the Winamp visualization module,
//so you MUST have that set up before starting this.
DWORD WINAPI PluginThreadProc(LPVOID lpParam)
{
	//Set up a simple window class.
	uWNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.hInstance = core_api::get_my_instance();
	wc.lpfnWndProc = FakeWndProc;
	wc.lpszClassName = fake_wndclass_name;
	if (!uRegisterClass(&wc))
	{
		console::error("Unable to register window class.");

		//Note that because the thread is exiting early, the handle
		//still needs to be closed.
		disable_sich = false;
		close_thread_handle_on_data = true;
		return 1;
	}


	{
		//Grab current song info, using title formatting string
		string8 title;
		titleformat_cs.enter();
		play_control::get()->playback_format_title(title, cfg_titleformat, 0);
		titleformat_cs.leave();
		
		//Now set up a simple window.
		hWnd_fake_winamp = uCreateWindowEx(0, fake_wndclass_name, title, 0,
			0, 0, 0, 0, NULL, NULL, core_api::get_my_instance(), 0);
	}
	if (!hWnd_fake_winamp)
	{
		console::error("Unable to set up demiwindow.");
		uUnregisterClass(fake_wndclass_name, core_api::get_my_instance());

		disable_sich = false;
		close_thread_handle_on_data = true;
		return 1;
	}


	//Start up the Winamp plugin now.
	wamp_vis_module->hwndParent = hWnd_fake_winamp;
	if (wamp_vis_module->Init(wamp_vis_module))
	{
		console::error("Winamp plugin died during its init.");
		DestroyWindow(hWnd_fake_winamp);
		hWnd_fake_winamp = NULL;
		uUnregisterClass(fake_wndclass_name, core_api::get_my_instance());

		close_thread_handle_on_data = true;
		return 1;
	}
	if (!wamp_vis_module)
	{
		console::error("Winamp plugin smells.");
		DestroyWindow(hWnd_fake_winamp);
		hWnd_fake_winamp = NULL;
		uUnregisterClass(fake_wndclass_name, core_api::get_my_instance());

		close_thread_handle_on_data = true;
		return 1;
	}


	//It's message loop time!
	HARTWORK_TIMER ht;
	MSG msg;
	UINT peekreturn;
	bool kearb = false;
	while (!force_thread_exit)	//I finally broke down and added a variable here.
	{
		//Handle all available messages FOR THE WHOLE THREAD
		//(caps added because it got seriously b0rked when I didn't use NULL ^^; ).
		while ((peekreturn = uPeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) > 0)
		{
			//Jump out on quit.
			if ((msg.message == WM_QUIT) || (msg.message == WM_DESTROY))
			{
				kearb = true;
			}

			TranslateMessage(&msg);
			uDispatchMessage(&msg);
		}
		if (kearb) break;

        //If something got borked, just die.
		if (peekreturn < 0)
		{
			console::error("wtf");
			DestroyWindow(hWnd_fake_winamp);
			hWnd_fake_winamp = NULL;
			uUnregisterClass(fake_wndclass_name, core_api::get_my_instance());

			close_thread_handle_on_data = true;
			return 1;
		}

		if ( ht.has_passed(wamp_vis_module->delayMs, true ) )
		{
			//Wheee
			//Sleep(wamp_vis_module->delayMs);

			//Grab a snapshot of the current data coming from foobar.
			crossthread_buffers_cs.enter();
				waveform_buffer_left.read(wamp_vis_module->waveformData[0], 576);
				waveform_buffer_right.read(wamp_vis_module->waveformData[1], 576);
				memcpy(wamp_vis_module->spectrumData, spectrum_buffer, 2*576);
			crossthread_buffers_cs.leave();

			//...and then do this.
			wamp_vis_module->Render(wamp_vis_module);
		}
	}


	//Shut down the vis plugin, our little window, and the window class.
	wamp_vis_module->Quit(wamp_vis_module);
	//Kill the window.
	DestroyWindow(hWnd_fake_winamp);
	hWnd_fake_winamp = NULL;

	uUnregisterClass(fake_wndclass_name, core_api::get_my_instance());

	//See if this window class needs to go away.
	if (eat_embed_wndclass)
	{
		uUnregisterClass(embed_wndclass_name, core_api::get_my_instance());
		eat_embed_wndclass = false;
	}

	//Finally, set this flag so I don't keep eating handles.
	close_thread_handle_on_data = true;


	//Um, yay, we're done.
	return 0;
}


//Window for handling messages expected to go to Winamp.
LRESULT WINAPI FakeWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//Check stuff.
	switch (msg)
	{
	case WM_WA_IPC:
		//TODO:  Have window return correct values for Winamp API calls.
		switch (lParam)
		{
		case IPC_GETVERSION:
			return 0x2950;	//Claim to be Winamp version 2.95.
			break;

		case IPC_ISPLAYING:
			return cfg_enabled;

			break;
		case IPC_GETOUTPUTTIME:
			{
				play_control * pc = play_control::get();
				if (wParam)
					return (LRESULT) pc->playback_get_length();
				else
					return (LRESULT) (pc->playback_get_position() * 1000.);
			}
			break;
		case IPC_GETLISTPOS:
			return 0;

			break;
		case IPC_GETPLAYLISTFILE:
			return (LRESULT)"C:\\b20-ghd.dmf";

			break;
		case IPC_GETPLAYLISTTITLE:
			return (LRESULT)"Hamster in a Can";

			break;
		case IPC_IS_FULLSCREEN:
			//Easy.
			return visualization_is_fullscreen;

			break;
		case IPC_SET_VIS_FS_FLAG:
			if (cfg_condebug)
				console::info("Received IPC_SET_VIS_FS_FLAG");

			//Also pretty easy.
			visualization_is_fullscreen = !!wParam;
			return 0;

			break;
		case IPC_GETINIFILE:
			{
				static char foo[MAX_PATH];
				string8 path;
				if (get_winamp_path(path))
				{
					path += "\\winamp.ini";
				}
				else
				{
					uGetModuleFileName(NULL, path);
					path.truncate(path.find_last('\\'));
					path += "\\bacon\\winamp.ini";
				}
				string_os_from_utf8 bleh(path);
				strncpy(foo, bleh, MAX_PATH);
				return (LRESULT) foo;
			}
			break;

		case IPC_GETINIDIRECTORY:
			{
				static char foo[MAX_PATH];
				string8 path;
				if (get_winamp_path(path))
				{
					path.add_byte('\\');
				}
				else
				{
					uGetModuleFileName(NULL, path);
					path.truncate(path.find_last('\\'));
					path += "\\bacon\\";
				}
				string_os_from_utf8 bleh(path);
				strncpy(foo, bleh, MAX_PATH);
				return (LRESULT) foo;
			}
			break;

		case IPC_GET_EMBEDIF:
			if (cfg_condebug)
				console::info("Receieved IPC_GET_EMBEDIF (gets embed info).");

			//If the parameter is NULL, return a pointer to the
			//embedWindow function thingy.
			//Otherwise, call the function with their parameter
			//and return the result.
			if (wParam)
			{
				return reinterpret_cast<LRESULT>(embedWindow(reinterpret_cast<embedWindowState *>(wParam)));
			}
			else
			{
				return reinterpret_cast<LRESULT>(embedWindow);
			}
			break;

		case IPC_SETVISWND:
			if (cfg_condebug)
				console::info("Received IPC_SETVISWND (sets handle of visualization window).");

			//Store the handle to the winamp visualization window.
			hWnd_winamp_plugin = reinterpret_cast<HWND>(wParam);

			//See if this is a child window of the embed window.
			//Short-circuit logic takes care of making sure this is a valid test.
			if (IsWindow(hWnd_winamp_plugin) && IsWindow(hWnd_embedded_parent)
					&& IsChild(hWnd_embedded_parent, hWnd_winamp_plugin))
			{
				//Don't let the plugin window be retarded and try to move
				//itself to some idiotic place.  THIS IS A HACK.  Why use it?
				//SVIS.c : CreateWindow in a position other than (0, 0).
				//Spy++ : Window has relative position (0, 0).
				//Conclusion:  Winamp does stuff.

				//Get the client rect for the parent window.
				RECT yayhacks;
				GetClientRect(hWnd_embedded_parent, &yayhacks);
				//Now set the position to 0, 0 and dimensions to those
				//of the parent window.
				MoveWindow(hWnd_winamp_plugin, 0, 0, yayhacks.right, yayhacks.bottom, true);
			}

			break;

		case IPC_IS_PLAYING_VIDEO:
			//We don't play video.  See the foobar2000 forums 
			//for a more in-depth discussion.

			return 0;
			break;
		default:
			//Only print this message if we're configured to.
			if (cfg_condebug)
				console::info(string_printf("unsupported winamp api: %d", lParam));
			return 0;
			break;
		}
		break;

	//Handle this.
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_WINDOWPOSCHANGING:
		{
			// Maybe all of these added flags are ludicrous and all it needs is SWP_HIDEWINDOW...
			PWINDOWPOS pwp = (PWINDOWPOS) lParam;
			pwp->flags = (pwp->flags & ~(SWP_SHOWWINDOW)) | (SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_HIDEWINDOW);
			return 0;
		}
		break;
	}

	//Be lazy.
	return uDefWindowProc(hWnd, msg, wParam, lParam);
}


//Function for handling the IPC_GET_EMBEDIF thing.
HWND embedWindow(embedWindowState * embed_o_matic)
{
	//Okay, here'w what's supposed to happen:
	//embed_o_matic->me gets set to the window for the embedded visualization.
	//embed_o_matic->flags is used to determine some aspects of
	// this window.  I don't care about transparency, but I
	// do care about EMBED_FLAGS_NORESIZE.
	//embed_o_matic->r is used to set the window rect of the embedded window.
	//embed_o_matic->user_ptr is undocumented :/.
	//I return a handle to the parent window of this
	//visualization, so I make a window to be the parent,
	//then make a child window to make the plugin window.
	//yay.

	//Make sure the pointer is valid.
	if (!embed_o_matic)
		return NULL;

	//Create a window class for this mess.
	uWNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.hInstance = core_api::get_my_instance();
	wc.lpfnWndProc = EmbedWndProc;
	wc.lpszClassName = embed_wndclass_name;
	if (!eat_embed_wndclass)
	{
		if (!uRegisterClass(&wc))
		{
			console::error("Unable to register window class for embed thingy.");
			return NULL;
		}
		//Let the thread exist stuff know it should kill this window class
		//if it gets the chance.
		eat_embed_wndclass = true;
	}

	//Create the parent
	hWnd_embedded_parent = uCreateWindowEx(WS_EX_APPWINDOW, embed_wndclass_name, "Winamp plugin",
		//Determine the window style based on whether the window can be resized or not.
		//If it can be resized, this is equivalent to WS_TILEDWINDOW.
		WS_TILED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | 
			((embed_o_matic->flags & EMBED_FLAGS_NORESIZE) ? WS_BORDER : WS_THICKFRAME),
		//Get the position and size from r
		embed_o_matic->r.left, embed_o_matic->r.top, (embed_o_matic->r.right - embed_o_matic->r.left),
		(embed_o_matic->r.bottom - embed_o_matic->r.top), hWnd_fake_winamp, NULL, core_api::get_my_instance(), NULL);
	//Test.
	if (!hWnd_embedded_parent)
	{
		console::error("Plugin requested an embed window; failed to create it.");
		return NULL;
	}

	//Set this to this.  I don't know why they're the same, but
	//I've compared the handle values coming from Winamp and they're identical :/.
	embed_o_matic->me = hWnd_embedded_parent;


	//Return the parent window for the embedded plugin, but only if it exists.
	if (IsWindow(hWnd_embedded_parent))
		return hWnd_embedded_parent;
	else
	{
		return NULL;
	}
}


//Window procedure for the embed plugin schtuff.
LRESULT WINAPI EmbedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//Process stuff.
	switch (msg)
	{
	case WM_SIZE:
		//Make sure the window that got resized was the embed parent,
		//that the given window handle was actually the one that got
		//resized, and that the resizing operation wasn't minimization,
		//which we frankly don't care about.
		//SIZE_RESTORED is #defined to 0, so & didn't work :|.
		if ((hWnd == hWnd_embedded_parent) && ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
		{
			//Now verify that we have a child window set up as we want
			//it and resize it to fill the parent window.
			if (IsWindow(hWnd_embedded_parent) && IsWindow(hWnd_winamp_plugin)
					&& IsChild(hWnd_embedded_parent, hWnd_winamp_plugin))
			{
				//Find the dimensions of the client area.
				//I'm re-using code here... maybe I should make a function :P.
				RECT yayhacks;
				GetClientRect(hWnd_embedded_parent, &yayhacks);
				//Resize the child window.  I'm also moving the child window
				//to position (0, 0), but that doesn't matter because I
				//want it there anyway.
				MoveWindow(hWnd_winamp_plugin, 0, 0, yayhacks.right, yayhacks.bottom, true);
			}
		}
		break;
	case WM_SETFOCUS:
		if ((hWnd == hWnd_embedded_parent) && IsWindow(hWnd_winamp_plugin))
		{
			SetFocus(hWnd_winamp_plugin);
			return 0;
		}
		break;
	case WM_PARENTNOTIFY:
		//HACK:  The Winamp 2.9 SDK didn't have an IPC_SET_VIS_WND
		//message, so for Milkdrop 1.04 and AVS 2.6.0, I have to
		//assume that any child of the embedded parent window is,
		//in fact, the plugin window.  This is probably stupid.

		//So anyway, here I check that a child window is being created
		//for hWnd_embedded_parent and that we don't already have
		//a child window for it (which would be bad and is an
		//example of why this is a big hack).
		if ((hWnd == hWnd_embedded_parent) && (LOWORD(wParam) == WM_CREATE) &&
				(!IsWindow(hWnd_winamp_plugin)))
		{
			//Set this.  Woo.
			hWnd_winamp_plugin = reinterpret_cast<HWND>(lParam);
		}
		break;
	case WM_SHOWWINDOW:
		//This is a continuation of the embed hacks.  If the parent window gets
		//shown through a call to ShowWindow, we have to resize the child window
		//and stuff.  So here are the conditions:
		//1.  The window being shown is the parent.
		//2.  The window is being SHOWN.
		//3.  The reason is a call to ShowWindow.
		//This should fix returning from fullscreen mode in AVS 2.6.0.
		if ((hWnd == hWnd_embedded_parent) && (wParam) && (!lParam))
		{
			//Woo.

			//Now verify that we have a child window set up as we want
			//it and resize it to fill the parent window.
			if (IsWindow(hWnd_embedded_parent) && IsWindow(hWnd_winamp_plugin)
					&& IsChild(hWnd_embedded_parent, hWnd_winamp_plugin))
			{
				//Find the dimensions of the client area.
				//I'm re-using code here... maybe I should make a function :P.
				RECT yayhacks;
				GetClientRect(hWnd_embedded_parent, &yayhacks);
				//Resize the child window.  I'm also moving the child window
				//to position (0, 0), but that doesn't matter because I
				//want it there anyway.
				MoveWindow(hWnd_winamp_plugin, 0, 0, yayhacks.right, yayhacks.bottom, true);
			}
		}
		break;
	}

	//Be lazy.
	return uDefWindowProc(hWnd, msg, wParam, lParam);
}


//Register this class.
static visualization_factory<baconwrap_vis> bacon;	//In a factory... downtown...


//Play callback, for window title...
class baconwrap_callback : public play_callback
{
public:
	virtual void on_playback_starting() {}
	virtual void on_playback_edited(metadb_handle * track) {}
	virtual void on_playback_stop(enum play_control::stop_reason reason) {}
	virtual void on_playback_time(metadb_handle * track, double val) {}
	virtual void on_volume_change(int new_val) {}
	virtual void on_playback_seek(double time) {}
	virtual void on_playback_pause(int state) {}

	virtual unsigned get_callback_mask()
	{
		return MASK_on_playback_dynamic_info | MASK_on_playback_new_track;
	}

	virtual void on_playback_new_track(metadb_handle * track)
	{
		if (cfg_enabled)
		{
			if (IsWindow(hWnd_fake_winamp))
			{
				string8 title;
				titleformat_cs.enter();
				track->handle_format_title(title, cfg_titleformat, 0);
				titleformat_cs.leave();
				uSetWindowText(hWnd_fake_winamp, title);
			}
		}
	}

	virtual void on_playback_dynamic_info(const file_info *info, bool b_track_change)
	{
		if (b_track_change && cfg_enabled)
		{
			if (IsWindow(hWnd_fake_winamp))
			{
				string8 title;
				titleformat_cs.enter();
				play_control::get()->playback_format_title(title, cfg_titleformat, 0);
				titleformat_cs.leave();
				uSetWindowText(hWnd_fake_winamp, title);
			}
		}
	}
};

//Register callback class
static play_callback_factory<baconwrap_callback> moo;


//Necessary stuff for getting this plugin shoved into the main menu.
class menu_item_bacon : public menu_item_main
{
	virtual unsigned int get_num_items() {return 1;}
	virtual void enum_item(unsigned n, string_base & out) {out = (n==0) ? "Components/Visualization/BACON" : 0;}
	virtual void perform_command(unsigned n) 
	{
		if (n==0 && core_api::assert_main_thread())
		{
			cfg_enabled = !cfg_enabled;
			if (cfg_enabled)
			{
				if (!visualization::is_vis_manager_present()) console::error("Visualization manager not present, please reinstall.");

				//TODO:  Reload shit.
			}
			else
			{
				//Stop visualization.  Now (no timer crap).
				g_close();
			}
		}
	}
	virtual bool is_checked(unsigned n) {return n==0 ? !!cfg_enabled : 0;};
};

static menu_item_factory<menu_item_bacon> shoop;


bool get_winamp_path(string8 & out)
{
	out.reset();
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Winamp",
		0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		DWORD type, size = 0;
		LONG ret = RegQueryValueEx(hKey, "UninstallString", 0, &type, NULL, &size);
		if ((ret == ERROR_SUCCESS || ret == ERROR_MORE_DATA) && type == REG_SZ)
		{
			mem_block_t<char> temp_block;
			char * temp = (size <= PFC_ALLOCA_LIMIT) ? (char*)alloca(size) : temp_block.set_size(size);
			assert(temp);
			if (RegQueryValueEx(hKey, "UninstallString", 0, &type, (LPBYTE) temp, &size) == ERROR_SUCCESS)
			{
				if (*temp == '"')
				{
					// joy, quotes!
					char * end = strchr(++temp, '"');
					if (end) *end = 0;
				}
				out.add_string_ansi(temp);
				out.truncate(out.find_last('\\'));
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
		RegCloseKey(hKey);

		return true;
	}
	else
	{
		return false;
	}
}





DECLARE_COMPONENT_VERSION("Bacon Visualization Wrapper", "minus0.389 alpha",
"It kind of lets you use Winamp visualization plugins\n\
with foobar... almost... maybe... no.\n \
Version minus0.389 alpha: \"Meep!\"\n \
\n \
singergr@msu.edu");