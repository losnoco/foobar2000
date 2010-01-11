//crap, throwing PTypes into another project
#include <ptypes.h>
#include <ptime.h>
#include <pasync.h>

USING_PTYPES

#define MTRAND_SYNC

class mt_rand
{
private:
	enum
	{
		N = 624,
		M = 397
	};

	unsigned long state[N]; /* the array for the state vector  */
	int left;
	int initf;
	unsigned long *next;

#ifdef MTRAND_SYNC
	mutex sync;
#endif

public:
	mt_rand();
	~mt_rand() {}

	void init_genrand(unsigned long s);
	void init_by_array(unsigned long init_key[], unsigned long key_length);

	unsigned long genrand_int32(void);
	long genrand_int31(void);

	double genrand_real1(void);
	double genrand_real2(void);
	double genrand_real3(void);

	double genrand_res53(void);

private:
	void next_state(void);
};
