#include <initguid.h>

#include <foobar2000.h>
#include <reader_helper_mem.h>

#include <7zip/archive/7z/7zHandler.h>

#include <7zip/ICoder.h>
#include <7zip/Compress/LZ/IMatchFinder.h>

#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"

static inline HRESULT ConvertBoolToHRESULT(bool result)
{
  // return result ? S_OK: E_FAIL;
  return result ? S_OK: (::GetLastError());
}

class CInFBFileStream: 
  public IInStream,
  public IStreamGetSize,
  public CMyUnknownImp
{
public:
  reader * File;
  CInFBFileStream() {}
  bool Open(reader * in);

  MY_UNKNOWN_IMP1(IStreamGetSize)

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(Seek)(INT64 offset, UINT32 seekOrigin, UINT64 *newPosition);

  STDMETHOD(GetSize)(UINT64 *size);
};

bool CInFBFileStream::Open(reader * in)
{
	File = in;
	return TRUE;
}

STDMETHODIMP CInFBFileStream::Read(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  realProcessedSize = File->read(data, size);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  return ConvertBoolToHRESULT(realProcessedSize > 0);
}

STDMETHODIMP CInFBFileStream::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
{
  return Read(data, size, processedSize);
}

STDMETHODIMP CInFBFileStream::Seek(INT64 offset, UINT32 seekOrigin, 
    UINT64 *newPosition)
{
  if(seekOrigin >= 3)
    return STG_E_INVALIDFUNCTION;
  if (!File->seek2(offset, seekOrigin)) return ::GetLastError();
  if (newPosition)
	*newPosition = File->get_position();
  return S_OK;
}

STDMETHODIMP CInFBFileStream::GetSize(UINT64 *size)
{
  if (size != NULL)
    *size = File->get_length();
  return S_OK;
}


class COutFBFileStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
private:
  char * ptr;
  UINT32 written;
  UINT32 limit;
public:
  reader * File;
  COutFBFileStream() {}
  bool Open(void * in, UINT32 size);
  
  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
};

bool COutFBFileStream::Open(void * in, UINT32 size)
{
	ptr = (char *) in;
	limit = size;
	written = 0;
	return 1;
}

STDMETHODIMP COutFBFileStream::Write(const void *data, UINT32 size, UINT32 *processedSize)
{
	UINT32 minsize = limit - written;
	if (minsize > size) minsize = size;
	memcpy(ptr, data, minsize);
	ptr += minsize;
	written += minsize;
	if (processedSize != NULL)
		*processedSize = minsize;
	return minsize == size ? S_OK : E_FAIL;
}

STDMETHODIMP COutFBFileStream::WritePart(const void *data, UINT32 size, UINT32 *processedSize)
{
	return Write(data, size, processedSize);
}

class COutFBRFileStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
private:
  const char * unpacker_path;
public:
  reader * File;
  COutFBRFileStream();
  ~COutFBRFileStream();
  bool Open(const char * path);
  
  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
};

COutFBRFileStream::COutFBRFileStream()
{
	File = NULL;
}

COutFBRFileStream::~COutFBRFileStream()
{
	if (File)
	{
		if (File->get_length()) metadb::get()->precache(unpacker_path, File);
		File->reader_release();
	}
}

bool COutFBRFileStream::Open(const char * path)
{
	File = file::g_open_tempmem();
	unpacker_path = path;
	return 1;
}

STDMETHODIMP COutFBRFileStream::Write(const void *data, UINT32 size, UINT32 *processedSize)
{
	UINT32 written = File->write(data, size);
	if (processedSize != NULL)
		*processedSize = written;
	return ConvertBoolToHRESULT(written == size);
}

STDMETHODIMP COutFBRFileStream::WritePart(const void *data, UINT32 size, UINT32 *processedSize)
{
	return Write(data, size, processedSize);
}

class CMyArchiveExtractCallback: 
  public IArchiveExtractCallback,
  // public IArchiveVolumeExtractCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)
  // COM_INTERFACE_ENTRY(IArchiveVolumeExtractCallback)

  // IProgress
  STDMETHOD(SetTotal)(UINT64 aize);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IExtractCallBack
  STDMETHOD(GetStream)(UINT32 anIndex, ISequentialOutStream **outStream, 
      INT32 askExtractMode);
  STDMETHOD(PrepareOperation)(INT32 askExtractMode);
  STDMETHOD(SetOperationResult)(INT32 resultEOperationResult);

  // IArchiveVolumeExtractCallback
  // STDMETHOD(GetInStream)(const wchar_t *name, ISequentialInStream **inStream);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

  void SetStream(ISequentialOutStream *outStream, UINT32 index);

private:
  ISequentialOutStream * m_outStream;
  UINT32 myIndex;
};

STDMETHODIMP CMyArchiveExtractCallback::GetStream(UINT32 anIndex, ISequentialOutStream **outStream, INT32 askExtractMode)
{
	if (anIndex == myIndex)
	{
		CMyComPtr<ISequentialOutStream> outStreamTemp = m_outStream;
		*outStream = outStreamTemp.Detach();
	}
	else
	{
		*outStream = NULL;
	}
	return S_OK;
}

void CMyArchiveExtractCallback::SetStream(ISequentialOutStream *outStream, UINT32 index)
{
	m_outStream = outStream;
	myIndex = index;
}

STDMETHODIMP CMyArchiveExtractCallback::SetTotal(UINT64 size)
{
  return S_OK;
}

STDMETHODIMP CMyArchiveExtractCallback::SetCompleted(const UINT64 *completeValue)
{
  return S_OK;
}

STDMETHODIMP CMyArchiveExtractCallback::PrepareOperation(INT32 askExtractMode)
{
  return S_OK;
}

STDMETHODIMP CMyArchiveExtractCallback::SetOperationResult(INT32 operationResult)
{
  return S_OK;
}

STDMETHODIMP CMyArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
  return S_OK;
}

class CMyPCArchiveExtractCallback: 
  public IArchiveExtractCallback,
  // public IArchiveVolumeExtractCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)
  // COM_INTERFACE_ENTRY(IArchiveVolumeExtractCallback)

  // IProgress
  STDMETHOD(SetTotal)(UINT64 aize);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IExtractCallBack
  STDMETHOD(GetStream)(UINT32 anIndex, ISequentialOutStream **outStream, 
      INT32 askExtractMode);
  STDMETHOD(PrepareOperation)(INT32 askExtractMode);
  STDMETHOD(SetOperationResult)(INT32 resultEOperationResult);

  // IArchiveVolumeExtractCallback
  // STDMETHOD(GetInStream)(const wchar_t *name, ISequentialInStream **inStream);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

  void Setup(ptr_list_t<char> * pathlist, const UINT32 * pindices, UINT32 count);

private:
  const UINT32 * indices;
  UINT32 index_count;
  ptr_list_t<char> * paths;
};

STDMETHODIMP CMyPCArchiveExtractCallback::GetStream(UINT32 anIndex, ISequentialOutStream **outStream, INT32 askExtractMode)
{
	const UINT32 * index = indices;
	*outStream = NULL;
	for (UINT32 i = 0; i < index_count; i++)
	{
		if (anIndex == *index++)
		{
			COutFBRFileStream * stream = new COutFBRFileStream;
			stream->Open(paths->get_item(i));
			CMyComPtr<ISequentialOutStream> outStreamTemp = stream;
			*outStream = outStreamTemp.Detach();
			break;
		}
	}
	return S_OK;
}

void CMyPCArchiveExtractCallback::Setup(ptr_list_t<char> * pathlist, const UINT32 * pindices, UINT32 count)
{
	paths = pathlist;
	indices = pindices;
	index_count = count;
}

STDMETHODIMP CMyPCArchiveExtractCallback::SetTotal(UINT64 size)
{
  return S_OK;
}

STDMETHODIMP CMyPCArchiveExtractCallback::SetCompleted(const UINT64 *completeValue)
{
  return S_OK;
}

STDMETHODIMP CMyPCArchiveExtractCallback::PrepareOperation(INT32 askExtractMode)
{
  return S_OK;
}

STDMETHODIMP CMyPCArchiveExtractCallback::SetOperationResult(INT32 operationResult)
{
  return S_OK;
}

STDMETHODIMP CMyPCArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
  return S_OK;
}

int sevenzip_exists(reader * src, const char * fn)
{
	CInFBFileStream * fileSpec = new CInFBFileStream;
	fileSpec->Open(src);
	CMyComPtr<IInStream> file(fileSpec);
	CMyComPtr<IInArchive> archive = (IInArchive *) new NArchive::N7z::CHandler;
	if (archive->Open(file, 0, 0) != S_OK)
		return 0;
	UINT32 numItems = 0;
	archive->GetNumberOfItems(&numItems);
	for (UINT32 i = 0; i < numItems; i++)
	{
		NWindows::NCOM::CPropVariant propVariant;
		archive->GetProperty(i, kpidAttributes, &propVariant);
		{
			NWindows::NCOM::CPropVariant propVariantIsFolder;
			archive->GetProperty(i, kpidAttributes, &propVariantIsFolder);
			if (propVariant.ulVal & FILE_ATTRIBUTE_DIRECTORY ||
				(propVariantIsFolder.vt == VT_BOOL && propVariantIsFolder.boolVal == VARIANT_TRUE)) continue;
		}
		archive->GetProperty(i, kpidPath, &propVariant);
		CSysString string = ConvertPropVariantToString(propVariant);
		if (!stricmp_utf8(string_utf8_from_os((LPCTSTR)string), fn)) break;
	}
	archive->Close();
	return i < numItems;
}

int sevenzip_loadfile(reader * src,const char * fn,void * &out_data,int &out_size)
{
	CInFBFileStream * fileSpec = new CInFBFileStream;
	fileSpec->Open(src);
	CMyComPtr<IInStream> file(fileSpec);
	CMyComPtr<IInArchive> archive = (IInArchive *) new NArchive::N7z::CHandler;
	if (archive->Open(file, 0, 0) != S_OK)
		return 0;
	UINT32 numItems = 0;
	archive->GetNumberOfItems(&numItems);
	for (UINT32 i = 0; i < numItems; i++)
	{
		NWindows::NCOM::CPropVariant propVariant;
		archive->GetProperty(i, kpidAttributes, &propVariant);
		{
			NWindows::NCOM::CPropVariant propVariantIsFolder;
			archive->GetProperty(i, kpidAttributes, &propVariantIsFolder);
			if (propVariant.ulVal & FILE_ATTRIBUTE_DIRECTORY ||
				(propVariantIsFolder.vt == VT_BOOL && propVariantIsFolder.boolVal == VARIANT_TRUE)) continue;
		}
		archive->GetProperty(i, kpidPath, &propVariant);
		CSysString string = ConvertPropVariantToString(propVariant);
		if (fn)
		{
			if (!stricmp_utf8(string_utf8_from_os((LPCTSTR)string), fn)) break;
		}
		else
		{
			// HACK - anti-TXT stuffs
			if (stricmp(string_extension_8(string_utf8_from_os((LPCTSTR)string)), "txt")) break;
		}
	}
	if (i < numItems)
	{
		int rv = 0;
		NWindows::NCOM::CPropVariant propVariant;
		archive->GetProperty(i, kpidSize, &propVariant);
		out_size = (int) ConvertPropVariantToUINT64(propVariant);
		out_data = malloc(out_size);
		if (out_data)
		{
			UINT32 indices[] = {i};
			COutFBFileStream * outSpec = new COutFBFileStream;
			outSpec->Open(out_data, out_size);
			CMyComPtr<ISequentialOutStream> out(outSpec);
			CMyArchiveExtractCallback * callbackSpec = new CMyArchiveExtractCallback;
			callbackSpec->SetStream(out, i);
			CMyComPtr<IArchiveExtractCallback> callback(callbackSpec);
			if (archive->Extract((const UINT32 *) &indices, 1, 0, callback) == S_OK)
			{
				rv = 1;
			}
		}
		archive->Close();
		return rv;
	}
	archive->Close();
	return 0;
}

int sevenzip_list(reader * src, const char * path, directory_callback * out)
{
	CInFBFileStream * fileSpec = new CInFBFileStream;
	fileSpec->Open(src);
	CMyComPtr<IInStream> file(fileSpec);
	CMyComPtr<IInArchive> archive = (IInArchive *) new NArchive::N7z::CHandler;
	if (archive->Open(file, 0, 0) != S_OK)
		return 0;
	UINT32 numItems = 0;
	archive->GetNumberOfItems(&numItems);
	string8 unpack_path;
	unpack_path.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);
	for (UINT32 i = 0; i < numItems; i++)
	{
		NWindows::NCOM::CPropVariant propVariant;
		archive->GetProperty(i, kpidAttributes, &propVariant);
		{
			NWindows::NCOM::CPropVariant propVariantIsFolder;
			archive->GetProperty(i, kpidAttributes, &propVariantIsFolder);
			if (propVariant.ulVal & FILE_ATTRIBUTE_DIRECTORY ||
				(propVariantIsFolder.vt == VT_BOOL && propVariantIsFolder.boolVal == VARIANT_TRUE))
				continue;
		}
		archive->GetProperty(i, kpidPath, &propVariant);
		CSysString string = ConvertPropVariantToString(propVariant);
		archive_i::make_unpack_path(unpack_path, path, string_utf8_from_os((LPCTSTR)string), "7z");
		out->on_file(unpack_path);
	}
	archive->Close();
	return 1;
}

class unpacker_7z : public unpacker
{
public:
	virtual reader *open(reader * file)
	{
//		if (!file->canSeek()) return 0;
		void * data=0;
		int size=0;

		//zip
		void * _data=0;
		int _size = 0;
		if (sevenzip_loadfile(file,0,_data,_size))
		{
			data = (char*)_data;
			size = _size;
		}

		if (!data)
		{
			file->seek(0);
			return 0;
		}
		return new reader_mem(data,size);
	}
};

class reader_7z : public reader_mem_base
{
private:
	virtual bool open(const char * path,enum reader::MODE mode)
	{
		if (mode!=reader::MODE_READ) return 0;
		string8 archive,file;
		if (!archive_i::parse_unpack_path(path,archive,file)) return 0;

		reader * r = file::g_open(archive,reader::MODE_READ);
		if (r==0) return 0;

		void * data = 0;
		int size = 0;

		bool rv = !!sevenzip_loadfile(r,file,data,size);

		r->reader_release();

		if (rv) mem_init(data,size);
		return rv;
	}
};

class archive_7z : public archive_i
{
private:
	virtual int precache(const char * path,playlist_loader_callback * callback)
	{
		metadb * p_metadb = metadb::get();

		reader * r = file::g_open(path,reader::MODE_READ);
		if (!r) return 0;

		string8 unpack_path;
		unpack_path.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);

		CInFBFileStream * fileSpec = new CInFBFileStream;
		fileSpec->Open(r);
		CMyComPtr<IInStream> file(fileSpec);
		CMyComPtr<IInArchive> archive = (IInArchive *) new NArchive::N7z::CHandler;

		int rv=0;
		if (archive->Open(file, 0, 0) == S_OK)
		{
			rv=1;
			NWindows::NCOM::CPropVariant propVariant;
			mem_block_t<UINT32> indices;
			ptr_list_t<char> paths;

			UINT32 numItems = 0;
			archive->GetNumberOfItems(&numItems);
			for (UINT32 i = 0; i < numItems; i++)
			{
				archive->GetProperty(i, kpidAttributes, &propVariant);
				{
					NWindows::NCOM::CPropVariant propVariantIsFolder;
					archive->GetProperty(i, kpidAttributes, &propVariantIsFolder);
					if (propVariant.ulVal & FILE_ATTRIBUTE_DIRECTORY ||
						(propVariantIsFolder.vt == VT_BOOL && propVariantIsFolder.boolVal == VARIANT_TRUE)) continue;
				}
				archive->GetProperty(i, kpidPath, &propVariant);
				CSysString string = ConvertPropVariantToString(propVariant);
				make_unpack_path(unpack_path,path,string_utf8_from_os((LPCTSTR)string));
				if (!callback->on_progress(unpack_path)) break;
				if (p_metadb->query_dbonly(make_playlist_entry(unpack_path,0),0)) break;
				indices.append(i);
				char * path = new char[unpack_path.length() + 1];
				strcpy(path, unpack_path);
				paths.add_item(path);
			}
			CMyPCArchiveExtractCallback * callbackSpec = new CMyPCArchiveExtractCallback;
			callbackSpec->Setup(&paths, indices.get_ptr(), indices.get_size());
			CMyComPtr<IArchiveExtractCallback> callback(callbackSpec);
			archive->Extract(indices.get_ptr(), indices.get_size(), 0, callback);
			archive->Close();

			paths.delete_all();
		}

		r->reader_release();

		return rv;
	}

	virtual int list(const char * path,directory_callback *out,playlist_loader_callback * callback)
	{
		if (stricmp(string_extension_8(path),"7z"))
			return 0;

		reader * r = file::g_open(path,reader::MODE_READ);
		if (!r) return 0;

		int rv = sevenzip_list(r,path,out);

		r->reader_release();

		return rv;
	}

	virtual const char * get_archive_type() {return "7z";}

	virtual int exists_in_archive(const char * archive,const char * file)
	{
		reader * r = file::g_open(archive,reader::MODE_READ);
		if (r==0) return 0;

		int rv = sevenzip_exists(r,file);

		r->reader_release();

		return rv ? FILE_EXISTS : 0;

	}

	virtual reader * get_reader(const char * path)
	{
		return new reader_7z;
	}

};



static service_factory_t<unpacker,unpacker_7z> foo;
static archive_factory<archive_7z> foo2;

DECLARE_COMPONENT_VERSION("7-Zip reader","1.02",0);
