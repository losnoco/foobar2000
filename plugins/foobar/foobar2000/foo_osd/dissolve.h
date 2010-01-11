class dissolve
{
private:
	unsigned width, height, decay;
	void * ptr;
public:
	dissolve(void * in, unsigned w, unsigned h, unsigned d);
	~dissolve() {}

	bool draw();
};
