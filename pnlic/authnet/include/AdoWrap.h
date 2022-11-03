/*
ProudNet v1.x.x


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

 */

#pragma once

#if defined(_WIN32)

#include "PropNode.h"
#include "FakeClrBase.h"

#include "Variant.h"

#ifdef _WIN64
#include "./x86_64/msado15.tlh"
#else
#include "./x86_32/msado15.tlh"
#endif

//#pragma pack(push,8)

namespace Proud
{
	// UUID가 string으로 변환되면 character 수
	static const int UUIDVarcharLength = 38;

	/*
	\~korean
	#import 구문을 쓰는 대신 tlh, tli를 #include합니다.
	- 이렇게 해야 /MP 옵션이 잘 작동합니다.
	- 또한 유저가 c:\program files\common files\system\ado를 추가하는 수고를 덥니다.

	\~english
	#include tlh and tli instead of #import syntax.
	- This method makes /MP options to operate better.
	- And user doesn't have to add c:\program files\common files\system\ado.

	\~chinese
	#include tlh 和tli instead替代#import 语法。
	- 这样才能让/MP 选项更好的运转。
	- 而且用户节省增加c:\program files\common files\system\ado 的麻烦。

	\~japanese
	\~
	 */

	/** \addtogroup db_group
	*  @{
	 */

	/**
	\~korean
	\ref ado_overview 의 지연 감지체커
	- 이 객체는 사용자가 사용하지 않습니다.내부 용도로 사용됩니다.

	\~english
	This method monitors and checks delay of \ref ado_overview.
	- This object is for the internal purpose only, not usable by user.

	\~chinese
	\ref ado_overview%的延迟感知测验。
	- 用户不使用这个对象,是用于内部用途。

	\~japanese
	\~
	*/
	class CQueryChecker
	{
	protected:
		void	ExecuteBegin();
		void	ExecuteEnd(const TCHAR* command);
	protected:
		uint32_t	m_prevTime;
	};

	/**
	\~korean
	\brief DBMS 접근시 이벤트를 받는 객체의 인터페이스 클래스입니다.
	- 사용자는 이 인터페이스를 상속받아 이벤트를 노티를 받습니다.
	- \ref ado_access_tracker 를 참고하십시오.

	\~english
	\brief DBMS Interface class of object that receives event when it access.
	- User inherits this interface and receives notification of event.
	- Refer to \ref ado_access_tracker for more details.

	\~chinese
	\brief DBMS接近时接受事件对象的主界面类。
	- 玩家接到此界面通知接受event。
	- 请参考\ref ado_access_tracker%。

	\~japanese
	\~
	 */
	class IDbmsAccessEvent
	{
	public:
		virtual ~IDbmsAccessEvent();

		/**
		\~korean
		\ref ado_overview 를 사용시에 지연되는 커맨드를 발견시에 콜백됩니다.
		\param lpszcommand 사용된 커맨드 입니다. 쿼리 문구이거나 프로시져 이름입니다.
		\param curtime 감지된 당시의 시간입니다.
		\param querytick 커맨드를 사용하는데 지연되 시간입니다. (밀리초단위)

		\~english
		This method callbacks when it spots any delayed command while using \ref ado_overview.
		\param lpszcommand The list of commands in use. They are either in query or procedure name.
		\param curtime The exact time of a delayed command detected.
		\param querytick The delayted time of command being used (in milliseconds).

		\~chinese
		\ref ado_overview%使用时发现迟延的指令会自动回调。
		\param lpszcommand 是使用的命令。查询或程序的名称。
		\param curtime 感应当时时间。
		\param querytick 命令使用的时候迟延的时间。(毫秒为单位)

		\~japanese
		\~
		*/
		virtual void OnQueryDelayed(const TCHAR* lpszcommand, CPnTime curtime, uint32_t querytick) = 0;
	};

	/**
	\~korean
	\ref ado_overview 를 사용시에 이벤트를 노티하는 주체입니다.
	- 전역으로 사용됩니다.
	- \ref ado_access_tracker 를 참고하십시오.

	\~english
	The main which notifies events while using  \ref ado_overview.
	- Can be used globally.
	- Refer to \ref ado_access_tracker for more details.

	\~chinese
	\ref ado_overview%使用时event通知的主题。
	- 可以全方位使用。
	- \ref 请参考ado_access_tracker。

	\~japanese
	\~
	*/
	class CDbmsAccessTracker
	{
		friend CQueryChecker;

		CDbmsAccessTracker();
	public:
		~CDbmsAccessTracker();
		/**
		\~korean
		이벤트를 노티 받을 인터페이스를 지정합니다.
		\param pEvent 이벤트를 노티받을 인터페이스의 포인터입니다.

		\~english
		This method appoints an interface that will receive notification of event.
		\param pEvent The pointer of interface receiving notification of event.

		\~chinese
		指定将要通知事件的主界面。
		\param pEvent 是接受通知界面的关键点。

		\~japanese
		\~
		*/
		static void SetAdoWrapEvent(IDbmsAccessEvent* pEvent);

	private:
		static IDbmsAccessEvent*	m_DbmsAcessEvent;
	public:
		/**
		\~korean
		쿼리지연 이벤트 시간
		- \ref ado_overview 를 사용시에 커맨드(혹은 쿼리)가 이 시간이상이 지연되면 IDbmsAccessEvent로 이벤트를 노티합니다.
		- 기본값은 500으로 설정되어 있습니다.

		\~english
		Query delay event time
		- This method notifies event through IDbmsAccessEvent when command(query) is delayed for a specified time as using \ref ado_overview.
		- The default time is 500.

		\~chinese
		查询延迟event时间。
		- \ref ado_overview%使用时的命令（或查询）迟延的时间超过此时间，会用IDbmsAccessEvent通知event。
		- 默认值是500。

		\~japanese
		\~
		*/
		PROUD_API static uint32_t	DelayedAccessThresholdMilisec;
	};

	enum DbOpenFor { OpenForRead,OpenForFastRead,OpenForReadWrite,OpenForAppend };
	enum DbmsType  { MsSql,MySql};

	class CAdoRecordset;
	class CAdoOfflineRecord;
	class CVariant;

	/**
	\~korean
	\ref ado_overview 의 DBMS 연결 객체
	- 전역으로 이 객체를 두어서 여러 스레드에서 접근하는 것은 여러가지 문제(가령 race condition)을 야기합니다.
	따라서 필요할 때마다 이 객체를 로컬 변수로서 생성해서 쓰는 것이 바람직합니다. 한편, 성능을 위해서 연결만 하고 다른 용도로 사용되지 않는 프로세스 전역
	객체를 하나 두는 것이 좋습니다. 자세한 것은 \ref ado_connection_pooling 을 참고하십시오.
	- 이 객체를 생성하기 전에, 최소 1회의 CoInitialize를 호출해야 합니다.
	그냥 스레드 메인 함수에 다음과 같은 구문을 넣어두면 편할 것입니다.
	\code
	Proud::CCoInitializer coi;
	\endcode
	- DB를 억세스하는 동안 critical section을 건들지 않음을 체크하는 기능: ADODB_RefCriticalSection, ADODB_RecommendCriticalSectionProc

	\~english
	DMBS link object of \ref ado_overview

	- If you access other thread with this object, it will occur some problems (e.g. race condition)
	So we recommend to use this object with changing local variable when you needed. However it is better to leave process object without using to improve performance.
	Please refer to \ref ado_connection_pooling for more detail.
	- Before creating this object, at least once of Colnitialize must be called.
	It will be much convenient if the following syntax is included in the thread main function.
	\code
	Proud::CCoInitializer coi;
	\endcode
	- Function that checks whether or not critical section has been touched while accessing DB: ADODB_RefCriticalSection, ADODB_RecommendCriticalSectionProc

	\~chinese
	\ref ado_overview%的DBMS连接对象
	- 全域里用此对象从许多线程接近的会导致不少问题(如race condition)。
	因此，每当必要的时候把此对象生成为本地变数使用是最好的。另外，为了性能，建议留一个只连接不用于其他用途的程序全域对象。详细的请参考\ref ado_connection_pooling%。
	- 生成此对象之前，要最少呼叫一次CoInitialize。
	在线程主函数里添加以下内容的话会方便的。
	\code
	Proud::CCoInitializer 的COI;
	\endcode
	- 连接DB的时候给不碰critical section打勾的技能：ADODB_RefCriticalSection ， ADODB_RecommendCriticalSectionProc

	\~japanese
	\~
	 */
	class CAdoConnection:public ADODB::_ConnectionPtr,protected CQueryChecker
	{
		// Execute에서 사용하는 할당 메모리양
		const static size_t SQLTEXTLEN = 12800;

		//CAdoConnectionPool* m_connectionPool; // NULL이면 connection pool을 쓰지 않고 있음을 의미한다.
	public:
		PROUD_API CAdoConnection(ADODB::_Connection *conn);
		//CAdoConnection(CAdoConnectionPool* connectionPool);
		PROUD_API CAdoConnection();
		PROUD_API ~CAdoConnection();
	private:
		// 복사 금지. ADODB COM smartptr을 래핑하고 있으나, 정작 이 함수의 dtor에서 close를 하기 때문임.
		CAdoConnection& operator=(const CAdoConnection&);
		CAdoConnection(const CAdoConnection&);
	public:
		/**
		\~korean
		DB에 연결한다. ADODB.Connection을 확장한 기능을 갖고 있다.
		\param pszConnStr DBMS 연결 문자열
		- ADO DB connection string 규약을 따른다. 자세한 것은 <a href="http://www.connectionstrings.com/" target="_blank">http://www.connectionstrings.com/</a> 참고.
		- Driver 혹은 Provider가 들어가지 않는다면,sqloledb를 provider로 사용한다.
		아래는 한 예이다.
		\code
		Data Source=.;Database=ProudDB-Test;Trusted_Connection=yes // .을 써야지, localhost를 쓰지 말 것. 일부 컴에서 연결 실패 오류로 이어짐.
		Data Source=.;Database=ProudDB-Test;user id=xxx;password=yyy
		\endcode

		\param AppName SQL Server Enterprise Manager에서 표시되는 이름. connection의 이름에 특정 루틴을 의미하는 이름을 추가하고 싶으면 이걸 넣으면 데드락 등을 감지할 때 더 도움이 된다. 안 넣으면 무시된다.

		\~english
		This method connects to DB. It has a function that expanse ADODB.Connection.
		\param pszConnStr DBMS link string
		- It follows the rule of ADO DB connection string. Refer to <a href="http://www.connectionstrings.com/" target="_blank">http://www.connectionstrings.com/</a> for more details.
		- If Driver or Provider is not included, use sqloledb as provider.
		Below is an example.
		\code
		Data Source=.;Database=ProudDB-Test;Trusted_Connection=yes // use . instead of localhost. using localhost causes connection error in some computers.
		Data Source=.;Database=ProudDB-Test;user id=xxx;password=yyy
		\endcode

		\param AppName Name being displayed on SQL Server Enterprise Manager. This method helps detecting dead-lock when adding a name that points to a specific routine to the name of connection. If you don't insert this method, this will be ignores.

		\~chinese
		连接到DB。拥有扩展 ADODB.Connection 的功能。
		\param pszConnStr DBMS连接字符串
		- 遵守ADO DB connection string规则。详细请参考 <a href="http://www.connectionstrings.com/" target="_blank">http://www.connectionstrings.com/</a> 。
		- 没有Driver或者Provider的话，把sqloledb用成provider。
		下面就是一个例子。
		\code
		Data Source=localhost;Database=ProudDB-Test;Trusted_Connection=yes
		Data Source=localhost;Database=ProudDB-Test;user id=xxx;password=yyy
		\endcode

		\param AppName 在SQL ServeEnterprise Manager显示的名称。想在connection的名字上添加意味着特定routine的一个名字，添加这个会在检查死锁等的时候有所帮助。不添加的话会被忽略。

		\~japanese
		\~
		 */
		PROUD_API void OpenEx(const TCHAR* pszConnStr, const TCHAR* AppName);

		/**
		\~korean
		DB에 연결한다. 자세한 것은 OpenEx 도움말 참고
		\param pszConnStr DB Connection String

		\~english
		This method connects to DB. Refer to OpenEx guideline for more details.
		\param pszConnStr DB Connection String

		\~chinese
		连接到DB。详细的请参考OpenEx帮助。
		\param pszConnStr DB Connection String

		\~japanese
		\~
		*/
		inline void Open(const TCHAR* pszConnStr)
		{
			OpenEx(pszConnStr,0);
		}

		/**
		\~korean
		DB에 연결한다. ADODB.Connection을 확장한 기능을 갖고 있다.
		\param pszConnStr DBMS 연결 문자열
		- ADO DB connection string 규약을 따른다. 자세한 것은 <a href="http://www.connectionstrings.com/" target="_blank">http://www.connectionstrings.com/</a> 참고.
		아래는 한 예이다.
		\code
		Data Source=.;Database=ProudDB-Test;Trusted_Connection=yes // .을 써야지, localhost를 쓰지 말 것. 일부 컴에서 연결 실패 오류로 이어짐.
		Data Source=.;Database=ProudDB-Test;user id=xxx;password=yyy

		//MySql
		Driver={MySQL ODBC 5.1 Driver};server=xxx.xxx.xxx.xxx;port=3306;Database=TableName;User ID=UserID;Password=yyy;
		\endcode

		\param AppName SQL Server Enterprise Manager에서 표시되는 이름. connection의 이름에 특정 루틴을 의미하는 이름을 추가하고 싶으면 이걸 넣으면 데드락 등을 감지할 때 더 도움이 된다. 안 넣으면 무시된다.
		\param Type DbmsType형태의 enum값을 넣습니다.MsSql,MySql등이 있습니다.(MsSql은 내부적으로 sqloledb Provider을 사용하고 있습니다.)

		\~english TODO:translate needed.
		This method connects to DB. It has a function that expans ADODB.Connection.
		\param pszConnStr DBMS link string
		- It follows the rule of ADO DB connection string. Refer to <a href="http://www.connectionstrings.com/" target="_blank">http://www.connectionstrings.com/</a> for more details.
		Below is one example.
		\code
		Data Source=.;Database=ProudDB-Test;Trusted_Connection=yes // .을 써야지, localhost를 쓰지 말 것. 일부 컴에서 연결 실패 오류로 이어짐.
		Data Source=.;Database=ProudDB-Test;user id=xxx;password=yyy

		//MySql
		Driver={MySQL ODBC 5.1 Driver};server=xxx.xxx.xxx.xxx;port=3306;Database=TableName;User ID=UserID;Password=yyy;
		\endcode

		\param AppName Name being displayed on SQL Server Enterprise Manager. This method helps detecting dead-lock when adding a name that points to a specific routine to the name of connection. It ignores if you don't insert this method.
		\param Type This inserts enum value of DbmsType such as for Sql,MySql and etc.(MsSql internally uses sqloledb Provider.)

		\~chinese
		连接到DB。拥有扩展 ADODB.Connection 的功能。
		\param pszConnStr DBMS连接字符串
		- 遵守ADO DB connection string规则。详细请参考 <a href="http://www.connectionstrings.com/" target="_blank">http://www.connectionstrings.com/</a> 。
		下面就是一个例子。
		\code
		Data Source=localhost;Database=ProudDB-Test;Trusted_Connection=yes
		Data Source=localhost;Database=ProudDB-Test;user id=xxx;password=yyy

		//MySql
		Driver={MySQL ODBC 5.1 Driver};server=xxx.xxx.xxx.xxx;port=3306;Database=TableName;User ID=UserID;Password=yyy;
		\endcode

		\param AppName 在SQL ServeEnterprise Manager显示的名称。想在connection的名字上添加意味着特定routine的一个名字，添加这个会在检查死锁等的时候有所帮助。不添加的话会被忽略。
		\param Type 放入DbmsType形式的enum值。有.MsSql，MySql 等。（MsSql 内部使用sqloledb Provider。）

		\~japanese
		\~
		*/
		PROUD_API void OpenEx(const TCHAR* pszConnStr, const TCHAR* AppName, DbmsType Type);

		/**
		\~korean
		DB에 연결한다. 자세한 것은 OpenEx 도움말 참고
		\param pszConnStr DB Connection String
		\param type

		\~english
		This method connects to DB. Refer to OpenEx guideline for more details.
		\param pszConnStr DB Connection String

		\~chinese
		连接DB。详细的请参考OpenEx帮助。
		\param pszConnStr DB Connection String
		\param type

		\~japanese
		\~
		*/
		inline void Open(const TCHAR* pszConnStr, DbmsType Type)
		{
			OpenEx(pszConnStr,0,Type);
		}

		/**
		\~korean
		ADODB를 닫습니다.

		\~english
		ADODB will be closed.

		\~chinese
		关闭ADODB

		\~japanese
		\~
		*/
		PROUD_API void Close();

		/**
		\~korean
		ADODB를 닫습니다.
		- throw를 발생시키지 않습니다

		\~english
		ADODB will be closed.
		- throw will not occur.

		\~chinese
		关闭ADODB。
		- 不让发生throw。

		\~japanese
		\~
		*/
		PROUD_API void Close_NoThrow();

		/**
		\~korean
		ADO 객체가 DB 서버에 연결됐는지?
		\return 연결 성공하면 true를 리턴하고, 실패하면 false를 리턴합니다.

		\~english
		 Is ADO object connected to DB server?
		\return true if it succeed to connect DB;false if it failed to connect DB

		\~chinese
		ADO 对象是否与DB服务器连接？
		\return 连接成功的话返回true，失败返回false。

		\~japanese
		\~
		*/
		PROUD_API bool IsOpened();

		/**
		\~korean
		SQL 구문을 실행한다. printf()처럼 사용 가능.
		\lpszSQL 쿼리 구문
		\return stored procedure에 의해 영향받은 record의 갯수, 영향받은 행이 없거나 실패시 0, Select 구문이면 -1 리턴

		\~english
		Executes SQL syntax. Can use as printf().
		\lpszSQL SQL syntax
		\return Number of records affected by stored procedure; 0 if no rows were affected or the statement failed; and -1 for SELECT statements

		\~chinese
		实行SQL语句。跟printf()一样可以使用。
		\lpszSQL SQL 语句
		\return 被stored procedure受影响的record的个数，没有受影响的行或者失败时是0，Select 语句的话返回-1。

		\~japanese
		\~
		  */
		PROUD_API long Execute(const TCHAR* lpszSQL);

		/**
		\~korean
		SQL 구문을 실행합니다.결과는 outputRecords에 저장됩니다.
		\param outRecordset 받아올 결과 레코드셋.
		\lpszSQL 쿼리 구문
		\return stored procedure에 의해 영향받은 record의 갯수, 영향받은 행이 없거나 실패시 0, Select 구문이면 -1 리턴

		\~english
		Executes SQL syntax. The result gets stored at outputRecords.
		\param outRecordset Recordset of results to get.
		\lpszSQL SQL syntax
		\return Number of records affected by stored procedure; 0 if no rows were affected or the statement failed; and -1 for SELECT statements

		\~chinese
		实行SQL语句。结果储存于outputRecords。
		\param outRecordset 要得到Recordset结果
		\lpszSQL SQL 语句
		\return 被stored procedure受影响的record的个数，没有受影响的行或者失败时是0，Select 语句的话返回-1。

		\~japanese
		\~
		 */
		PROUD_API long Execute(CAdoRecordset& outputRecords, const TCHAR* lpszSQL);

		/**
		\~korean
		DB 트랜잭션을 시작합니다.

		\~english
		DB transaction begins

		\~chinese
		DB transaction 的开始

		\~japanese
		\~
		  */
		inline void BeginTrans() {  (*this)->BeginTrans(); }

		/**
		\~korean
		DB 트랜잭션 롤백

		\~english
		DB transaction rollback

		\~chinese
		DB transaction 的回滚

		\~japanese
		\~
		  */
		inline void RollbackTrans() {  (*this)->RollbackTrans(); }

		/**
		\~korean
		DB 트랜잭션 커밋

		\~english
		 DB transaction commit

		\~chinese
		DB transaction 的提交

		\~japanese
		\~
		  */
		inline void CommitTrans() {  (*this)->CommitTrans(); }
	private:
	};

	extern CriticalSection* ADODB_RefCriticalSection;

	typedef void(*ADODB_RecommendCriticalSectionProc_t)( const TCHAR* text );
	extern ADODB_RecommendCriticalSectionProc_t ADODB_RecommendCriticalSectionProc;

	/**
	\~korean
	ADO Recordset 클래스 + 몇가지 기능 향상

	용도
	- CAdoConnection 과 혼용한다.

	\~english
	 ADO Recordset class + some enhanced functions

	Use
	- Use this method with CAdoConnection.

	\~chinese
	ADO Recordset 类+几种功能改进

	用途
	- 与 CAdoConnection%混用。

	\~japanese
	\~
	  */
	class CAdoRecordset:public ADODB::_RecordsetPtr,protected CQueryChecker
	{
	public:
		PROUD_API CAdoRecordset();

		inline CAdoRecordset(ADODB::_Recordset *rc)
		{
			if(rc)
				Attach(rc,true);		// AddRef, not Attach
		}

		inline ~CAdoRecordset()
		{
			Close_NoThrow(); // 이게 있는 이유는 ~CAdoConnection()에 적혀있음. 반드시 있어야 함.
		}

		inline CAdoRecordset& operator=(ADODB::_Recordset *rc)
		{
			if(rc)
				Attach(rc,true);

			return *this;
		}
	private:
		// 복사 금지. ADODB COM smartptr을 래핑하고 있으나, 정작 이 함수의 dtor에서 close를 하기 때문임.
		CAdoRecordset& operator=(const CAdoRecordset&);
		CAdoRecordset(const CAdoRecordset&);
	public:

		// various ways of accessing bind data

/*		inline void GetFV(bool& value,LPCWSTR pszFieldName)
		{ value=GetInterfacePtr()->GetCollect(pszFieldName); }

		inline void SetFV(LPCWSTR pszFieldName,bool value)
		{ GetInterfacePtr()->PutCollect(pszFieldName,value); }

		inline void GetFV(long& value,LPCWSTR pszFieldName)
		{ value=GetInterfacePtr()->GetCollect(pszFieldName); }

		inline void SetFV(LPCWSTR pszFieldName,long value)
		{ GetInterfacePtr()->PutCollect(pszFieldName,value); }

		inline void GetFV(float& value,LPCWSTR pszFieldName)
		{ value=GetInterfacePtr()->GetCollect(pszFieldName); }

		inline void SetFV(LPCWSTR pszFieldName,float value)
		{ GetInterfacePtr()->PutCollect(pszFieldName,value); }

		inline void GetFV(DATE& value,LPCWSTR pszFieldName)
		{ value=GetInterfacePtr()->GetCollect(pszFieldName); }

		inline void SetFV(LPCWSTR pszFieldName,DATE value)
		{ GetInterfacePtr()->PutCollect(pszFieldName,value); }

		inline void GetFV(String &value,LPCWSTR pszFieldName)
		{ value=(LPCWSTR)(_bstr_t)GetInterfacePtr()->GetCollect(pszFieldName); }

		inline void SetFV(LPCWSTR pszFieldName,LPCWSTR value)
		{ GetInterfacePtr()->PutCollect(pszFieldName,value); } */

		PROUD_API bool GetFieldValue(const TCHAR* pszFieldName, String &var);
		PROUD_API bool GetFieldValue(const TCHAR* pszFieldName, UUID &var);
		PROUD_API bool GetFieldValue(const TCHAR* pszFieldName, CPnTime &var);

		PROUD_API CVariant GetFieldValue(const TCHAR* pszFieldName);
		PROUD_API void SetFieldValue(const TCHAR* pszFieldName, const CVariant &value);

		PROUD_API CVariant GetFieldValue(int index);
		PROUD_API void SetFieldValue(int index,const CVariant &value);

		/**
		\~korean
		필드의 값을 읽고 쓴다.
		- 파라메터로 CVariant를 쓰므로 어떠한 타입의 데이터든지 읽고 쓸 수 있다.
		- 배열 인자로 필드 문자열이나 0 based 인덱스 숫자를 쓸 수 있다.

		\~english
		Reads and writes field value.
		- It can read and write just about any type of data since it use CVariant as parameter.
		- It can use field string or 0 based index number as array factor.

		\~chinese
		读写领域的值。
		- 用参数写 CVariant%，因此能读写任何类型的数据。
		- 用排列因子写对领域字符串或者0 base索引数据。

		\~japanese
		\~
		*/
#if defined (_MSC_VER)
		__declspec(property(get=GetFieldValue,put=SetFieldValue)) CVariant FieldValues[];
#endif

		PROUD_API _bstr_t GetFieldNames(int index);

		/**
		\~korean
		필드의 이름을 얻는다.
		- 배열 인자로 0 based 인덱스 숫자를 쓸 수 있다.

		\~english
		This method obtain name of field.
		- It can use 0 based index number as an array factor.

		\~chinese
		获得领域的名称。
		- 用排列因子写入0 based索引数字。

		\~japanese
		\~
		*/
#if defined (_MSC_VER)
		__declspec(property(get=GetFieldNames)) _bstr_t FieldNames[];
#endif
		/**
		\~korean
		ADODB를 닫습니다.

		\~english
		ADODB will be closed.

		\~chinese
		关闭ADODB。

		\~japanese
		\~
		*/
		PROUD_API void Close();

		/**
		\~korean
		ADODB를 닫습니다.
		- throw를 발생시키지 않습니다

		\~english
		ADODB will be closed.
		- throw will not occur.

		\~chinese
		关闭ADODB。
		- 不让发生throw。

		\~japanese
		\~
		*/
		PROUD_API void Close_NoThrow();

		/**
		\~korean
		레코드의 끝인지 확인합니다.
		\return 끝이면 true 아니면 false

		\~english TODO:translate needed.

		\~chinese
		确认是否是record的最后。
		\return 结束的话true，不是的话false。

		\~japanese
		\~
		*/
		inline bool IsEOF()
		{
			return GetInterfacePtr()->adoEOF?true:false;
		}

		/**
		\~korean
		ADODB가 open상태인지 확인합니다.
		\return open 상태이면 true, close 상태이면 false

		\~english TODO:translate needed.

		\~chinese
		确认ADODB是否是open状态。
		\return open状态的话true，close 状态的话false。

		\~japanese
		\~
		*/
		PROUD_API bool IsOpened();

		/**
		\~korean
		DbmsSave함수를 통하여 저장된 데이터를 업데이트 시킵니다.

		\~english TODO:translate needed.

		\~chinese
		通过DbmsSave函数，更新存储的数据。

		\~japanese
		\~
		*/
		PROUD_API void Update();

		/**
		\~korean
		새로운 데이터를 추가할 것을 예약합니다.

		\~english TODO:translate needed.

		\~chinese
		预约添加新的数据。

		\~japanese
		\~
		*/
		PROUD_API void AddNew();

		/**
		\~korean
		데이터를 제거합니다.

		\~english TODO:translate needed.

		\~chinese
		删除数据。

		\~japanese
		\~
		*/
		PROUD_API void Delete();

		/**
		\~korean
		Recordset을 연다.
		\param conn DB 연결 객체
		\param openFor OpenForRead: 읽기 전용, OpenForReadWrite, OpenForAppend: 읽기/쓰기. 게임 서버상에서 용도가 최적화된 값이다.
		\param lpszSQL 쿼리 구문.

		\~english
		This method opens Recordset.
		\param conn DB Link Object
		\param openFor OpenForRead: Read Only, OpenForReadWrite, OpenForAppend: Read/Write. The most optimized values for game server use.
		\param lpszSQL Query Syntax.

		\~chinese
		打开Recordset。
		\param conn DB连接对象
		\param openFor OpenForRead:专用于读，OpenForReadWrite, OpenForAppend:读/写，游戏服务器中用途被优化的值。
		\param lpszSQL SQL语句。

		\~japanese
		\~
		*/
		PROUD_API void Open(ADODB::_Connection *conn, DbOpenFor openFor, const TCHAR* lpszSQL);

		/**
		\~korean
		Recordset을 연다.
		\param conn DB 연결 객체
		\param cursorType ADODB::CursorTypeEnum
		\param lockType ADODB::LockTypeEnum
		\param lpszSQL 쿼리 구문.

		\~english
		This method opens Recordset.
		\param conn DB Link Object
		\param lpszSQL Query Syntax.

		\~chinese
		打开Recordset。
		\param conn DB连接对象
		\param cursorType ADODB::CursorTypeEnum
		\param lockType ADODB::LockTypeEnum
		\param lpszSQL SQL语句。

		\~japanese
		\~
		*/
		PROUD_API void Open(ADODB::_Connection *conn, ADODB::CursorTypeEnum cursorType, ADODB::LockTypeEnum lockType, const TCHAR* lpszSQL);

		/**
		\~korean
		모든 parameter를 default value로 하여 연다.
		ADO command에서 return한 recordset object를 open할 때 사용될 수 있다.

		\~english
		This method opens all parameters as default value.
		This can be used when opening recordset object returned by ADO command.

		\~chinese
		把所有的parameter当做default value打开。
		可以在打开ADO commandreturn的recordset object的时候可以使用。

		\~japanese
		\~
		*/
		PROUD_API void Open(void);

		/**
		\~korean
		ADODB::adOpenStatic 타입의 Cursor 로 recordset 을 연다.
		ADO command에서 return한 recordset object를 open할 때 사용될 수 있다.

		\~english
		This method opens all parameters as default value.
		This can be used when opening recordset object returned by ADO command.

		\~chinese
		用ADODB::adOpenStatic 类型的cursor打开recordset。
		可以在打开ADO commandreturn的recordset object的时候使用。

		\~japanese
		\~
		*/
		PROUD_API void OpenForUpdate();

		/**
		\~korean
		\param src \ref CAdoOfflineRecord에 있는 key값과 value값을 차례로 복사해옵니다.

		\~english TODO:translate needed.

		\~chinese
		\param src 依次复制\ref CAdoOfflineRecord%的key值和value值。

		\~japanese
		\~
		*/
		PROUD_API void CopyFrom(CAdoOfflineRecord &src);
		const static int defaultCacheSize=100;

		/**
		\~korean
		다음 레코드로 커서를 옮긴다.

		\~english
		This method moves cursor to the next record.

		\~chinese
		把光标移动到下一个record上。

		\~japanese
		\~
		 */
		PROUD_API void MoveNext();

		/**
		\~korean
		처음 레코드로 커서를 옮긴다.

		\~english
		This method moves cursor to the initial record

		\~chinese
		把光标移动到初始record上。

		\~japanese
		\~
		 */
		PROUD_API void MoveFirst();

		/**
		\~korean
		이전 레코드로 커서를 옮긴다.

		\~english
		This method moves cursor to the previous record

		\~chinese
		把光标移动到上一个record上。

		\~japanese
		\~
		 */
		PROUD_API void MovePrevious();

		/**
		\~korean
		 마지막 레코드로 커서를 옮긴다.

		\~english
		This method moves cursor to the last record.

		\~chinese
		把光标移动到最后一个record上。

		\~japanese
		\~
		 */
		PROUD_API void MoveLast();

		/**
		\~korean
		다음 레코드셋으로 이동합니다.
		- 다음 레코드셋을 자기자신에게 할당합니다.
		\param recordsAffected 쿼리에 영향을 받은 레코드 갯수

		\~english
		This method moves to the next recordset.
		- Assign the next recordset to itself.
		\param recordsAffected Number of records being affected by query

		\~chinese
		移动到下一个recordset。
		- 把下一个recordset分配给自己。
		\param recordsAffected 受SQL影响的record个数

		\~japanese
		\~
		 */
		PROUD_API bool MoveNextRecordset(long *recordsAffected = NULL);
		/**
		\~korean
		다음 레코드셋을 다른 인스턴스에게 넘겨줍니다.
		\param outRecordset 다음 레코드셋을 넘겨받을 레코드셋 인스턴스
		\param recordsAffected 쿼리에 영향을 받은 레코드 갯수

		\~english
		This method passes the next recordset to a different instance.
		\param outRecordset Recordset instance which will receive the next recordset
		\param recordsAffected Number of records being affected by query

		\~chinese
		把下一个recordset给其他instance
		\param outRecordset 接到下一个recordset的recordset instance
		\param recordsAffected 受query影响的recordset个数

		\~japanese
		\~
		 */
		PROUD_API void NextRecordset(OUT CAdoRecordset& outRecordset,OUT long *recordsAffected=NULL);
	};

	/**
	\~korean
	ADO Command object를 wrapping한 클래스이다.

	이것으로 stored procedure를 호출하는 방법은 다음과 같다.
	- Prepare로 호출할 stored procedure, active connection을 설정한다.
	- Parameters 멤버로 입력 아규먼트를 설정한다.
	- Execute로 준비된 stored procedure를 호출한다.
	- Parameters 멤버로 출력 아규먼트를 구한다.

	예제
	\code
	CAdoConnection db;
	db.Open(L"Data Source=.;Database=ProudDB-Test;Trusted_Connection=yes");
	CAdoCommand co;
	co.Prepare(db,L"pn_sum");
	co.Parameters[1]=1L;
	co.Parameters[2]=2L;
	co.Execute();
	long x=co.Parameters[3];
	\endcode

	\~english
	This is a class wrapping ADO Command object.

	How to call stored procedure with this method is shown in below.
	- Set stored procedure and active connection to call as Prepare.
	- Set input arguments with Parameters member.
	- Call already prepared stored procedure with Execute.
	- Get output argument with Parameters member.

	Example
	\code
	CAdoConnection db;
	db.Open(L"Data Source=.;Database=ProudDB-Test;Trusted_Connection=yes");
	CAdoCommand co;
	co.Prepare(db,L"pn_sum");
	co.Parameters[1]=1L;
	co.Parameters[2]=2L;
	co.Execute();
	long x=co.Parameters[3];
	\endcode

	\~chinese
	把ADO Command object wrapping的类。
	用找个呼叫stored procedure的方法如下。
	- 设置用prepare呼出的stored procedure, active connection。
	- 用parameters成员设置输入arguments。
	- 呼出 呼叫用execute准备的stored procedure。
	- 用parameter成员输出arguments。

	例
	\code
	CAdoConnection db;
	db.Open(L"Data Source=.;Database=ProudDB-Test;Trusted_Connection=yes");
	CAdoCommand co;
	co.Prepare(db,L"pn_sum");
	co.Parameters[1]=1L;
	co.Parameters[2]=2L;
	co.Execute();
	long x=co.Parameters[3];
	\endcode

	\~japanese
	\~
	*/
	class CAdoCommand:public ADODB::_CommandPtr,protected CQueryChecker
	{
	public:
		PROUD_API CAdoCommand(ADODB::_Command *conn);
		PROUD_API CAdoCommand();
	private:
		// 복사 금지. ADODB COM smartptr을 래핑하고 있으나, 정작 이 함수의 dtor에서 close를 하기 때문임.
		CAdoCommand& operator=(const CAdoCommand&);
		CAdoCommand(const CAdoCommand&);
	public:

		/**
		\~korean
		stored procedure를 호출할 준비를 한다. 즉, ADO Connection, stored procedure name, inout argument 준비를 한다.
		\param connection
		\param storedProcName

		\~english
		This method sets ready to call stored procedure. Meaning, it prepares ADO Connection, stored procedure name and inout argument.
		\param connection
		\param storedProcName

		\~chinese
		准备呼叫stored procedure。即准备ADO Connection, stored procedure name, inout argument。
		\param connection
		\param storedProcName

		\~japanese
		\~
		*/
		PROUD_API void /*FASTCALL */ Prepare(ADODB::_Connection* connection, const TCHAR* storedProcName, ADODB::CommandTypeEnum cmdType = ADODB::adCmdStoredProc);

		/**
		\~korean
		PrepareSP에 지정되었던 stored procedure를 실행한다.
		\param recordsAffected stored procedure에 의해 영향받은 record의 갯수

		\~english
		Execute the appointed stored procedure at PrepareSP.
		\param recordsAffected Number of records affected by stored procedure

		\~chinese
		执行被PrepareSP指定的stored procedure。
		\param recordsAffected 被stored procedure影响的record的个数。

		\~japanese
		\~
		  */
		PROUD_API void /*FASTCALL */ Execute(OUT long *recordsAffected=NULL);

		/**
		\~korean
		PrepareSP에 지정되었던 stored procedure를 실행한다.
		\param outRecordset 받아올 결과 레코드셋.
		\param recordsAffected stored procedure에 의해 영향받은 record의 갯수

		\~english
		Execute the appointed stored procedure at PrepareSP.
		\param outRecordset Recordset of results to get.
		\param recordsAffected Number of records affected by stored procedure

		\~chinese
		执行被PrepareSP指定的stored procedure。
		\param outRecordset 接收结果的recordset
		\param recordsAffected 被stored procedure影响的record个数

		\~japanese
		\~
		  */
		PROUD_API void /*FASTCALL */ Execute(OUT CAdoRecordset& outRecordset,OUT long *recordsAffected=NULL);


		/**
		\~korean
		index가 가리키는 번째의 paramter 값을 구한다. PrepareSP를 호출한 후에 사용할 수 있다.
		\return 리턴되는 parameter 값.
		\param index 몇번째 parameter의 값을 구할 것인가?
		0은 stored procedure의 return value를, 나머지 parameter는 1-based index이다.

		\~english
		Get parameter value of nth pointed by index. This can be used after PrepareSP is called.
		\return Returned parameter value.
		\param Which nth of parameter value do you want to get?
		0 is return value of stored procedure and the res of parameters are 1-based index.

		\~chinese
		index 所指的第n个paramter值。呼出PrepareSP后可以使用。
		\return 返回的parameter值
		\param index 需要求第几个parameter的值？
		0是stored procedure的return value，剩下的parameter是1-based index。

		\~japanese
		\~
		*/
		PROUD_API CVariant /*FASTCALL */ GetParam(int index);

		/**
		\~korean
		index가 가리키는 번째의 parameter 값을 설정한다. PrepareSP를 호출한 후에 사용할 수 있다.
		\param index 몇번째 parameter의 값을 설정할 것인가?
		0은 stored procedure의 return value를, 나머지 parameter는 1-based index이다.
		\param value 설정할 paramter 값

		\~english
		This method sets value of nth parameter pointed by index. It can be used after PrepareSP is called.
		\param index Which nth parameter value do you want to set?
		0 is return value of stored procedure and the res of parameters are 1-based index.
		\param value Setting paramter value

		\~chinese
		设置index所指的第n个paramter值。呼出 叫PrepareSP后可以使用。
		\param index 要设置第几个parameter的值？
		0是stored procedure的return value，剩下的parameter是1-based index。
		\param value 要设置的parameter值

		\~japanese
		\~

		**NOTE** This function is very slow because of executing an additional stored procedure for getting procedure information.
		It is not recommended to use this function. The workaround is to use AppendParameter() instead.
		*/
		PROUD_API void /*FASTCALL */ SetParam(int index,const CVariant &value);
#if defined (_MSC_VER)
		__declspec(property(get=GetParam,put=SetParam)) CVariant Parameters[];
#endif
		/**
		\~korean
		parameter 값을 붙인다.MsSQL에서는 별로 사용될 일이 없지만, MySQL에서는 SetParam대신 이 메소드를 사용해야 한다.
		\return 만들어진 _ParameterPtr
		\param paramName 호출할 프로시져의 parameter의 이름
		\param paramType 데이터의 타입 ADODB::DataTypeEnum 참조.
		\param paramDirection 데이터의 들어갈 형태 ADODB::ParameterDirectionEnum 참조.(MySQL은 adParamInput만 쓰여야함.output 지원 안함)
		\param defaultValue 실제 프로시져에 전달될 값

		\~english
		Add parameter value. This is not commonly used in MsSQL, but this method must be used in MySQL instead of SetParam.
		\return Created _ParameterPtr
		\param paramName Name of parameter procedure to call for
		\param paramType Refer to Data Type ADODB::DataTypeEnum
		\param paramDirection Refer to Data insertion direction ADODB::ParameterDirectionEnum.(MySQL must use adParamInput only. Output is not supported.)
		\param defaultValue Value that will be passed to the actual procedure

		\~chinese
		粘贴parameter值。虽然在MsSQL不常使用，在MySQL要使用此方法来代替SetParam。
		\return 创建的_ParameterPtr
		\param paramName 要呼叫的程序parameter的名称
		\param paramType 参考数据类型ADODB::DataTypeEnum
		\param paramDirection 数据进入的形式参考ADODB::ParameterDirectionEnum。（MySQL 只能写入adParamInput。不支持output）
		\param defaultValue 实际传达到程序的值。

		\~japanese
		\~
		*/
		PROUD_API ADODB::_ParameterPtr /*FASTCALL */ AppendParameter(const TCHAR* paramName, ADODB::DataTypeEnum paramType, ADODB::ParameterDirectionEnum paramDirection, const CVariant &defaultValue);

		/**
		\~korean
		동명 메소드 참조
		\param defaultValue 실제 프로시져에 스트링 값

		\~english
		Refer to the same method
		\param defaultValue String value of the actual procedure

		\~chinese
		参考同名方法
		\param defaultValue 程序的实际字符串值

		\~japanese
		\~
		*/
		PROUD_API ADODB::_ParameterPtr /*FASTCALL */ AppendParameter(const TCHAR* paramName, ADODB::DataTypeEnum paramType, ADODB::ParameterDirectionEnum paramDirection, const TCHAR* defaultValue);
		PROUD_API ADODB::_ParameterPtr /*FASTCALL */ AppendParameter(ADODB::DataTypeEnum paramType, ADODB::ParameterDirectionEnum paramDirection, const TCHAR* defaultValue);

		/**
		\~korean
		동명 메소드 참조 다만, 내부적으로 _variant_t가 들어감

		\~english
		Refer to the same method but internally _variant_t will be added

		\~chinese
		参考同名方法。但内部进入_variant_t。

		\~japanese
		\~
		*/
		PROUD_API ADODB::_ParameterPtr /*FASTCALL */ AppendParameter(const TCHAR* paramName, ADODB::DataTypeEnum paramType, ADODB::ParameterDirectionEnum paramDirection);

		/**
		\~korean
		동명 메소드 참조
		\param defaultValue 실제 프로시져에 스트링 값
		\param length 스트링의 최대 길이

		\~english
		Refer to the same method
		\param defaultValue String value of the actual procedure
		\param length The maximum length of string

		\~chinese
		参考同名方法
		\param defaultValue 程序的实际字符串值
		\param length 字符串的最大长度

		\~japanese
		\~
		*/
		PROUD_API ADODB::_ParameterPtr /*FASTCALL */ AppendParameter(const TCHAR *paramName, ADODB::DataTypeEnum paramType, ADODB::ParameterDirectionEnum paramDirection, const TCHAR* defaultValue, long length);

		/**
		Convenient function for AppendParameter.
		*/
		ADODB::_ParameterPtr AppendInputParameter(const TCHAR *paramName, const Guid& value);

		ADODB::_ParameterPtr AppendReturnValue();

	private:
		void WarnIfParameterHasProblem();
	};

	/**
	\~korean
	ADO Record의 field 내용에서 복사된 사본 객체
	ADO record을 close하고 나서도 그 내용을 사용할 때 유용하다.
	ADO recordset의 field의 사본을 카피하되, 원본 recordset을 close해도 읽기 억세스 정도는
	할 수 있는 ADO object가 존재하면, 그것을 이 클래스 대신으로 써도 되겠지만, 성능이 보장된
	그러한 것이 없으므로 이 클래스를 대신 만들었다.
	MSSQL server의 특성상, field name은 case insensitive이다.

	\~english
	ven after closing ADO Record, its copy ADO Record is useful
	when using the field contetns contained within.
	If there is ADO object containing copied contents of recordset
	that can be read even after its source recordset is closed,
	you may use that method but since its performance is not guaranteed,
	the below class has been made.
	Due to the characterics of MSSQL server, field name must be case insensitive.

	\~chinese
	在ADO Record的field内容里复制的副本对象
	关闭ADO Record以后还可以有用的使用那个内容。
	把 虽然ADO Record的副本复印，但是关闭原本recordset也可读ADDobject存在程序的话，
	能代替使用此类，没有保障性能的此类东西，因此制作了这个类。
	MSSQL server 的特性上，field name 是case insensitive。

	\~japanese
	\~
	  */
	class CAdoOfflineRecord:public CProperty
	{
	public:
		/**
		\~korean
		원본 recordset가 가리키는 cursor에서 1개의 record를 복사한다.
		원본 recordset의 cursor에는 영향을 끼치지 않는다.
		\param src  원본 recordset

		\~english
		Copy one record of the source recordset pointed by cursor.
		This does not cursor of the source recordset.
		\param src  Source recordset

		\~chinese
		原本recordset所指的cursor上复制一个record。
		不给原本recordset的cursor施加影响。
		\param src 原本recordset

		\~japanese
		\~
		*/
		PROUD_API void CopyFrom(CAdoRecordset &src);

		inline CAdoOfflineRecord()
		{
		}

		inline CAdoOfflineRecord& operator=(CAdoRecordset& source)
		{
			CopyFrom(source);
			return *this;
		}
		inline CAdoOfflineRecord(CAdoRecordset& source)
		{
			CopyFrom(source);
		}
	};

	/**
	\~korean
	CAdoRecordset은 DBMS와의 연결을 유지한 채로 레코드를 접근하는데, 이와 달리 CAdoOfflineRecordset은
	DBMS와의 연결이 없는 채로 레코드를 접근하게 해줍니다.
	이를 위해 CAdoRecordset으로부터 IterateAndCopyFrom 또는 IterateAndAppendFrom를 통해 레코드 내용의 사본을
	이 객체로 저장할 수 있습니다.

	이 객체를 활용하면, 예를 들어, DBMS와의 연결 유지시간을 최소화하는 DBMS 접근법을 가능하게 해줍니다.

	이 객체는 CAdoOfflineRecord의 배열 객체입니다.

	주의: ToVariant(), ToLongLong() 설명 참고. Windows 2000 이하 관련 문제이다.

	\~english
	CAdoRecordset accesses record as maintaining the connection with DBMS
	while CAdoOfflineRecordset can do so without being connected DBMS.
	In order for this, you can save the copy of record through or
	IterateAndAppendFrom from CAdoRecordset as an object.
	This object can help minimizing the connection duration time of DBMS as accessing DBMS.

	This object is an array object of CAdoOfflineRecord.

	CAUTION: Refer to ToVariant(), ToLongLong(). This issue could arise in OS below than Windows 2000.

	\~chinese
	CAdoRecordset 是维持与DBMS的连接的情况下接近record，与此不同的 CAdoOfflineRecordset%是没有与DBMS的连接的情况下接近record。
	为了这个从 CAdoRecordset%到IterateAndCopyFrom或者通过IterateAndAppendFrom把record内容的复件存储到此对象。
	利用此对象的话，例如，最小化与DBMS的连接位置时间的DBMS接近法变为可能。
	此对象是 CAdoOfflineRecord%的排列对象。
	注意：参考ToVariant(), ToLongLong()说明。是Windows 2000以下相关问题。

	\~japanese
	\~
	*/
	class CAdoOfflineRecordset:public CFastArray<CAdoOfflineRecord>
	{
	public:

		/**
		\~korean
		source의 모든 레코드를 복사해옵니다.
		복사가 끝나면 source의 cursor는 EOF가 됩니다.
		\param source  원본 recordset

		\~english
		This method copies all records of source.
		When copying is done, cursor of source becomes EOF.
		\param source  Source recordset

		\~chinese
		复制source的所有record。
		复制结束的话source的cursor会变成EOF。
		\param source 原本recordset

		\~japanese
		\~
		*/
		PROUD_API void IterateAndCopyFrom(CAdoRecordset &source);

		/**
		\~korean
		source의 모든 레코드를 이 객체의 기존 레코드들의 마지막에 추가합니다.
		복사가 끝나면 source의 cursor는 EOF가 됩니다.
		\param source  원본 recordset

		\~english
		Add all records of source to the last line of existing records within this object.
		When copying is done, cursor of source becomes EOF.
		\param source  Source recordset

		\~chinese
		把source的所有record添加到此对象的原有record的最后。
		复制结束的话source的cursor会变成EOF。

		\~japanese
		\~
		*/
		PROUD_API void IterateAndAppendFrom(CAdoRecordset &source);
	};

	// Disconnected Recordset - Sample Source //
	// 연결 해제된 레코드셋 - 샘플 소스 //
	/*
	try
	{
	ADODB::_RecordsetPtr spRS(__uuidof(ADODB::Recordset));
	ADODB::_ConnectionPtr spConn(__uuidof(ADODB::Connection));

	// Connect and get recordset.
	spConn->Open(OLESTR("dsn=SQLServer"),OLESTR("sa"),
	OLESTR(""), -1);

	spRS->CursorLocation = ADODB::adUseClient;
	spRS->Open(OLESTR("select * from Table1"),
	spConn.GetInterfacePtr(),
	ADODB::adOpenForwardOnly, ADODB::adLockBatchOptimistic, -1);

	// Disassociate the connection from the recordset.
	spRS->PutRefActiveConnection(NULL);

	// Change the value.
	spRS->Fields->Item[0L]->Value = OLESTR("NewValue");

	// Re-connect.
	spRS->PutRefActiveConnection(spConn);

	// Send updates.
	spRS->UpdateBatch(ADODB::adAffectAll);

	spRS->Close();
	spConn->Close();

	}
	catch( _com_error e)
	{
	// Do Exception handling.
	}
	*/

	struct ExecuteSQLAsyncParam
	{
		String connectionString,stmt;
	};

	/**
	\~korean
	src문자열 내의 "'"문자는 오작동을 유발할 수 있기 때문에 "''"로 바꾸어 줍니다.

	\~english TODO:translate needed.

	\~chinese
	Src 字符串内的"'"文字可能会导致故障，改为"''"。

	\~japanese
	\~
	*/
	PROUD_API String gds(const TCHAR *src);

	PROUD_API void RecommendNoCriticalSectionLock();

	/**
	\~korean
	이 객체를 로컬 변수로 갖고 있는 상태의 스레드는 DB를 억세스하는 중에 critical section이 lock되어 있다 하더라도 경고 표시를
	무시하게 한다. 불가피하지만 빈도가 매우 낮은 DB 병목을 무시하기 위해서 이 클래스를 쓸 수 있다.

	주의:
	- 이걸 남용하지 말것. DB 병목을 무시하는것이니만큼 서버 최적화 과정에서 이것 때문에 정작 중요한 병목을 못 찾을 수 있다.
	- 로컬 변수로만 이 객체를 생성할 수 있다.

	\~english
	Thread holding this object as a loval variable ignores warning notice even though critical section is locked as accessing DB.
	This class can be used to ignore DB bottleneck that is inevitable but low in frequency of use.

	CAUTION:
	- DO NOT USE THIS TOO MUCH. Setting to ignore DB bottleneck can cause serious issues such as skipping important DB bottleneck in the process of server optimization.
	- This object can be created only by local variable.

	\~chinese
	把此对象拥有为本地变数状态的线程中连接DB的时候尽管critical section被lock，也会无视警告标志。虽然不可避免，为了无视频率很低的DB瓶颈，可以使用此类。
	注意：
	- 不用滥用此项。因为是无视DB瓶颈，在服务器优化过程中可能因为这个找不到重要的瓶颈。
	- 使用本地变数就可以生成此对象。

	\~japanese
	\~
	   */
	class CAdoDBTemporaryBottleneckCheckDisabler
	{
	public:
		PROUD_API CAdoDBTemporaryBottleneckCheckDisabler();
		PROUD_API ~CAdoDBTemporaryBottleneckCheckDisabler();
	};

	/**
	\~korean
	ProudDB 실행중 발생되는 에러 객체
	- 문제 발생 시 ILogWriterDelegate::OnLogWriterException함수를 통하여 CallBack받으실 수 있습니다.
	- AdoException::what()함수를 통하여 발생된 내용의 String을 얻으실 수 있습니다.

	\~english TODO:translate needed.
	Error object that could occur as running ProudDB
	-
	-

	\~chinese
	ProudDB 实行时发生的错误对象
	- 问题发生时通过ILogWriterDelegate::OnLogWriterException 函数得到CallBack。
	- 通过 AdoException::what()%函数可以得到发生内容的String。

	\~japanese
	\~
	  */
	class AdoException:public Exception
	{
	public:
		/**
		\~korean
		ProudDB는 ADO를 쓰고 있다. ProudDB가 쓰는 ADO는 _com_error를 예외로 던지는데
		그것이 여기에 담긴다.

		\~english
		ProudDB uses ADO. ADO of ProudDB throws _com_error as exception
		which gets stored in here.

		\~chinese
		ProudDB 用ADO。 ProudDB 使用的ADO例外地抛_com_error，那个会装进这里。

		\~japanese
		\~
		  */
		_com_error m_comError;

		PROUD_API AdoException(const TCHAR *txt, _com_error &e);
	};





	/**  @}  */


}

//#pragma pack(pop)

#endif //_WIN32
