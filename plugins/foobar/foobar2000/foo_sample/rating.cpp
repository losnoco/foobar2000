#include "stdafx.h"


/*
========================================================================
   Sample implementation of metadb_index_client and a rating database.
========================================================================

The rating data is all maintained by metadb backend, we only present and alter it when asked to.
Relevant classes:
metadb_index_client_impl - turns track info into a database key to which our data gets pinned.
init_stage_callback_impl - initializes ourselves at the proper moment of the app lifetime.
metadb_display_field_provider_impl - publishes %foo_sample_rating% via title formatting.
contextmenu_rating - context menu command to cycle rating values.
mainmenu_rating - main menu command to show a dump of all present ratings.
track_property_provider_impl - serves info for the Properties dialog
*/

namespace {
	
	// I am foo_sample and these are *my* GUIDs.
	// Replace with your own when reusing.
	// Always recreate guid_foo_sample_rating_index if your metadb_index_client_impl hashing semantics changed or else you run into inconsistent/nonsensical data.
	static const GUID guid_foo_sample_rating_index = { 0x88cf3f09, 0x26a8, 0x42ef,{ 0xb7, 0xb8, 0x42, 0x21, 0xb9, 0x62, 0x26, 0x78 } };
	static const GUID guid_foo_sample_rating_contextmenu = { 0x5d71c93, 0x5d38, 0x4e63,{ 0x97, 0x66, 0x8f, 0xb7, 0x6d, 0xc7, 0xc5, 0x9e } };
	static const GUID guid_foo_sample_contextmenu_group = { 0x572de7f4, 0xcbdf, 0x479a,{ 0x97, 0x26, 0xa, 0xb0, 0x97, 0x47, 0x69, 0xe3 } };
	static const GUID guid_foo_sample_rating_mainmenu = { 0x53327baa, 0xbaa4, 0x478e,{ 0x87, 0x24, 0xf7, 0x38, 0x4, 0x15, 0xf7, 0x27 } };
	static const GUID guid_foo_sample_mainmenu_group  = { 0x44963e7a, 0x4b2a, 0x4588,{ 0xb0, 0x17, 0xa8, 0x69, 0x18, 0xcb, 0x8a, 0xa5 } };

	// Pattern by which we pin our data to. 
	// If multiple songs in the library evaluate to the same string, they will be considered the same by our component, 
	// and data applied to one will also show up with the rest.
	// For an example, modify this to "%album artist% - %album%" and you get a whole-album rating shown for each track of the album
	// Always recreate guid_foo_sample_rating_index if you change this
	static const char strPinTo[] = "%artist% - %title%";


	// Our group in Properties dialog / Details tab, see track_property_provider_impl
	static const char strPropertiesGroup[] = "Sample Component";

	// Retain pinned data for four weeks if there are no matching items in library
	static const t_filetimestamp retentionPeriod = system_time_periods::week * 4;

	// Returns a titleformat_object used to generate key values for our database.
	static titleformat_object::ptr makeKeyObj(const char * pinTo) {
		titleformat_object::ptr obj;
		static_api_ptr_t<titleformat_compiler>()->compile_force(obj, pinTo);
		return obj;
	}

	// A class that turns metadata + location info into hashes to which our data gets pinned by the backend.
	class metadb_index_client_impl : public metadb_index_client {
	public:
		metadb_index_hash transform(const file_info & info, const playable_location & location) {
			static auto obj = makeKeyObj(strPinTo); // initialized first time we get here and reused later
			pfc::string_formatter str;
			obj->run_simple( location, &info, str );
			// Make MD5 hash of the string, then reduce it to 64-bit metadb_index_hash
			return static_api_ptr_t<hasher_md5>()->process_single_string( str ).xorHalve();
		}
	};
	
	// Static instance, never destroyed (dies with the process)
	// Uses service_impl_single_t, reference counting disabled
	static metadb_index_client_impl * g_client = new service_impl_single_t<metadb_index_client_impl>;
	

	// An init_stage_callback to hook ourselves into the metadb
	// We need to do this properly early to prevent dispatch_global_refresh() from new fields that we added from hammering playlists etc
	class init_stage_callback_impl : public init_stage_callback {
	public:
		void on_init_stage(t_uint32 stage) {
			if (stage == init_stages::before_config_read) {
				static_api_ptr_t<metadb_index_manager> api;
				// Important, handle the exceptions here!
				// This will fail if the files holding our data are somehow corrupted.
				try {
					api->add(g_client, guid_foo_sample_rating_index, retentionPeriod);
				} catch (std::exception const & e) {
					api->remove(guid_foo_sample_rating_index);
					FB2K_console_formatter() << "[foo_sample rating] Critical initialization failure: " << e;
					return;
				}
				api->dispatch_global_refresh();
			}
		}
	};

	static service_factory_single_t<init_stage_callback_impl> g_init_stage_callback_impl;

	typedef uint32_t rating_t;
	static const rating_t rating_invalid = 0;
	static const rating_t rating_max = 5;

	static rating_t rating_get(metadb_index_hash hash, static_api_ptr_t<metadb_index_manager> & api) {
		rating_t rating;
		if (api->get_user_data_here(guid_foo_sample_rating_index, hash, &rating, sizeof(rating)) != sizeof(rating)) return rating_invalid;
		return rating;
	}

	static rating_t rating_get( metadb_index_hash hash ) {
		static_api_ptr_t<metadb_index_manager> api;
		return rating_get(hash, api);
	}

	static void rating_set( metadb_index_hash hash, rating_t rating ) {
		static_api_ptr_t<metadb_index_manager> api;
		api->set_user_data(guid_foo_sample_rating_index, hash, &rating, sizeof(rating) );
	}

	// Provider of the %foo_sample_rating% field
	class metadb_display_field_provider_impl : public metadb_display_field_provider {
	public:
		t_uint32 get_field_count() {
			return 1;
		}
		void get_field_name(t_uint32 index, pfc::string_base & out) {
			PFC_ASSERT( index == 0 );
			out = "foo_sample_rating";
		}
		bool process_field(t_uint32 index, metadb_handle * handle, titleformat_text_out * out) {
			PFC_ASSERT( index == 0 );
			
			metadb_index_hash hash;
			if (!g_client->hashHandle( handle, hash ) ) return false;

			rating_t rating = rating_get( hash );
			if ( rating == rating_invalid ) return false;

			out->write_int( titleformat_inputtypes::meta, rating );

			return true;
		}
	};

	static service_factory_single_t<metadb_display_field_provider_impl> g_metadb_display_field_provider_impl;

	class contextmenu_rating : public contextmenu_item_simple {
	public:
		GUID get_parent() {
			return guid_foo_sample_contextmenu_group;
		}
		unsigned get_num_items() {
			return 1;
		}
		void get_item_name(unsigned p_index, pfc::string_base & p_out) {
			p_out = "Cycle rating";
		}
		void context_command(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller) {
			const size_t count = p_data.get_count();
			if ( count == 0 ) return;
			
			rating_t rating = rating_invalid;

			// Sorted/dedup'd set of all hashes of p_data items.
			// pfc::avltree_t<> is pfc equivalent of std::set<>.
			// We go the avltree_t<> route because more than one track in p_data might produce the same hash value, see metadb_index_client_impl / strPinTo
			pfc::avltree_t<metadb_index_hash> allHashes;
			for( size_t w = 0; w < count; ++w ) {
				metadb_index_hash hash;
				if (g_client->hashHandle(p_data[w], hash)) {
					allHashes += hash;

					// Take original rating to increment from the first selected song
					if (w == 0) rating = rating_get(hash);
				}
			}

			if ( allHashes.get_count() == 0 ) {
				FB2K_console_formatter() << "[foo_sample rating] Could not hash any of the tracks due to unavailable metadata, bailing";
				return;
			}

			// Now cycle the rating value
			if ( rating == rating_invalid ) rating = 1;
			else if ( rating >= rating_max ) rating = rating_invalid;
			else ++ rating;
			
			// Now set the new rating
			pfc::list_t<metadb_index_hash> lstChanged; // Linear list of hashes that actually changed
			for( auto iter = allHashes.first(); iter.is_valid(); ++iter ) {
				const metadb_index_hash hash = *iter;
				if ( rating_get(hash) != hash ) {
					rating_set( hash, rating );
					lstChanged += hash;
				}
			}

			FB2K_console_formatter() << "[foo_sample rating] " << lstChanged.get_count() << " entries updated";
			if ( lstChanged.get_count() > 0 ) {
				// This gracefully tells everyone about what just changed, in one pass regardless of how many items got altered
				static_api_ptr_t<metadb_index_manager>()->dispatch_refresh(guid_foo_sample_rating_index, lstChanged );
			}

		}
		GUID get_item_guid(unsigned p_index) {
			return guid_foo_sample_rating_contextmenu;
		}
		bool get_item_description(unsigned p_index, pfc::string_base & p_out) {
			PFC_ASSERT( p_index == 0 );
			p_out = "Alters foo_sample's rating on one or more selected tracks. Use %foo_sample_rating% to display the rating."; 
			return true;
		}
	};

	static contextmenu_item_factory_t< contextmenu_rating > g_contextmenu_rating;

	static pfc::string_formatter formatRatingDump() {
		static_api_ptr_t<metadb_index_manager> api;
		pfc::list_t<metadb_index_hash> hashes;
		api->get_all_hashes(guid_foo_sample_rating_index, hashes);
		pfc::string_formatter message;
		message << "The database contains " << hashes.get_count() << " hashes.\n";
		for( size_t hashWalk = 0; hashWalk < hashes.get_count(); ++ hashWalk ) {
			auto hash = hashes[hashWalk];
			message << pfc::format_hex( hash, 8 ) << " : ";
			auto rating = rating_get(hash, api);
			if ( rating == rating_invalid ) message << "no rating";
			else message << "rating " << rating;
			
			metadb_handle_list tracks;
			
			// Note that this returns only handles present in the media library
			
			// Extra work is required if the user has no media library but only playlists, 
			// have to walk the playlists and match hashes by yourself instead of calling this method
			api->get_ML_handles(guid_foo_sample_rating_index, hash, tracks);
			

			if ( tracks.get_count() == 0 ) message << ", no matching tracks in Media Library\n";
			else {
				message << ", " << tracks.get_count() << " matching track(s)\n";
				for( size_t w = 0; w < tracks.get_count(); ++ w ) {
					// pfc string formatter operator<< for metadb_handle prints the location
					message << tracks[w] << "\n";
				}
			}
		}

		return message;
	}

	class mainmenu_rating : public mainmenu_commands {
	public:
		t_uint32 get_command_count() {
			return 1;
		}
		GUID get_command(t_uint32 p_index) {
			return guid_foo_sample_rating_mainmenu;
		}
		void get_name(t_uint32 p_index, pfc::string_base & p_out) {
			PFC_ASSERT( p_index == 0 );
			p_out = "Dump rating database";
		}
		bool get_description(t_uint32 p_index, pfc::string_base & p_out) {
			PFC_ASSERT(p_index == 0);
			p_out = "Shows a dump of the foo_sample rating database."; return true;
		}
		GUID get_parent() {
			return guid_foo_sample_mainmenu_group;
		}
		void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
			PFC_ASSERT( p_index == 0 );

			try {

				auto msg = formatRatingDump();
				
				popup_message::g_show(msg, "foo_sample rating dump");
			} catch(std::exception const & e) {
				// should not really get here
				popup_message::g_complain("Rating dump failed", e);
			}
		}
	};
	static service_factory_single_t<mainmenu_rating> g_mainmenu_rating;


	// This class provides our information for the properties dialog
	class track_property_provider_impl : public track_property_provider_v2 {
	public:
		void enumerate_properties(metadb_handle_list_cref p_tracks, track_property_callback & p_out) {
			pfc::avltree_t<metadb_index_hash> hashes;
			const size_t trackCount = p_tracks.get_count();
			for( size_t trackWalk = 0; trackWalk < trackCount; ++trackWalk ) {
				metadb_index_hash hash;
				if ( g_client->hashHandle( p_tracks[trackWalk], hash ) ) {
					hashes += hash;
				}
			}

			pfc::string8 strAverage = "N/A", strMin = "N/A", strMax = "N/A";

			{
				static_api_ptr_t<metadb_index_manager> api; // reuse object in a loop
				size_t count = 0;
				rating_t minval = rating_invalid;
				rating_t maxval = rating_invalid;
				uint64_t accum = 0;
				for( auto i = hashes.first(); i.is_valid(); ++i ) {
					auto r = rating_get(*i, api);
					if ( r != rating_invalid ) {
						++count;
						accum += r;

						if ( minval == rating_invalid || minval > r ) minval = r;
						if ( maxval == rating_invalid || maxval < r ) maxval = r;
					}
				}

				if ( count > 0 ) {
					strMin = pfc::format_uint( minval );
					strMax = pfc::format_uint( maxval );
					strAverage = pfc::format_float( (double) accum / (double) count, 0, 3 );
				}
			}

			p_out.set_property(strPropertiesGroup, 0, "Average Rating", strAverage );
			p_out.set_property(strPropertiesGroup, 1, "Minimum Rating", strMin );
			p_out.set_property(strPropertiesGroup, 2, "Maximum Rating", strMax );
		}
		void enumerate_properties_v2(metadb_handle_list_cref p_tracks, track_property_callback_v2 & p_out) {
			if ( p_out.is_group_wanted( strPropertiesGroup ) ) {
				enumerate_properties( p_tracks, p_out );
			}
		}
		
		bool is_our_tech_info(const char * p_name) { 
			// If we do stuff with tech infos read from the file itself (see file_info::info_* methods), signal whether this field belongs to us
			// We don't do any of this, hence false
			return false; 
		}

	};


	static service_factory_single_t<track_property_provider_impl> g_track_property_provider_impl;
}
