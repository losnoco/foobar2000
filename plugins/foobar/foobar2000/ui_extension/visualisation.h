#ifndef _VIS_EXTENSION_H_
#define _VIS_EXTENSION_H_

namespace ui_extension
{

/**
 * Interface for vis_extension_host service.
\todo update
 */
class NOVTABLE visualisation_host : public service_base
{
public:
	/**
	 * vis_extension_host class GUID
	 * 
	 * Used by service mechanism.
	 */
	static const GUID class_guid;

	/**
	 * Get vis_extension_host class GUID
	 * 
	 * Used by service mechanism.
	 *
	 * \return class GUID
	 */
	static inline const GUID & get_class_guid(){return class_guid;}

	struct vis_paint_struct
	{
		HDC dc;
		RECT rc_client;
		void * reserved;
	};

	/**
	 * \todo document
	 */
	virtual void begin_paint(vis_paint_struct & p_out)=0;

	/**
	 * \todo document
	 */
	virtual void end_paint(vis_paint_struct & p_in)=0;
};

/**
 * Service factory for vis extensions hosts.
 * \par Usage example
 * \code
 * static vis_extension_host_factory< my_vis_extension_host > foo_host;
 * \endcode
 */
template<class T>
class visualisation_host_factory : public service_factory_t<visualisation_host,T> {};

template<class T>
class visualization_host_factory : public visualisation_host_factory<T> {};

/**
 * Interface for vis_extension service.
 */
class NOVTABLE visualisation : public extension_base
{
public:

	/**
	 * vis_extension class GUID
	 *
	 * Used by service mechanism.
	 */
	static const GUID class_guid;

	/**
	 * Get vis_extension class GUID
	 * 
	 * Used by service mechanism.
	 *
	 * \return class GUID
	 */
	static inline const GUID & get_class_guid(){return class_guid;}

	/**
	 * Enable.
	 * \todo document
	 */
	virtual void enable(const visualisation_host_ptr & p_host)=0; 

	/**
	 * Paint.
	 * \todo document
	 */
	virtual void paint_background(HDC dc, const RECT * rc_area)=0; 

	/**
	 * Disable.
	 */
	virtual void disable()=0; 

	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	/**
	 * Create extension by GUID.
	 *
	 * \param[in] guid GUID of a vis_extension
	 * \return A pointer to an instance of a vis_extension with the given
	 * GUID, or NULL if no such vis_extension was found. In the first case,
	 * the returned instance must be freed using <code>service_release()</code>.
	 */
	static inline void create_by_guid(const GUID & guid, visualisation_ptr & p_out)
	{
		service_enum_t<ui_extension::visualisation> e;
		visualisation_ptr ptr;

		
		if (e.first(ptr)) do {
			if (ptr->get_extension_guid() == guid)
			{
				p_out.copy(ptr);
				return;
				
			}
		} while(e.next(ptr));

	}
};

/**
 * Service factory for vis extensions.
 * \par Usage example
 * \code
 * static vis_extension_factory< my_vis_extension > foo_vis;
 * \endcode
 */
template<class T>
class visualisation_factory : public service_factory_t<ui_extension::visualisation,T> {};

//template<class T>
//class visualization_factory : public visualisation_factory<T>{};

}
#endif