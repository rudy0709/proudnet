#pragma once

// Ŀ�� ������Ʈ �̸���
#ifdef WIN32
// ���� : �������� ������ �̸��� ����ȭ�� ������ �ֽ��ϴ�.
#define WIN_PIPE_NAME1			_PNT("\\\\.\\pipe\\")
#define WIN_PIPE_NAME2			_PNT("pnpipe.")
#define EVENT_FOR_WRITE			_PNT("PN_EVENT_FOR_WRITE.")
#define EVENT_FOR_READ			_PNT("PN_EVENT_FOR_READ.")
#define PN_MUTEX_NAME			_PNT("PN_HIDDEN_MUTEX")
#else
#define LINUX_PIPE_NAME			_PNT("/tmp/.systmp0503.")
#define LINUX_SEMAPHORE_NAME	_PNT("PN_HIDDEN_SEMAPHORE")
#endif