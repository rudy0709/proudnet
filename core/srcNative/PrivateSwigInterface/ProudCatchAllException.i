%exception {
	try {
		$action
	} catch(std::exception& e)
	{
		SWIG_CSharpSetPendingException(SWIG_CSharpApplicationException, e.what());
		return $null;
	}
}