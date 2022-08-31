// ??? ????? ???? C# code ?????? ????? ???? ??????? private?? ???????.
%csmethodmodifiers Proud::HostIDArray::add "private"
%csmethodmodifiers Proud::HostIDArray::at "private"

// ??????? C# code?? ?????? ??????.
%typemap(cscode) Proud::HostIDArray
%{
	public void Add(Nettention.Proud.HostID hostID)
	{
		add((Nettention.Proud.HostID)hostID);
	}

	public Nettention.Proud.HostID At(int index)
	{
		return (Nettention.Proud.HostID)at(index);;
	}
%}

// ??????? C++ code?? ???? ?????? ??????.
%extend Proud::HostIDArray
{
	inline void add(Proud::HostID data)
	{
		assert(self);
		self->Add(data);
	}

	inline void Clear(Proud::HostID data)
	{
		assert(self);
		self->Clear();
	}

	inline int GetCount()
	{
		assert(self);
		return self->GetCount();
	}

	inline Proud::HostID at(int index) throw (Proud::Exception)
	{
		assert(self);

		int count = self->GetCount();

		if(index < 0 || count <= index)
		{
			throw Proud::Exception("number is out of range");
		}

		return (*self)[index];
	}
}
