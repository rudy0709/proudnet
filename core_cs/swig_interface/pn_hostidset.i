
%extend Proud::HostIDSet
{
	inline int GetCount()
	{
		assert(self);
		return self->GetCount();
	}
}