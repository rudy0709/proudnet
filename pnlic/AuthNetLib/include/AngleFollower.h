/*
ProudNet v1


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


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

#include "BasicTypes.h"

//#pragma pack(push,8)

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif
	/** \addtogroup util_group
	*  @{
	*/

	/**
	\~korean
	특정한 각도를 향해 부드럽게 각도가 변화하도록 한다.
	각도의 부드러운 움직임을 구현할 때 사용하는 클래스이다.
	예를 들어, remote character의 방향 전환이 부드럽게 이루어지는 처리 등에 효과적이다.

	일반적인 사용법:
	- GetFollowerAngle()로, follower angle을 넣는다. (보통 초기 1회)
	- SetTargetAngle()로, target angle을 지정한다. 통상 매 framemove마다 한다.
	- FrameMove()을 매 프레임마다 호출한다.
	- 매 프레임미다 GetTargetAngle()로 follower angle을 얻는다.

	\~english
	This method helps changing an angle more smoothly.
	This class is helpful and effective in handling smooth direction change of remote character.

	General Usage:
	- Insert a follower angle with GetFollowerAngle() (Normally at one inital time)
	- Assign a target angle with SetTargetAngle(). Normally set a target at every each framemove.
	- Calll for FrameMove() at each frame.
	- Get a follower angle by GetTargetAngle() at each frame.

	\~chinese
	向特定的角度，让角度柔软地变化。
	体现角度的柔软动作时使用的class。
	例如，对remote character的方向转换柔软进行的处理等有效。

	一般使用方法：
	- 用 GetFollowerAngle()%放入follower angle。（一般初期一次）
	- 用 SetTargetAngle()%指定target angle。通常每framemove使用。
	- 每frame都呼出 FrameMove()%。
	- 每frame都用 GetTargetAngle()%获得follower angle。

	\~japanese
	\~
	 */
	class CAngleFollower
	{
		double m_followerAngle; // following angle, 즉 source.
		double m_targetAngle; // following target, 즉 source가 이 값에 근접하게 변한다.
		double m_followerAngleVelocity; // follower의 각속도
	public:
		PROUD_API CAngleFollower(void);
		PROUD_API ~CAngleFollower(void);

		/**
		\~korean
		매 프레임마다 이 메서드를 호출해야 follower가 값이 계속 변한다.
		\param elapsedTime 프레임 처리 시간(in sec)

		\~english TODO:translate needed.

		\~chinese
		每frame都要呼出此method，follower值才可以继续改变。
		\param elapsedTime frame的处理事件（in sec）

		\~japanese
		\~
		*/
		PROUD_API void FrameMove(double elapsedTime);

		/**
		\~korean
		follower angle을 세팅한다.
		\param value 각도(in radian)

		\~english TODO:translate needed.

		\~chinese
		设置follower angle。
		\param value 角度（in radian）

		\~japanese
		\~
		*/
		PROUD_API void SetFollowerAngle(double value);

		/**
		\~korean
		follower angle을 구한다.
		\return 각도(in radian)

		\~english TODO:translate needed.

		\~chinese
		求follower angle。
		\return 角度（in radian）

		\~japanese
		\~
		*/
		PROUD_API double GetFollowerAngle();

		/**
		\~korean
		target angle을 세팅한다.
		\param value 각도(in radian)

		\~english TODO:translate needed.

		\~chinese
		设置target angle。
		\param value 角度（in radian）

		\~japanese
		\~
		*/
		PROUD_API void SetTargetAngle(double value);

		/**
		\~korean
		target angle을 구한다.
		\return 각도(in radian)

		\~english TODO:translate needed.

		\~chinese
		求target angle。
		\return 角度（in radian）

		\~japanese
		\~
		*/
		PROUD_API double GetTargetAngle();

		/**
		\~korean
		follower의 이동 각속도를 넣는다.
		\param value 각속도(in radian/sec)

		\~english TODO:translate needed.

		\~chinese
		放入follower的移动角速度。
		\param value 角速度（in radian/sec）

		\~japanese
		\~
		*/
		PROUD_API void SetFollowerAngleVelocity(double value);

		/**
		\~korean
		follower의 이동 각속도를 구한다.
		\return 각속도(in radian/sec)

		\~english TODO:translate needed.

		\~chinese
		求follower的移动角速度。
		\return 角速度（in radian/sec）

		\~japanese
		\~
		*/
		PROUD_API double GetFollowerAngleVelocity();

#if defined(_MSC_VER)
		__declspec(property(get = GetFollowerAngle, put = SetFollowerAngle)) double followerAngle;
		__declspec(property(get = GetTargetAngle, put = SetTargetAngle)) double targetAngle;
		__declspec(property(get = GetFollowerAngleVelocity, put = SetFollowerAngleVelocity)) double followerAngleVelocity;
#endif
	};

	/**  @} */

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}

//#pragma pack(pop)
