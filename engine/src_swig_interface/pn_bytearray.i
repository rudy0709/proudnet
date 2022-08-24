

%ignore Proud::ByteArray::ByteArray(const uint8_t* data, int count);
%ignore Proud::ByteArray::ByteArray(const ByteArray &src);
%ignore Proud::ByteArray::FromHexString(String text);

%extend Proud::ByteArray
{
	inline void Add(uint8_t data)
	{
		assert(self);
		self->Add(data);
	}

	inline void Clear(uint8_t data)
	{
		assert(self);
		self->Clear();
	}

	inline int GetCount()
	{
		assert(self);
		return self->GetCount();
	}

	inline uint8_t At(int index) throw (Proud::Exception)
	{
		assert(self);

		int count = self->GetCount();

		if(index < 0)
		{
			throw Proud::Exception("index is out of range");
		}

		return (*self)[index];
	}
}

//%include "../../include/ByteArray.h"

/*
namespace Proud
{

%naturalvar ByteArray;

class ByteArray;

// const ByteArray &

%typemap(ctype) const Proud::ByteArray  & "void *"
%typemap(imtype) const Proud::ByteArray  & "ByteArray"
%typemap(cstype) const Proud::ByteArray  & "ByteArray"

%typemap(csdirectorin) const Proud::ByteArray & "$iminput"
%typemap(csdirectorout) const Proud::ByteArray & "$cscall"

%typemap(csin) const Proud::ByteArray & "$csinput"

%typemap(csvarout, excode=SWIGEXCODE2) const Proud::ByteArray & %{
    get {
      ByteArray ret = $imcall;$excode
      return ret;
    } %}

}
*/