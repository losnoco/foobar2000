#ifndef _DEQUE_H_
#define _DEQUE_H_

// whee! std::deque replacement, ala PFC. not the same, but good enough for me

template <class T>
class deque
{
	unsigned first;
	unsigned written;
	unsigned size;
	unsigned filled;
	mem_block_t<T> theBlock;
public:
	deque(unsigned s)
	{
		theBlock.set_size(s);
		theBlock.zeromemory();
		first = 0;
		written = 0;
		size = s;
		filled = s;
	}

	void write(const T* src, unsigned count)
	{
		if ((filled += count) > size)
		{
			first = (first + filled - size) % size;
			filled = size;
		}
		written = theBlock.write_circular(written, src, count);
	}

	void push(T val)
	{
		write(&val, 1);
	}

	void read(T* dest, unsigned count)
	{
		theBlock.read_circular(first, dest, count);
	}

	void clear()
	{
		theBlock.zeromemory();
		first = 0;
		written = 0;
		filled = size;
	}
};

#endif
