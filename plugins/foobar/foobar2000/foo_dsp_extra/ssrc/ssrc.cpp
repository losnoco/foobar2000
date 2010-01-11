#include <math.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>

#include "ssrc.h"

#include "../math_shared.h"

typedef float ssrc_real;//internal data type
typedef __int64 ssrc_int64;

//helper
template<class T>
class mem_ops
{
public:
	static void copy(T* dst,const T* src,unsigned count) {memcpy(dst,src,count*sizeof(T));}
	static void move(T* dst,const T* src,unsigned count) {memmove(dst,src,count*sizeof(T));}
	static void set(T* dst,int val,unsigned count) {memset(dst,val,count*sizeof(T));}
	static void setval(T* dst,T val,unsigned count) {for(;count;count--) *(dst++)=val;}
	static T* alloc(unsigned count) {return reinterpret_cast<T*>(malloc(count * sizeof(T)));}
	static T* alloc_zeromem(unsigned count)
	{
		T* ptr = alloc(count);
		if (ptr) set(ptr,0,count);
		return ptr;
	}
	static T* realloc(T* ptr,unsigned count)
	{return ptr ? reinterpret_cast<T*>(::realloc(reinterpret_cast<void*>(ptr),count * sizeof(T))) : alloc(count);}

	static void free(T* ptr) {::free(reinterpret_cast<void*>(ptr)); }
	inline static T make_null_item()
	{
		char item[sizeof(T)];
		memset(&item,0,sizeof(T));
		return * reinterpret_cast<T*>(&item);
	}
	inline static void swap(T& item1,T& item2) {T temp; temp=item1; item1=item2; item2=temp;}
};



class ssrc_buffer
{
private:
	ssrc_sample * m_buffer;
	unsigned m_buffer_size;
	unsigned buf_data;
	void check_size(unsigned p_size);
public:
	ssrc_buffer();
	~ssrc_buffer();
	inline ssrc_sample * GetBuffer(unsigned * siz) {*siz=buf_data;return m_buffer;}
	inline ssrc_sample * GetBuffer() {return m_buffer;}
	inline unsigned Size() {return buf_data;}
	void Read(unsigned size);
	void Reset();
	void Write(const ssrc_sample * ptr,unsigned size);
};


class Resampler_base : public ssrc_resampler
{
private:
	void bufloop(bool finish);
protected:
	ssrc_buffer in,out;	

	int nch,sfrq,dfrq;
	double AA,DF;
	int FFTFIRLEN;

	Resampler_base(const ssrc_config & c);

	virtual unsigned Resample(ssrc_sample * input,unsigned int size,bool ending) = 0;

	void make_outbuf(int samples, const ssrc_real* outbuf);
	void make_inbuf(int nsmplread, int inbuflen, const ssrc_sample* rawinbuf, ssrc_real* inbuf, int toberead);


public:

	void Write(const ssrc_sample* ptr,unsigned size);
	void Finish();
	unsigned GetDataDoneSize();
	const ssrc_sample * GetDataDoneBuffer();
	void ResetDataDone();
	double GetLatency();
};

void Resampler_base::Finish() {bufloop(1);}

unsigned Resampler_base::GetDataDoneSize()
{
	return out.Size();
}

const ssrc_sample * Resampler_base::GetDataDoneBuffer()
{
	return out.GetBuffer();
}

void Resampler_base::ResetDataDone()
{
	out.Reset();
}

#pragma warning(disable:4244)

#define M 15

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028842
#endif

extern "C" 
{
extern double dbesi0(double);
}


static double alpha(double a)
{
  if (a <= 21) return 0;
  if (a <= 50) return 0.5842*pow(a-21,0.4)+0.07886*(a-21);
  return 0.1102*(a-8.7);
}


static double win(double n,int len,double alp,double iza)
{
  return dbesi0(alp*sqrt(1-4*n*n/(((double)len-1)*((double)len-1))))/iza;
}

static double sinc(double x)
{
  return x == 0 ? 1 : sin(x)/x;
}

static double hn_lpf(int n,double lpf,double fs)
{
  double t = 1/fs;
  double omega = 2*M_PI*lpf;
  return 2*lpf*t*sinc(n*omega*t);
}




static int gcd(int x, int y)
{
    int t;

    while (y != 0) {
        t = x % y;  x = y;  y = t;
    }
    return x;
}

bool ssrc_resampler::CanResample(unsigned sfrq,unsigned dfrq)
{
	if (sfrq==dfrq) return 1;
    int frqgcd = gcd(sfrq,dfrq);

	if (dfrq>sfrq)
	{
		int fs1 = sfrq / frqgcd * dfrq;

		if (fs1/dfrq == 1) return 1;
		else if (fs1/dfrq % 2 == 0) return 1;
		else if (fs1/dfrq % 3 == 0) return 1;
		else return 0;
	}
	else
	{
		if (dfrq/frqgcd == 1) return 1;
		else if (dfrq/frqgcd % 2 == 0) return 1;
		else if (dfrq/frqgcd % 3 == 0) return 1;
		else return 0;
	}
}

void ssrc_buffer::Reset()
{
	buf_data = 0;
}

void ssrc_buffer::Read(unsigned size)
{
	if (size)
	{
		if (buf_data==size) buf_data=0;
		else
		{
			mem_ops<ssrc_sample>::move(m_buffer,m_buffer+size,(buf_data-size));
			buf_data-=size;
		}
	}
}

void ssrc_buffer::Write(const ssrc_sample * ptr,unsigned size)
{
	check_size(buf_data + size);
	mem_ops<ssrc_sample>::copy(m_buffer+buf_data,ptr,size);
	buf_data+=size;
}

void Resampler_base::bufloop(bool finish)
{
	unsigned s;
	ssrc_sample * ptr = in.GetBuffer(&s);
	int done=0;
	for(;;)
	{
		unsigned d = Resample(ptr,s-done,finish);
		if (d==0) break;
		done+=d;
		ptr+=d;
	}
	in.Read(done);
}

void Resampler_base::Write(const ssrc_sample * input,unsigned size)
{
	in.Write(input,size);
	bufloop(0);
}

static int muldiv64(ssrc_int64 p1,ssrc_int64 p2,ssrc_int64 p3)
{
	return p1 * p2 / p3;
}

void Resampler_base::make_inbuf(int nsmplread, int inbuflen, const ssrc_sample* rawinbuf, ssrc_real* inbuf, int toberead)
{
	const int	MaxLoop = nsmplread * nch;
	const int	InbufBase = inbuflen * nch;

	for(int i = 0; i < MaxLoop; i++) {
		inbuf[InbufBase + i] = rawinbuf[i];
	}

	size_t	ClearSize = toberead - nsmplread;

	if(ClearSize) {
		memset(inbuf + InbufBase + MaxLoop, 0, ClearSize * nch * sizeof ssrc_real);
	}
}

void Resampler_base::make_outbuf(int samples, const ssrc_real* outbuf)
{
	int todo = samples * nch;
	
	if (sizeof(ssrc_sample) == sizeof(ssrc_real))
	{
		out.Write((const ssrc_sample*)outbuf,todo);
	}
	else
	{
		const unsigned tempsize = 256;
		ssrc_sample temp[tempsize];
		while(todo>0)
		{
			int delta = todo;
			if (delta > tempsize) delta = tempsize;
			int todo2 = delta;
			ssrc_sample * tempptr = temp;
			do *(tempptr++) = (ssrc_sample)(*(outbuf++)); while(--todo2 > 0);
			out.Write(temp,delta);
			todo -= delta;
		}
	}
}

double Resampler_base::GetLatency()
{
	return (double)in.Size() / (double)(sfrq*nch) + (double)out.Size() / (double)(dfrq*nch);
}

class Upsampler : public Resampler_base
{
  int frqgcd,osf,fs1,fs2;
  ssrc_real **stage1,*stage2;
  int n1,n1x,n1y,n2,n2b;
  int filter2len;
  int *f1order,*f1inc;
  int *fft_ip;// = NULL;
  ssrc_real *fft_w;// = NULL;
  //unsigned char *rawinbuf,*rawoutbuf;
  ssrc_real *inbuf,*outbuf;
  ssrc_real **buf1,**buf2;
  int spcount;
  int i,j;

		int n2b2;//=n2b/2;
		int rp;        // inbufのfs1での次に読むサンプルの場所を保持
		int ds;        // 次にdisposeするsfrqでのサンプル数
//		int nsmplwrt1; // 実際にファイルからinbufに読み込まれた値から計算した
					   // stage2 filterに渡されるサンプル数
//		int nsmplwrt2; // 実際にファイルからinbufに読み込まれた値から計算した
					   // stage2 filterに渡されるサンプル数
		int s1p;       // stage1 filterから出力されたサンプルの数をn1y*osfで割った余り
		ssrc_int64 sumread,sumwrite;
		int osc;
		ssrc_real *ip,*ip_backup;
		int s1p_backup,osc_backup;
		int p;
		int inbuflen;
		int delay;// = 0;


public:
	Upsampler(const ssrc_config & c) : Resampler_base(c)
	{
		fft_ip = NULL;
		fft_w = NULL;
		spcount = 0;


		filter2len = FFTFIRLEN; /* stage 2 filter length */

	  /* Make stage 1 filter */

	  {
		double aa = AA; /* stop band attenuation(dB) */
		double lpf,delta,d,df,alp,iza;
		double guard = 2;

		frqgcd = gcd(sfrq,dfrq);

		fs1 = sfrq / frqgcd * dfrq;

		if (fs1/dfrq == 1) osf = 1;
		else if (fs1/dfrq % 2 == 0) osf = 2;
		else if (fs1/dfrq % 3 == 0) osf = 3;
		else {
//		  fprintf(stderr,"Resampling from %dHz to %dHz is not supported.\n",sfrq,dfrq);
//		  fprintf(stderr,"%d/gcd(%d,%d)=%d must be divided by 2 or 3.\n",sfrq,sfrq,dfrq,fs1/dfrq);
//		  exit(-1);
			return;
		}

		df = (dfrq*osf/2 - sfrq/2) * 2 / guard;
		lpf = sfrq/2 + (dfrq*osf/2 - sfrq/2)/guard;

		delta = pow(10.0,-aa/20);
		if (aa <= 21) d = 0.9222; else d = (aa-7.95)/14.36;

		n1 = fs1/df*d+1;
		if (n1 % 2 == 0) n1++;

		alp = alpha(aa);
		iza = dbesi0(alp);
		//printf("iza = %g\n",iza);

		n1y = fs1/sfrq;
		n1x = n1/n1y+1;

		f1order = (int*)malloc(sizeof(int)*n1y*osf);
		for(i=0;i<n1y*osf;i++) {
		  f1order[i] = fs1/sfrq-(i*(fs1/(dfrq*osf)))%(fs1/sfrq);
		  if (f1order[i] == fs1/sfrq) f1order[i] = 0;
		}

		f1inc = (int*)malloc(sizeof(int)*n1y*osf);
		for(i=0;i<n1y*osf;i++) {
		  f1inc[i] = f1order[i] < fs1/(dfrq*osf) ? nch : 0;
		  if (f1order[i] == fs1/sfrq) f1order[i] = 0;
		}

		stage1 = (ssrc_real**)malloc(sizeof(ssrc_real *)*n1y);
		stage1[0] = (ssrc_real*)malloc(sizeof(ssrc_real)*n1x*n1y);

		for(i=1;i<n1y;i++) {
		  stage1[i] = &(stage1[0][n1x*i]);
		  for(j=0;j<n1x;j++) stage1[i][j] = 0;
		}

		for(i=-(n1/2);i<=n1/2;i++)
		  {
		stage1[(i+n1/2)%n1y][(i+n1/2)/n1y] = win(i,n1,alp,iza)*hn_lpf(i,lpf,fs1)*fs1/sfrq;
		  }
	  }

	  /* Make stage 2 filter */

	  {
		double aa = AA; /* stop band attenuation(dB) */
		double lpf,delta,d,df,alp,iza;
		int ipsize,wsize;

		delta = pow(10.0,-aa/20);
		if (aa <= 21) d = 0.9222; else d = (aa-7.95)/14.36;

		fs2 = dfrq*osf;

		for(i=1;;i = i * 2)
		  {
		n2 = filter2len * i;
		if (n2 % 2 == 0) n2--;
		df = (fs2*d)/(n2-1);
		lpf = sfrq/2;
		if (df < DF) break;
		  }

		alp = alpha(aa);

		iza = dbesi0(alp);

		for(n2b=1;n2b<n2;n2b*=2);
		n2b *= 2;

		stage2 = (ssrc_real*)malloc(sizeof(ssrc_real)*n2b);

		for(i=0;i<n2b;i++) stage2[i] = 0;

		for(i=-(n2/2);i<=n2/2;i++) {
		  stage2[i+n2/2] = win(i,n2,alp,iza)*hn_lpf(i,lpf,fs2)/n2b*2;
		}

		ipsize    = 2+sqrt((double)n2b);
		fft_ip    = (int*)malloc(sizeof(int)*ipsize);
		fft_ip[0] = 0;
		wsize     = n2b/2;
		fft_w     = (ssrc_real*)malloc(sizeof(ssrc_real)*wsize);

		dsp_math<ssrc_real>::rdft(n2b,1,stage2,fft_ip,fft_w);
	  }

//	  delay=0;
	  n2b2=n2b/2;

		buf1 = (ssrc_real**)malloc(sizeof(ssrc_real *)*nch);
		for(i=0;i<nch;i++)
		  {
		buf1[i] = (ssrc_real*)malloc(sizeof(ssrc_real)*(n2b2/osf+1));
		for(j=0;j<(n2b2/osf+1);j++) buf1[i][j] = 0;
		  }

		buf2 = (ssrc_real**)malloc(sizeof(ssrc_real *)*nch);
		for(i=0;i<nch;i++) buf2[i] = (ssrc_real*)malloc(sizeof(ssrc_real)*n2b);


		inbuf  = (ssrc_real*)calloc(nch*(n2b2+n1x),sizeof(ssrc_real));
		outbuf = (ssrc_real*)malloc(sizeof(ssrc_real)*nch*(n2b2/osf+1));

		s1p = 0;
		rp  = 0;
		ds  = 0;
		osc = 0;

		inbuflen = n1/2/(fs1/sfrq)+1;
		delay = (double)n2/2/(fs2/dfrq);

		sumread = sumwrite = 0;

	}


	unsigned int Resample(ssrc_sample * rawinbuf,unsigned int in_size,bool ending)
	{
	  /* Apply filters */

		int nsmplread,toberead;
		unsigned int rv=0;
		int ch;
		int nsmplwrt1,nsmplwrt2;

		toberead = muldiv64(n2b2,sfrq,dfrq*osf)+1+n1x-inbuflen;
		if (!ending)
		{
			unsigned toberead_size = nch*toberead;
			if (in_size<toberead_size) return 0;
			rv=toberead_size;
			nsmplread=toberead;
		}
		else
		{
			nsmplread=in_size/nch;
			rv=nsmplread*nch;
		}
		
		make_inbuf(nsmplread,inbuflen,rawinbuf,inbuf,toberead);

		sumread += nsmplread;

		inbuflen += toberead;

		//nsmplwrt1 = ((rp-1)*sfrq/fs1+inbuflen-n1x)*dfrq*osf/sfrq;
		//if (nsmplwrt1 > n2b2) nsmplwrt1 = n2b2;
		nsmplwrt1 = n2b2;


		// apply stage 1 filter

		ip = &inbuf[((sfrq*(rp-1)+fs1)/fs1)*nch];

		s1p_backup = s1p;
		ip_backup  = ip;
		osc_backup = osc;

		for(ch=0;ch<nch;ch++)
		{
			ssrc_real *op = &outbuf[ch];
			int fdo = fs1/(dfrq*osf),no = n1y*osf;

			s1p = s1p_backup; ip = ip_backup+ch;

			switch(n1x)
			{
			case 7:
				for(p=0;p<nsmplwrt1;p++)
				{
					int s1o = f1order[s1p];

					buf2[ch][p] =
					stage1[s1o][0] * *(ip+0*nch)+
					stage1[s1o][1] * *(ip+1*nch)+
					stage1[s1o][2] * *(ip+2*nch)+
					stage1[s1o][3] * *(ip+3*nch)+
					stage1[s1o][4] * *(ip+4*nch)+
					stage1[s1o][5] * *(ip+5*nch)+
					stage1[s1o][6] * *(ip+6*nch);
				
					ip += f1inc[s1p];

					s1p++;
					if (s1p == no) s1p = 0;
				}
				break;

			case 9:
				for(p=0;p<nsmplwrt1;p++)
				{
					int s1o = f1order[s1p];

					buf2[ch][p] =
					stage1[s1o][0] * *(ip+0*nch)+
					stage1[s1o][1] * *(ip+1*nch)+
					stage1[s1o][2] * *(ip+2*nch)+
					stage1[s1o][3] * *(ip+3*nch)+
					stage1[s1o][4] * *(ip+4*nch)+
					stage1[s1o][5] * *(ip+5*nch)+
					stage1[s1o][6] * *(ip+6*nch)+
					stage1[s1o][7] * *(ip+7*nch)+
					stage1[s1o][8] * *(ip+8*nch);
				
					ip += f1inc[s1p];

					s1p++;
					if (s1p == no) s1p = 0;
				}
				break;

			 default:
				for(p=0;p<nsmplwrt1;p++)
				{
					ssrc_real tmp = 0;
					ssrc_real *ip2=ip;

					int s1o = f1order[s1p];

					for(i=0;i<n1x;i++)
					{
						tmp += stage1[s1o][i] * *ip2;
						ip2 += nch;
					}
					buf2[ch][p] = tmp;

					ip += f1inc[s1p];

					s1p++;
					if (s1p == no) s1p = 0;
				}
				break;
			}

			osc = osc_backup;

			// apply stage 2 filter

			for(p=nsmplwrt1;p<n2b;p++) buf2[ch][p] = 0;

			//for(i=0;i<n2b2;i++) printf("%d:%g ",i,buf2[ch][i]);

			dsp_math<ssrc_real>::rdft(n2b,1,buf2[ch],fft_ip,fft_w);


			buf2[ch][0] = stage2[0]*buf2[ch][0];
			buf2[ch][1] = stage2[1]*buf2[ch][1]; 



			for(i=1;i<n2b/2;i++)
			{
				ssrc_real re,im;

				re = stage2[i*2  ]*buf2[ch][i*2] - stage2[i*2+1]*buf2[ch][i*2+1];
				im = stage2[i*2+1]*buf2[ch][i*2] + stage2[i*2  ]*buf2[ch][i*2+1];

				//printf("%d : %g %g %g %g %g %g\n",i,stage2[i*2],stage2[i*2+1],buf2[ch][i*2],buf2[ch][i*2+1],re,im);

				buf2[ch][i*2  ] = re;
				buf2[ch][i*2+1] = im;
			 }

			dsp_math<ssrc_real>::rdft(n2b,-1,buf2[ch],fft_ip,fft_w);

			for(i=osc,j=0;i<n2b2;i+=osf,j++)
			{
				ssrc_real f = (buf1[ch][j] + buf2[ch][i]);
				op[j*nch] = f;
			}

			nsmplwrt2 = j;

			osc = i - n2b2;

			for(j=0;i<n2b;i+=osf,j++)
				buf1[ch][j] = buf2[ch][i];

		}

		rp += nsmplwrt1 * (sfrq / frqgcd) / osf;

		if (delay >= nsmplwrt2)
		{
			delay -= nsmplwrt2;
			nsmplwrt2 = 0;
		}
		else
		{
			ssrc_real * p_outbuf = outbuf + delay * nch;
			nsmplwrt2 -= delay;
			delay = 0;

			if (ending)
			{
				int max_write = muldiv64(sumread,dfrq,sfrq)+2-sumwrite;
				if (max_write<0) max_write = 0;
				if (nsmplwrt2>max_write) nsmplwrt2=max_write;				
			}
			make_outbuf(nsmplwrt2,p_outbuf);
		}

		sumwrite += nsmplwrt2;

		{
			int ds = (rp-1)/(fs1/sfrq);

			assert(inbuflen >= ds);

			mem_ops<ssrc_real>::move(inbuf,inbuf+nch*ds,nch*(inbuflen-ds));
			
			inbuflen -= ds;

			rp -= ds*(fs1/sfrq);
		}

		return rv;
	}

	~Upsampler()
	{
		free(f1order);
		free(f1inc);
		free(stage1[0]);
		free(stage1);
		free(stage2);
		free(fft_ip);
		free(fft_w);
		for(i=0;i<nch;i++) free(buf1[i]);
		free(buf1);
		for(i=0;i<nch;i++) free(buf2[i]);
		free(buf2);
		free(inbuf);
		free(outbuf);
	}
};

class Downsampler : public Resampler_base
{
private:
  int frqgcd,osf,fs1,fs2;
  ssrc_real *stage1,**stage2;
  int n2,n2x,n2y,n1,n1b;
  int filter1len;
  int *f2order,*f2inc;
  int *fft_ip;// = NULL;
  ssrc_real *fft_w;// = NULL;
  //unsigned char *rawinbuf,*rawoutbuf;
  ssrc_real *inbuf,*outbuf;
  ssrc_real **buf1,**buf2;
  int i,j;
  int spcount;// = 0;



    int n1b2;// = n1b/2;
    int rp;        // inbufのfs1での次に読むサンプルの場所を保持
    int rps;       // rpを(fs1/sfrq=osf)で割った余り
    int rp2;       // buf2のfs2での次に読むサンプルの場所を保持
    int ds;        // 次にdisposeするsfrqでのサンプル数
//    int nsmplwrt2; // 実際にファイルからinbufに読み込まれた値から計算した
                   // stage2 filterに渡されるサンプル数
    int s2p;       // stage1 filterから出力されたサンプルの数をn1y*osfで割った余り
    int osc;
    ssrc_real *bp; // rp2から計算される．buf2の次に読むサンプルの位置
    int rps_backup,s2p_backup;
    int k,ch,p;
    int inbuflen;//=0;
    ssrc_int64 sumread,sumwrite;
    int delay;// = 0;
    ssrc_real *op;

public:
	Downsampler(const ssrc_config & c) : Resampler_base(c)
  {
		spcount=0;
		fft_ip=0;
		fft_w=0;



  filter1len = FFTFIRLEN; /* stage 1 filter length */

  /* Make stage 1 filter */

  {
    double aa = AA; /* stop band attenuation(dB) */
    double lpf,delta,d,df,alp,iza;
    int ipsize,wsize;

    frqgcd = gcd(sfrq,dfrq);

    if (dfrq/frqgcd == 1) osf = 1;
    else if (dfrq/frqgcd % 2 == 0) osf = 2;
    else if (dfrq/frqgcd % 3 == 0) osf = 3;
    else {
//      fprintf(stderr,"Resampling from %dHz to %dHz is not supported.\n",sfrq,dfrq);
//      fprintf(stderr,"%d/gcd(%d,%d)=%d must be divided by 2 or 3.\n",dfrq,sfrq,dfrq,dfrq/frqgcd);
//      exit(-1);
		return;
    }

    fs1 = sfrq*osf;

    delta = pow(10.0,-aa/20);
    if (aa <= 21) d = 0.9222; else d = (aa-7.95)/14.36;

    n1 = filter1len;
    for(i=1;;i = i * 2)
      {
	n1 = filter1len * i;
	if (n1 % 2 == 0) n1--;
	df = (fs1*d)/(n1-1);
	lpf = (dfrq-df)/2;
	if (df < DF) break;
      }

    alp = alpha(aa);

    iza = dbesi0(alp);

    for(n1b=1;n1b<n1;n1b*=2);
    n1b *= 2;

    stage1 = (ssrc_real*)malloc(sizeof(ssrc_real)*n1b);

    for(i=0;i<n1b;i++) stage1[i] = 0;

    for(i=-(n1/2);i<=n1/2;i++) {
      stage1[i+n1/2] = win(i,n1,alp,iza)*hn_lpf(i,lpf,fs1)*fs1/sfrq/n1b*2;
    }

    ipsize    = 2+sqrt((double)n1b);
    fft_ip    = (int*)malloc(sizeof(int)*ipsize);
    fft_ip[0] = 0;
    wsize     = n1b/2;
    fft_w     = (ssrc_real*)malloc(sizeof(ssrc_real)*wsize);

    dsp_math<ssrc_real>::rdft(n1b,1,stage1,fft_ip,fft_w);
  }

  /* Make stage 2 filter */

  if (osf == 1) {
    fs2 = sfrq/frqgcd*dfrq;
    n2 = 1;
    n2y = n2x = 1;
    f2order = (int*)malloc(sizeof(int)*n2y);
    f2order[0] = 0;
    f2inc = (int*)malloc(sizeof(int)*n2y);
    f2inc[0] = sfrq/dfrq;
    stage2 = (ssrc_real**)malloc(sizeof(ssrc_real *)*n2y);
    stage2[0] = (ssrc_real*)malloc(sizeof(ssrc_real)*n2x*n2y);
    stage2[0][0] = 1;
  } else {
    double aa = AA; /* stop band attenuation(dB) */
    double lpf,delta,d,df,alp,iza;
    double guard = 2;

    fs2 = sfrq / frqgcd * dfrq ;

    df = (fs1/2 - sfrq/2) * 2 / guard;
    lpf = sfrq/2 + (fs1/2 - sfrq/2)/guard;

    delta = pow(10.0,-aa/20);
    if (aa <= 21) d = 0.9222; else d = (aa-7.95)/14.36;

    n2 = fs2/df*d+1;
    if (n2 % 2 == 0) n2++;

    alp = alpha(aa);
    iza = dbesi0(alp);

    n2y = fs2/fs1; // 0でないサンプルがfs2で何サンプルおきにあるか？
    n2x = n2/n2y+1;

    f2order = (int*)malloc(sizeof(int)*n2y);
    for(i=0;i<n2y;i++) {
      f2order[i] = fs2/fs1-(i*(fs2/dfrq))%(fs2/fs1);
      if (f2order[i] == fs2/fs1) f2order[i] = 0;
    }

    f2inc = (int*)malloc(sizeof(int)*n2y);
    for(i=0;i<n2y;i++) {
      f2inc[i] = (fs2/dfrq-f2order[i])/(fs2/fs1)+1;
      if (f2order[i+1==n2y ? 0 : i+1] == 0) f2inc[i]--;
    }

    stage2 = (ssrc_real**)malloc(sizeof(ssrc_real *)*n2y);
    stage2[0] = (ssrc_real*)malloc(sizeof(ssrc_real)*n2x*n2y);

    for(i=1;i<n2y;i++) {
      stage2[i] = &(stage2[0][n2x*i]);
      for(j=0;j<n2x;j++) stage2[i][j] = 0;
    }

    for(i=-(n2/2);i<=n2/2;i++)
      {
	stage2[(i+n2/2)%n2y][(i+n2/2)/n2y] = win(i,n2,alp,iza)*hn_lpf(i,lpf,fs2)*fs2/fs1;
      }
  }

  /* Apply filters */

    n1b2 = n1b/2;
	inbuflen=0;
//    delay = 0;

    //    |....B....|....C....|   buf1      n1b2+n1b2
    //|.A.|....D....|             buf2  n2x+n1b2
    //
    // まずinbufからBにosf倍サンプリングしながらコピー
    // Cはクリア
    // BCにstage 1 filterをかける
    // DにBを足す
    // ADにstage 2 filterをかける
    // Dの後ろをAに移動
    // CをDにコピー

    buf1 = (ssrc_real**)malloc(sizeof(ssrc_real *)*nch);
    for(i=0;i<nch;i++)
      buf1[i] = (ssrc_real*)malloc(n1b*sizeof(ssrc_real));

    buf2 = (ssrc_real**)malloc(sizeof(ssrc_real *)*nch);
    for(i=0;i<nch;i++) {
      buf2[i] = (ssrc_real*)malloc(sizeof(ssrc_real)*(n2x+1+n1b2));
      for(j=0;j<n2x+n1b2;j++) buf2[i][j] = 0;
    }

    //rawoutbuf = (unsigned char*)malloc(dbps*nch*((double)n1b2*sfrq/dfrq+1));
    inbuf = (ssrc_real*)calloc(nch*(n1b2/osf+osf+1),sizeof(ssrc_real));
    outbuf = (ssrc_real*)malloc(sizeof(ssrc_real)*nch*((double)n1b2*sfrq/dfrq+1));

    op = outbuf;

    s2p = 0;
    rp  = 0;
    rps = 0;
    ds  = 0;
    osc = 0;
    rp2 = 0;

    delay = (double)n1/2/((double)fs1/dfrq)+(double)n2/2/((double)fs2/dfrq);

    sumread = sumwrite = 0;



  };

  ~Downsampler()
  {
	free(stage1);
	free(fft_ip);
	free(fft_w);
	free(f2order);
	free(f2inc);
	free(stage2[0]);
	free(stage2);
	for(i=0;i<nch;i++) free(buf1[i]);
	free(buf1);
	for(i=0;i<nch;i++) free(buf2[i]);
	free(buf2);
	free(inbuf);
	free(outbuf);
	//free(rawoutbuf);
  }

	unsigned int Resample(ssrc_sample * rawinbuf,unsigned int in_size,bool ending)
	{
		unsigned int rv;
		int nsmplread;
		int toberead;
		int nsmplwrt2;

		toberead = (n1b2-rps-1)/osf+1;

		if (!ending)
		{
			rv=nch*toberead;
			if (in_size<rv) return 0;
			nsmplread=toberead;
		}
		else
		{
			nsmplread=in_size/nch;
			rv=nsmplread*nch;
		}

		make_inbuf(nsmplread,inbuflen,rawinbuf,inbuf,toberead);

		sumread += nsmplread;

		rps_backup = rps;
		s2p_backup = s2p;

		for(ch=0;ch<nch;ch++)
		{
			rps = rps_backup;

			for(k=0;k<rps;k++) buf1[ch][k] = 0;

			for(i=rps,j=0;i<n1b2;i+=osf,j++)
			{
				assert(j < ((n1b2-rps-1)/osf+1));

				buf1[ch][i] = inbuf[j*nch+ch];

				for(k=i+1;k<i+osf;k++) buf1[ch][k] = 0;
			}

			assert(j == ((n1b2-rps-1)/osf+1));

			for(k=n1b2;k<n1b;k++) buf1[ch][k] = 0;

			rps = i - n1b2;
			rp += j;

			dsp_math<ssrc_real>::rdft(n1b,1,buf1[ch],fft_ip,fft_w);

			buf1[ch][0] = stage1[0]*buf1[ch][0];
			buf1[ch][1] = stage1[1]*buf1[ch][1]; 

			for(i=1;i<n1b2;i++)
			{
				ssrc_real re,im;

				re = stage1[i*2  ]*buf1[ch][i*2] - stage1[i*2+1]*buf1[ch][i*2+1];
				im = stage1[i*2+1]*buf1[ch][i*2] + stage1[i*2  ]*buf1[ch][i*2+1];

				buf1[ch][i*2  ] = re;
				buf1[ch][i*2+1] = im;
			}

			dsp_math<ssrc_real>::rdft(n1b,-1,buf1[ch],fft_ip,fft_w);

			for(i=0;i<n1b2;i++) {
				buf2[ch][n2x+1+i] += buf1[ch][i];
			}

			{
				int t1 = rp2/(fs2/fs1);
				if (rp2%(fs2/fs1) != 0) t1++;

				bp = &(buf2[ch][t1]);
			}

			s2p = s2p_backup;

			for(p=0;bp-buf2[ch]<n1b2+1;p++)
			{
				ssrc_real tmp = 0;
				ssrc_real *bp2;
				int s2o;

				bp2 = bp;
				s2o = f2order[s2p];
				bp += f2inc[s2p];
				s2p++;

				if (s2p == n2y) s2p = 0;

				assert((bp2-&(buf2[ch][0]))*(fs2/fs1)-(rp2+p*(fs2/dfrq)) == s2o);

				for(i=0;i<n2x;i++)
					tmp += stage2[s2o][i] * *bp2++;

				op[p*nch+ch] = tmp;
			}

			nsmplwrt2 = p;
		}

		rp2 += nsmplwrt2 * (fs2 / dfrq);

		if (delay >= nsmplwrt2)
		{
			delay -= nsmplwrt2;
			nsmplwrt2 = 0;
		}
		else
		{
			ssrc_real * p_outbuf = outbuf + delay * nch;
			nsmplwrt2 -= delay;
			delay = 0;

			if (ending)
			{
				int max_write = muldiv64(sumread,dfrq,sfrq)+2-sumwrite;
				if (max_write<0) max_write = 0;
				if (nsmplwrt2>max_write) nsmplwrt2=max_write;				
			}

			make_outbuf(nsmplwrt2,p_outbuf);
		}

		sumwrite += nsmplwrt2;

		{
			int ds = (rp2-1)/(fs2/fs1);

			if (ds > n1b2) ds = n1b2;

			for(ch=0;ch<nch;ch++)
				mem_ops<ssrc_real>::move(buf2[ch],buf2[ch]+ds,n2x+1+n1b2-ds);

			rp2 -= ds*(fs2/fs1);
		}

		for(ch=0;ch<nch;ch++)
			mem_ops<ssrc_real>::copy(buf2[ch]+n2x+1,buf1[ch]+n1b2,n1b2);

		return rv;
	}
};

Resampler_base::Resampler_base(const ssrc_config & c)
{
	if (c.fast)
	{
		AA = 96;
		DF = 8000;
		FFTFIRLEN = 1024;
	}
	else
	{
		AA=120;
		DF=100;
		FFTFIRLEN=16384;
	}
/*
#else
	AA=170;
	DF=100;
	FFTFIRLEN=65536;
#endif
	*/

	nch=c.nch;
	sfrq=c.sfrq;
	dfrq=c.dfrq;
	
	double noiseamp = 0.18;
	//double att=0;

}

ssrc_resampler * ssrc_resampler::Create(const ssrc_config & c)
{
	if (!CanResample(c.sfrq,c.dfrq)) return 0;

/*	if (c.math)
	{
		if (c.sfrq < c.dfrq) return new Upsampler<double>(c);
		else if (c.sfrq > c.dfrq) return new Downsampler<double>(c);
		else return 0;
	}
	else*/
	{
		if (c.sfrq < c.dfrq) return new Upsampler(c);
		else if (c.sfrq > c.dfrq) return new Downsampler(c);
		else return 0;
	}
}

void ssrc_buffer::check_size(unsigned p_size)
{
	if (p_size > m_buffer_size)
	{
		if (m_buffer_size == 0) m_buffer_size = 1;
		while(p_size > m_buffer_size) m_buffer_size<<=1;

		if (m_buffer == 0) m_buffer = mem_ops<ssrc_sample>::alloc(m_buffer_size);
		else m_buffer = mem_ops<ssrc_sample>::realloc(m_buffer,m_buffer_size);
	}
}

ssrc_buffer::ssrc_buffer()
{
	buf_data=0;
	m_buffer_size = 0;
	m_buffer = 0;
}

ssrc_buffer::~ssrc_buffer()
{
	if (m_buffer) free(m_buffer);
}
