class  VbriHeaderTable { 

public: 

    int				SampleRate;
    char			VbriSignature[5];
    unsigned int	        VbriVersion;
    unsigned int			VbriDelay;
    unsigned int	        VbriQuality;
    unsigned int	        VbriStreamBytes;
    unsigned int	        VbriStreamFrames;
    unsigned int	        VbriTableSize;
    unsigned int	        VbriTableScale;
    unsigned int	        VbriEntryBytes;
    unsigned int	        VbriEntryFrames;
    int		*		VbriTable;
    

	VbriHeaderTable()
	{
		VbriTable = 0;
	}
	~VbriHeaderTable()
	{
		if (VbriTable) delete[] VbriTable;
	}

} ;

class CVbriHeader{

public: 
  
  CVbriHeader();
  ~CVbriHeader();
  
  int				readVbriHeader(unsigned char *Hbuffer);

  int				seekPointByTime(float EntryTimeInSeconds);
  float				seekTimeByPoint(unsigned int EntryPointInBytes);
  int				seekPointByPercent(float percent);

  float				totalDuration();
  int				totalFrames();

private:

  int				getSampleRate(unsigned char * buffer);
  int				readFromBuffer ( unsigned char * HBuffer, int length );

  VbriHeaderTable	VBHeader;

  int				position ;

  enum offset{
    
    BYTE	=		1,
    WORD	=		2,
    DWORD	=		4
    
  };

};
