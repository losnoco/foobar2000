/* Winamp plugin for ".mpc Musepack", based on miniSDK by Nullsoft */
/* by Andree Buschmann, contact: Andree.Buschmann@web.de           */
/* --------- ".mpc Musepack" is a registered trademark  ---------- */


/************************ DEFINES ************************/
//#define WINAMP
//#define WM_WA_MPEG_EOF  (WM_USER + 2)                   // post this to the main window at end of file
//#define NCH_MAX         2                               // number of channels (only stereo!)
//#define BPS             16                              // bits per sample (only 16 bps!)
//#define BYTES           4                               // = (NCH_MAX*(BPS/8)) -> number of bytes needed for one stereo/16bit-sample
//#define M_PI            3.141592653589793238462643383276

                                                        // 2nd "*2" for reconstruction of exact filelength


/*********************** INCLUDES ************************/
#include "stdafx.h"



long  JumpID3v2     ( MPC_Reader& fp );

static const char*
Stringify ( unsigned int profile )            // profile is 0...15, where 7...13 is used
{
    static const char   na    [] = "n.a.";
    static const char*  Names [] = {
        na, "'Unstable/Experimental'", na, na,
        na, "'quality 0'", "'quality 1'", "'Telephone'",
        "'Thumb'", "'Radio'", "'Standard'", "'Xtreme'",
        "'Insane'", "'BrainDead'", "'quality 9'", "'quality 10'"
    };

    return profile >= sizeof(Names)/sizeof(*Names)  ?  na  :  Names [profile];
}

// read information from SV8 header
int StreamInfo::ReadHeaderSV8 ( MPC_Reader&  fp )
{
//    Update (simple.StreamVersion);
    return 0;
}

// read information from SV7 header
int StreamInfo::ReadHeaderSV7 ( unsigned int HeaderData [8] )
{
    const long samplefreqs [4] = { 44100, 48000, 37800, 32000 };

    //unsigned int    HeaderData [8];
    unsigned short  EstimatedPeakTitle = 0;

    if ( simple.StreamVersion > 0x71 ) {
//        Update (simple.StreamVersion);
        return 0;
    }

    /*
    if ( fp.seek ( simple.HeaderPosition, SEEK_SET ) != 0 )         // seek to header start
        return ERROR_CODE_FILE;
    if ( fp.read ( HeaderData, sizeof HeaderData) != sizeof HeaderData )
        return ERROR_CODE_FILE;
    */

    simple.Bitrate          = 0;
    simple.Frames           =  HeaderData[1];
    simple.IS               = 0;
    simple.MS               = (HeaderData[2] >> 30) & 0x0001;
    simple.MaxBand          = (HeaderData[2] >> 24) & 0x003F;
    simple.BlockSize        = 1;
    simple.Profile          = (HeaderData[2] << 8) >> 28;
    simple.ProfileName      = Stringify ( simple.Profile );

    simple.SampleFreq       = samplefreqs [(HeaderData[2]>>16) & 0x0003];

    EstimatedPeakTitle      =  HeaderData[2]        & 0xFFFF;         // read the ReplayGain data
    simple.GainTitle        = (HeaderData[3] >> 16) & 0xFFFF;
    simple.PeakTitle        =  HeaderData[3]        & 0xFFFF;
    simple.GainAlbum        = (HeaderData[4] >> 16) & 0xFFFF;
    simple.PeakAlbum        =  HeaderData[4]        & 0xFFFF;

    simple.IsTrueGapless    = (HeaderData[5] >> 31) & 0x0001;         // true gapless: used?
    simple.LastFrameSamples = (HeaderData[5] >> 20) & 0x07FF;         // true gapless: valid samples for last frame

    simple.EncoderVersion   = (HeaderData[6] >> 24) & 0x00FF;
    if ( simple.EncoderVersion == 0 ) {
        sprintf ( simple.Encoder, "Buschmann 1.7.0...9, Klemm 0.90...1.05" );
    } else {
        switch ( simple.EncoderVersion % 10 ) {
        case 0:
            sprintf ( simple.Encoder, "Release %u.%u", simple.EncoderVersion/100, simple.EncoderVersion/10%10 );
            break;
        case 2: case 4: case 6: case 8:
            sprintf ( simple.Encoder, "Beta %u.%02u", simple.EncoderVersion/100, simple.EncoderVersion%100 );
            break;
        default:
            sprintf ( simple.Encoder, "--Alpha-- %u.%02u", simple.EncoderVersion/100, simple.EncoderVersion%100 );
            break;
        }
    }

//    if ( simple.PeakTitle == 0 )                                      // there is no correct PeakTitle contained within header
//        simple.PeakTitle = (unsigned short)(EstimatedPeakTitle * 1.18);
//    if ( simple.PeakAlbum == 0 )
//        simple.PeakAlbum = simple.PeakTitle;                          // no correct PeakAlbum, use PeakTitle

    //simple.SampleFreq    = 44100;                                     // AB: used by all files up to SV7
    simple.Channels      = 2;

    return ERROR_CODE_OK;
}

// read information from SV4-SV6 header
int StreamInfo::ReadHeaderSV6 ( unsigned int HeaderData [8] )
{
    //unsigned int    HeaderData [8];

    /*
    if ( fp.seek (  simple.HeaderPosition, SEEK_SET ) != 0 )         // seek to header start
        return ERROR_CODE_FILE;
    if ( fp.read ( HeaderData, sizeof HeaderData ) != sizeof HeaderData )
        return ERROR_CODE_FILE;
    */

    simple.Bitrate          = (HeaderData[0] >> 23) & 0x01FF;         // read the file-header (SV6 and below)
    simple.IS               = (HeaderData[0] >> 22) & 0x0001;
    simple.MS               = (HeaderData[0] >> 21) & 0x0001;
    simple.StreamVersion    = (HeaderData[0] >> 11) & 0x03FF;
    simple.MaxBand          = (HeaderData[0] >>  6) & 0x001F;
    simple.BlockSize        = (HeaderData[0]      ) & 0x003F;
    simple.Profile          = 0;
    simple.ProfileName      = Stringify ( -1 );
    if ( simple.StreamVersion >= 5 )
        simple.Frames       =  HeaderData[1];                         // 32 bit
    else
        simple.Frames       = (HeaderData[1]>>16);                    // 16 bit

    simple.GainTitle        = 0;                                      // not supported
    simple.PeakTitle        = 0;
    simple.GainAlbum        = 0;
    simple.PeakAlbum        = 0;

    simple.LastFrameSamples = 0;
    simple.IsTrueGapless    = 0;

    simple.EncoderVersion   = 0;
    simple.Encoder[0]       = '\0';

    if ( simple.StreamVersion == 7 ) return ERROR_CODE_SV7BETA;       // are there any unsupported parameters used?
    if ( simple.Bitrate       != 0 ) return ERROR_CODE_CBR;
    if ( simple.IS            != 0 ) return ERROR_CODE_IS;
    if ( simple.BlockSize     != 1 ) return ERROR_CODE_BLOCKSIZE;

    if ( simple.StreamVersion < 6 )                                   // Bugfix: last frame was invalid for up to SV5
        simple.Frames -= 1;

    simple.SampleFreq    = 44100;                                     // AB: used by all files up to SV7
    simple.Channels      = 2;

    if ( simple.StreamVersion < 4  ||  simple.StreamVersion > 7 )
        return ERROR_CODE_INVALIDSV;

    return ERROR_CODE_OK;
}

// reads file header and tags
int StreamInfo::ReadStreamInfo ( MPC_Reader & fp)
{
    unsigned int    HeaderData[8];
    int             Error = 0;

    //memset ( &simple, 0, sizeof (BasicData) );                          // Reset Info-Data


		if ( (simple.HeaderPosition = JumpID3v2 (fp)) < 0 )                 // get header position
			return ERROR_CODE_FILE;

		if ( !fp.seek ( simple.HeaderPosition ) )            // seek to first byte of mpc data
			return ERROR_CODE_FILE;
    if ( fp.read ( HeaderData, 8*4 ) != 8*4 )
        return ERROR_CODE_FILE;
	if (!fp.seek(simple.HeaderPosition+6*4)) return ERROR_CODE_FILE;

/*	    if ( fp.read ( HeaderData, 6*4 ) != 6*4 )
		    return ERROR_CODE_FILE;
		HeaderData[6]=HeaderData[7]=0;
	*/
    /*
    if ( fp.seek ( 0L, SEEK_END ) != 0 )                               // get filelength
        return ERROR_CODE_FILE;
    simple.TotalFileLength = fp.tell();
    */
    simple.TotalFileLength = fp.get_size ();
    simple.TagOffset = simple.TotalFileLength;

    if ( memcmp ( HeaderData, "MP+", 3 ) == 0 ) {                       // check version
        simple.StreamVersion = HeaderData[0] >> 24;

        if ( (simple.StreamVersion & 15) >= 8 )                         // StreamVersion 8
            Error = ReadHeaderSV8 ( fp );
        else if ( (simple.StreamVersion & 15) == 7 )                    // StreamVersion 7
            Error = ReadHeaderSV7 ( HeaderData );
    } else {                                                            // StreamVersion 4-6
        Error = ReadHeaderSV6 ( HeaderData );
    }


    simple.PCMSamples = 1152 * simple.Frames - 576;                     // estimation, exact value needs too much time
    if ( simple.PCMSamples > 0 )
        simple.AverageBitrate = (simple.TagOffset - simple.HeaderPosition) * 8. * simple.SampleFreq / simple.PCMSamples;
    else
        simple.AverageBitrate = 0;

    return Error;
}

/* end of in_mpc.c */
