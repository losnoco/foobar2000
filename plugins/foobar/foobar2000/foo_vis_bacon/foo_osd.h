#include "foobar2000.h"

namespace foo_osd
{
	extern const GUID pop_up;
	extern const GUID toggle_auto_pop;
	extern const GUID show_current_playlist;
	extern const GUID show_current_volume;
	extern const GUID hide;

	class control
	{
	private:
		bool disabled;
		bool orig;

	public:
		control()
		{
			disabled = false;
		}

		void disable()
		{
			if (!disabled)
			{
				orig = menu_manager::is_command_checked(toggle_auto_pop);
				if (orig)
				{
					menu_manager::run_command(toggle_auto_pop);
					menu_manager::run_command(hide);
				}
				disabled = true;
			}
		}

		void enable()
		{
			if (disabled && orig)
			{
				if (!menu_manager::is_command_checked(toggle_auto_pop))
					menu_manager::run_command(toggle_auto_pop);
				disabled = false;
			}
		}
	};
}
