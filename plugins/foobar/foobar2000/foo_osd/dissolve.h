class dissolve
{
private:
	unsigned width, height, decay;
	void * ptr;
public:
	dissolve(unsigned w, unsigned h, unsigned d);
	~dissolve() {}

	bool draw(void * ptr);
};
