#include "foobar2000.h"

namespace {
	using namespace fb2k;

	class callOnReleaseImpl : public service_base {
    public:
        callOnReleaseImpl( std::function<void () > f_) : f(f_) {}
        std::function<void ()> f;
        
        ~callOnReleaseImpl () {
            try {
                f();
            } catch(...) {}
        }
    };
}

namespace fb2k {
    objRef callOnRelease( std::function< void () > f) {
        return new service_impl_t< callOnReleaseImpl > (f);
    }
    objRef callOnReleaseInMainThread( std::function< void () > f) {
        return callOnRelease( [f] {
           fb2k::inMainThread2( f );
        });
    }
}
