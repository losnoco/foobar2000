
#include "foo_vis_bacon.h"



//Whether the plugin is enabled or not.
extern cfg_int cfg_enabled;

//The search path
extern cfg_string cfg_basepath;

//The DLL to load and the module to pull out of it.
extern cfg_string cfg_dllname;
extern cfg_int cfg_vismod;

//Title formatting, and sync
extern cfg_string cfg_titleformat;
extern critical_section titleformat_cs;
extern critical_section basepath_cs;

//Whether we should dump some misc. info to the console or not.
extern cfg_int cfg_condebug;

//A flag signalling when the plugin should close its current
//DLL and load the one specified by cfg_dllname.
//extern bool loadup_stuff;

//The window handle for a Winamp-impersonanting parent window
//that (this is important) only sticks around while visualization
//is running.
//extern HWND hWnd_fake_winamp;

//Wahoo!
extern dll_manager dll;


//										-----Config stuff------
// might as well use ptr_list_t
ptr_list_t<char> found_plugins;
ptr_list_t<char> plugin_names;
ptr_list_t<char> module_names;
unsigned int curseldll = 0;	//Currently selected plugin.
unsigned int curselmod = 0;	//Currently selected module.
bool mod_has_config = false;	//Does the module have a config function?

HMODULE cfg_dll_handle = NULL;
winampVisHeader* cfg_wavh = NULL;
winampVisModule* cfg_wavm = NULL;

void ScanForDLLs(ptr_list_t<char> & output_list)
{
	//For storing data about the found file
	uFindFile * hFind = NULL;
	//Just to keep track of this.
	basepath_cs.enter();
	string8 components_dir(cfg_basepath);
	basepath_cs.leave();
	
	//Assemble a search string.
	//We're looking for (location of foo_vis_bacon.dll)\..\bacon\*.dll,
	//which will in all likelihood be
	//DRIVE:\Program Files\foobar2000\bacon\*.dll
	string8_fastalloc search_string;
	search_string = components_dir;
	search_string += "vis_*.dll";

	if (cfg_condebug)
		console::info(search_string);
	
	//Find the first file to get things started...
	hFind = uFindFirstFile(search_string);
	//See if we got anything.
	if (!hFind)
	{
		console::warning("No Winamp plugins found!");
		return;
	}
	//Put the filename into our list...
	string8_fastalloc temp;
	char * bleh;

	temp = components_dir;
	temp += hFind->GetFileName();
	bleh = new char[temp.length() + 1];
	strcpy(bleh, temp);
	output_list.add_item(bleh);
	//TODO:  See if I'm leaking memory here.
	//I shouldn't be, but something bothers me... bah

	//Now go through and see how many more DLL's we find.
	while (hFind->FindNext())
	{
		temp = components_dir;
		temp += hFind->GetFileName();
		bleh = new char[temp.length() + 1];
		strcpy(bleh, temp);
		output_list.add_item(bleh);
	}

	//Clean up.
	delete hFind;
}

winampVisHeader* LoadPluginDLL(const char* filename, HMODULE& dllHandle)
{
	winampVisHeader* wavh = NULL;

	//Try to load up some library.
	dllHandle = dll.load(filename);
	//See if it loaded or not.
	if (!dllHandle)
	{
		//ERRAR
		console::warning("Unable to load \"DLL.\"");
		return NULL;
	}
     
	//Now look for a Winamp visualization plugin header.
	//We'll need a function pointer to do this...
	winampVisGetHeaderType header_getter =
		reinterpret_cast<winampVisGetHeaderType>(
			GetProcAddress(dllHandle, "winampVisGetHeader")	);
	//...and we'll need to make sure this function exists...
	if (!header_getter)
	{
		//oops.  I guess this isn't a Winamp plugin, then.
		dll.free(dllHandle);
		dllHandle = NULL;
		return NULL;
	}
	//...and then we'll need to call the function *grumble*...
	wavh = header_getter();
	//...and then see if we got anything useful out of it...
	if (!wavh)
	{
		//ERRAR
		console::warning("Winamp DLL returned a null header :/");
		dll.free(dllHandle);
		dllHandle = NULL;
		return NULL;
	}
	//...and then check the version of it to make sure it matches
	//the Winamp header I'm compiling against...
	//HACK:  Also allow version 0x030.
	if ((wavh->version != VIS_HDRVER) && (wavh->version != 0x030))
	{
		//ERRAR
		console::warning("Winamp vis DLL is of the wrong SDK version.  Sorry :|");
		dll.free(dllHandle);
		dllHandle = NULL;
		return NULL;
	}

	return wavh;
}


int GetModuleNames(winampVisHeader* wavh, ptr_list_t<char> & name_list)
{
	UINT mord = 0;	//Hooray for meaningless variable names.
	UINT selected_mod = 0;
	winampVisModule* wavm = NULL;
	while (wavm = wavh->getModule(mord))	//assignment intended
	{
		//See if we've found our configured module.
		if (cfg_vismod == (int)mord)
		{
			//Yay
			selected_mod = mord;

			//Check for config...
			if (wavm->Config)
			{
				mod_has_config = true;
			}
			else
			{
				mod_has_config = false;
			}
		}
		//Store the description of each module we find because I'm
		//not using MFC so I don't get to use CBS_HASSTRINGS.
		char* cur_mod_name = NULL;
		if (wavm->description)
		{
			cur_mod_name = new char[strlen(wavm->description) + 1];
			strcpy(cur_mod_name, wavm->description);
		}
		else
		{
			cur_mod_name = new char[20];
			ltoa(mord, cur_mod_name, 10);
		}
		name_list.add_item(cur_mod_name);

		mord++;
	}

	return selected_mod;
}

void PickUpCleanUp()
{
	cfg_wavm = NULL;
	cfg_wavh = NULL;
	if (cfg_dll_handle)
	{
		dll.free(cfg_dll_handle);
		cfg_dll_handle = NULL;
	}
	module_names.delete_all();
	plugin_names.delete_all();
	found_plugins.delete_all();
}

//							---Callback for config dialog---
static BOOL CALLBACK ConfigProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//
	unsigned int i = 0;

	switch (msg)
	{
	case WM_INITDIALOG:
		//First, we need to find plugins.
		//Start by rounding up DLL's.
		ScanForDLLs(found_plugins);
		//Next, process each of them...
		curseldll = 0;
		while (i < found_plugins.get_count())	//can't use for because size can change.
		{
			//Try to load the DLL
			cfg_wavh = LoadPluginDLL(found_plugins[i], cfg_dll_handle);
			if (!cfg_wavh)
			{
				//Get rid of the filename entry if it's not a real plugin.
				found_plugins.delete_by_idx(i);

				//Don't increment i because we just killed an entry.
			}
			else
			{
				//We've got the DLL loaded, so we can store stuff now.
				char* bleh = new char[strlen(cfg_wavh->description) + 1];
				strcpy(bleh, cfg_wavh->description);
				plugin_names.add_item(bleh);

				//Add an entry for this plugin to the plugin-selecting combo box.
				uSendDlgItemMessageText(hWnd, IDC_DLL, CB_INSERTSTRING, i, plugin_names[i]);

				//See if we've found the DLL we're configured to use
				//and do stuff to it if it is.
				if ((strlen(cfg_dllname) == strlen(found_plugins[i])) &&
					(!strnicmp(cfg_dllname, found_plugins[i], strlen(cfg_dllname))))
				{
					//Remember this.
					curseldll = i;

					//Go module hunting.
					curselmod = GetModuleNames(cfg_wavh, module_names);
					
					//Fill in the module combo box entries.
					for (unsigned int i = 0; i < module_names.get_count(); i++)
					{
						uSendDlgItemMessageText(hWnd, IDC_MODULE, CB_INSERTSTRING, i, module_names[i]);
					}
				}

				i++;	//:o
			}

			//Unload the library we're using.
			if (cfg_dll_handle)
			{
				dll.free(cfg_dll_handle);
				cfg_dll_handle = NULL;
			}
		}

		//Blow up if we didn't find any plugins.
		if (found_plugins.get_count())
		{
			//Possibly undo crap from EVIL nested calls below
			EnableWindow(GetDlgItem(hWnd, IDC_DLL), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_MODULE), TRUE);

			//If we didn't find our configured DLL, module_names will still be
			//empty, so try to fill it in for the current DLL if it's still
			//empty.
			if (!module_names.get_count())
			{
				winampVisHeader* cfg_wavh = LoadPluginDLL(found_plugins[curseldll], cfg_dll_handle);
				if (!cfg_dll_handle)
				{
					console::error("A plugin DLL blew up :(");
					PickUpCleanUp();
					DestroyWindow(hWnd);
					return false;
				}
				if (!cfg_wavh)
				{
					console::error("A bad DLL stuck in...");
					PickUpCleanUp();
					DestroyWindow(hWnd);
					return false;
				}
				
				curselmod = GetModuleNames(cfg_wavh, module_names);
				for (unsigned int i = 0; i < module_names.get_count(); i++)
				{
					uSendDlgItemMessageText(hWnd, IDC_MODULE, CB_INSERTSTRING, i, module_names[i]);
				}
				
				if (cfg_dll_handle)
				{
					dll.free(cfg_dll_handle);
					cfg_dll_handle = NULL;
				}
			}	//BLARG

			//If we STILL don't have any modules, something's up.
			if (!module_names.get_count())
			{
				console::error("Couldn't get any modules.");
				PickUpCleanUp();
				DestroyWindow(hWnd);
				return false;
			}
		}
		else
		{
			console::error("No Winamp visualization plug-ins found in the configured folder.");
			EnableWindow(GetDlgItem(hWnd, IDC_DLL), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_MODULE), FALSE);
		}
		
		//Select our selections.
		uSendDlgItemMessage(hWnd, IDC_DLL, CB_SETCURSEL, curseldll, 0);
		uSendDlgItemMessage(hWnd, IDC_MODULE, CB_SETCURSEL, curselmod, 0);

		//Disable the config button if it doesn't have any use.
		EnableWindow(GetDlgItem(hWnd, IDC_CONFIG), mod_has_config);

		//Set the checkbox for the unsupported API console thingy.
		uSendDlgItemMessage(hWnd, IDC_CHECK_CONDEBUG, BM_SETCHECK, cfg_condebug, 0);

		//Fill in base path
		basepath_cs.enter();
		uSetDlgItemText(hWnd,IDC_PATH,cfg_basepath);
		basepath_cs.leave();

		//Fill in title formatting
		uSetDlgItemText(hWnd,IDC_TITLEFORMAT,cfg_titleformat);

		//Now load up everything we need for our magical global variables.
		/*
		if (found_plugins.get_count())
		{
			cfg_wavh = LoadPluginDLL(found_plugins[curseldll], cfg_dll_handle);
			if (!cfg_dll_handle || !cfg_wavh)
			{
				console::error("Couldn't initialize configured plugin.");
				PickUpCleanUp();
				DestroyWindow(hWnd);
				return false;
			}
			cfg_wavm = cfg_wavh->getModule(curselmod);
			if (!cfg_wavm)
			{
				console::error("Couldn't initialize configured module.");
				PickUpCleanUp();
				DestroyWindow(hWnd);
				return false;
			}
			cfg_wavm->hDllInstance = cfg_dll_handle;
			//cfg_wavm->hwndParent = hWnd;
		}*/


		return true;
		break;
	case WM_COMMAND:
		//See what's going on.
		switch (HIWORD(wParam))
		{
		case CBN_SELCHANGE:	//Combo box selection change
			//See who's bugging us.
			switch (LOWORD(wParam))
			{
			case IDC_DLL:
				//New DLL selected.  Update.
				curseldll = uSendDlgItemMessage(hWnd, IDC_DLL, CB_GETCURSEL, 0, 0);
				cfg_dllname = found_plugins[curseldll];
				//Set ourselves up to reload the plugin when playback is restarted.
				//loadup_stuff = true;
				//console::info(cfg_dllname);

				//Fix modules.
				module_names.delete_all();
				uSendDlgItemMessage(hWnd, IDC_MODULE, CB_RESETCONTENT, 0, 0);
				{
					HMODULE blarg = NULL;
					winampVisHeader* hlorp = LoadPluginDLL(found_plugins[curseldll], blarg);
					if (!blarg)
					{
						console::error("A plugin DLL blew up :(");
						PickUpCleanUp();
						DestroyWindow(hWnd);
						return false;
					}
					if (!hlorp)
					{
						console::error("A bad DLL stuck in...");
						curseldll = 0;
						PickUpCleanUp();
						DestroyWindow(hWnd);
						return false;
					}

					curselmod = GetModuleNames(hlorp, module_names);
					for (unsigned int i = 0; i < module_names.get_count(); i++)
					{
						uSendDlgItemMessageText(hWnd, IDC_MODULE, CB_INSERTSTRING, i, module_names[i]);
					}

					dll.free(blarg);
				}

				//Re-set up our globals
				/*
				cfg_wavm = cfg_wavh->getModule(curselmod);
				if (!cfg_wavm)
				{
					console::error("Couldn't initialize configured module.");
					PickUpCleanUp();
					DestroyWindow(hWnd);
					return false;
				}
				cfg_wavm->hDllInstance = cfg_dll_handle;
				//cfg_wavm->hwndParent = hWnd;*/

				uSendDlgItemMessage(hWnd, IDC_MODULE, CB_SETCURSEL, curselmod, 0);
				cfg_vismod = curselmod;
				//Set ourselves up to reload the plugin when playback is restarted.
				//loadup_stuff = true;

				break;
			case IDC_MODULE:
				//New mod.
				curselmod = uSendDlgItemMessage(hWnd, IDC_MODULE, CB_GETCURSEL, 0, 0);
				cfg_vismod = curselmod;
				//Set ourselves up to reload the plugin when playback is restarted.
				//loadup_stuff = true;

				//Set up our global module for the new thingy
				/*cfg_wavm = cfg_wavh->getModule(curselmod);
				if (!cfg_wavm)
				{
					console::error("Couldn't initialize configured module.");
					PickUpCleanUp();
					DestroyWindow(hWnd);
					return false;
				}
				cfg_wavm->hDllInstance = cfg_dll_handle;*/
				//cfg_wavm->hwndParent = hWnd;
				break;
			}
			break;
		case BN_CLICKED:	//Button click.
			//Make sure it's the button that's bugging us.
			if (LOWORD(wParam) == IDC_CONFIG)
			{
				//Launch the config dialog for our stupid thing.
				/*if (!cfg_wavm)
				{
					console::error("cfg:  No module loaded.  D'oh.");
					PickUpCleanUp();
					DestroyWindow(hWnd);
					return false;
				}*/
				cfg_wavh = LoadPluginDLL(found_plugins[curseldll], cfg_dll_handle);
				if (!cfg_dll_handle || !cfg_wavh)
				{
					console::error("Couldn't initialize configured plugin.");
					PickUpCleanUp();
					DestroyWindow(hWnd);
					return false;
				}
				cfg_wavm = cfg_wavh->getModule(curselmod);
				if (!cfg_wavm)
				{
					console::error("Couldn't initialize configured module.");
					PickUpCleanUp();
					DestroyWindow(hWnd);
					return false;
				}
				cfg_wavm->hDllInstance = cfg_dll_handle;
				//Make sure visualization isn't running right now.
				//TODO:  Figure out some way of making this plugin-specific...
				//which will probably be a real mess given how many times a DLL
				//is loaded in this file :/.
				/*if (IsWindow(hWnd_fake_winamp))
				{
					console::error("Cannot configure while visualization is running.  Close the config dialog, stop visualization, and try again.");
					return false;
				}*/
				/*if (!cfg_wavm->hwndParent)
					console::error("WTF mate");*/
				if (!cfg_wavm->hDllInstance)
					console::error("Still WTF mate");
				cfg_wavm->Config(cfg_wavm);
				cfg_wavm = NULL;
				cfg_wavh = NULL;
				dll.free(cfg_dll_handle);
				cfg_dll_handle = NULL;
			}
			else if (LOWORD(wParam) == IDC_PATHSET)
			{
				insync(basepath_cs);
				string8 path(cfg_basepath);
				if (uBrowseForFolder(hWnd, "Select a directory containing vis plug-ins", path))
				{
					if (*((const char *)path + path.length() - 1) != '\\')
						path.add_byte('\\');
					cfg_basepath = path;
					PickUpCleanUp();
					// EVIL
					uSendDlgItemMessage(hWnd, IDC_DLL, CB_RESETCONTENT, 0, 0);
					uSendDlgItemMessage(hWnd, IDC_MODULE, CB_RESETCONTENT, 0, 0);
					ConfigProc(hWnd, WM_INITDIALOG, 0, 0);
				}
			}
			else if (LOWORD(wParam) == IDC_PATHRESET)
			{
				insync(basepath_cs);
				cfg_basepath = get_default_path();
				PickUpCleanUp();
				// EVIL
				uSendDlgItemMessage(hWnd, IDC_DLL, CB_RESETCONTENT, 0, 0);
				uSendDlgItemMessage(hWnd, IDC_MODULE, CB_RESETCONTENT, 0, 0);
				ConfigProc(hWnd, WM_INITDIALOG, 0, 0);
			}
			else if (LOWORD(wParam) == IDC_CHECK_CONDEBUG)
			{
				cfg_condebug = uSendMessage((HWND)lParam,BM_GETCHECK,0,0);
			}
			break;
		case EN_CHANGE:
			//Only for the title formatting box...
			if (LOWORD(wParam) == IDC_TITLEFORMAT)
			{
				insync(titleformat_cs);
				cfg_titleformat = string_utf8_from_window((HWND)lParam);
			}
			break;
		}

		break;

	case WM_DESTROY:
		//Clean up all my shit.
		PickUpCleanUp();

		cfg_vismod = curselmod;
		//Set ourselves up to reload the plugin when playback is restarted.
		//loadup_stuff = true;

		break;
	}

	return 0;
}


//							---Config class---
class bacon_config : public config
{
public:

	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG, parent, ConfigProc, 0);
	}

	virtual const char * get_name()
	{
		return "Baconwrap";
	}

	virtual const char * get_parent_name()
	{
		return "Visualizations";
	}
};

static config_factory<bacon_config> taco;