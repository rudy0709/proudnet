//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (http://www.swig.org).
// Version 3.0.7
//
// Do not make changes to this file unless you know what you are doing--modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------

namespace Nettention.Proud {

public class AddrPort : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  public AddrPort(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  public static global::System.Runtime.InteropServices.HandleRef getCPtr(AddrPort obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~AddrPort() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          ProudNetServerPluginPINVOKE.delete_AddrPort(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

	public System.Net.IPAddress GetIPAddr()
	{
		return Nettention.Proud.ConvertToCSharp.ConvertNativeIPAddrToManagedIPAddr(GetNativeIPAddr());
	}

  public ushort port {
    set {
      ProudNetServerPluginPINVOKE.AddrPort_port_set(swigCPtr, value);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      ushort ret = ProudNetServerPluginPINVOKE.AddrPort_port_get(swigCPtr);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public bool IsIPv4MappedIPv6Addr() {
    bool ret = ProudNetServerPluginPINVOKE.AddrPort_IsIPv4MappedIPv6Addr(swigCPtr);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public void SetIPv4MappedIPv6Address(uint ipv4Address) {
    ProudNetServerPluginPINVOKE.AddrPort_SetIPv4MappedIPv6Address(swigCPtr, ipv4Address);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public void Synthesize(global::System.IntPtr pref, uint prefLength, uint v4BinaryAddress) {
    ProudNetServerPluginPINVOKE.AddrPort_Synthesize(swigCPtr, pref, prefLength, v4BinaryAddress);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public bool IsAddressEqualTo(AddrPort a) {
    bool ret = ProudNetServerPluginPINVOKE.AddrPort_IsAddressEqualTo(swigCPtr, AddrPort.getCPtr(a));
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public AddrPort() : this(ProudNetServerPluginPINVOKE.new_AddrPort(), true) {
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public virtual string IPToString() {
    string ret = ProudNetServerPluginPINVOKE.AddrPort_IPToString(swigCPtr);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public static AddrPort FromIPPortV4(string ipAddress, ushort port) {
    AddrPort ret = new AddrPort(ProudNetServerPluginPINVOKE.AddrPort_FromIPPortV4(ipAddress, port), true);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public static AddrPort FromIPPortV6(string ipAddress, ushort port) {
    AddrPort ret = new AddrPort(ProudNetServerPluginPINVOKE.AddrPort_FromIPPortV6(ipAddress, port), true);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public static AddrPort FromIPPort(int af, string ipAddress, ushort port) {
    AddrPort ret = new AddrPort(ProudNetServerPluginPINVOKE.AddrPort_FromIPPort(af, ipAddress, port), true);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public static AddrPort FromAnyIPPort(int af, ushort port) {
    AddrPort ret = new AddrPort(ProudNetServerPluginPINVOKE.AddrPort_FromAnyIPPort(af, port), true);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public static AddrPort From(NamedAddrPort src) {
    AddrPort ret = new AddrPort(ProudNetServerPluginPINVOKE.AddrPort_From(NamedAddrPort.getCPtr(src)), true);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public static AddrPort Unassigned {
    set {
      ProudNetServerPluginPINVOKE.AddrPort_Unassigned_set(AddrPort.getCPtr(value));
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      global::System.IntPtr cPtr = ProudNetServerPluginPINVOKE.AddrPort_Unassigned_get();
      AddrPort ret = (cPtr == global::System.IntPtr.Zero) ? null : new AddrPort(cPtr, false);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public bool IsUnicastEndpoint() {
    bool ret = ProudNetServerPluginPINVOKE.AddrPort_IsUnicastEndpoint(swigCPtr);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public bool IsAnyOrUnicastEndpoint() {
    bool ret = ProudNetServerPluginPINVOKE.AddrPort_IsAnyOrUnicastEndpoint(swigCPtr);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public static bool IsEqualAddress(AddrPort a, AddrPort b) {
    bool ret = ProudNetServerPluginPINVOKE.AddrPort_IsEqualAddress(AddrPort.getCPtr(a), AddrPort.getCPtr(b));
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public uint Hash() {
    uint ret = ProudNetServerPluginPINVOKE.AddrPort_Hash(swigCPtr);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  internal global::System.IntPtr GetNativeIPAddr() { return ProudNetServerPluginPINVOKE.AddrPort_GetNativeIPAddr(swigCPtr); }

}

}
