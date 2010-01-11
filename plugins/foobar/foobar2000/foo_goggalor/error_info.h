struct error_info_t
{
	const wchar_t * msg;
	HRESULT         hr;

	error_info_t( const wchar_t * m, HRESULT r ) : msg( m ), hr( r ) {}
};