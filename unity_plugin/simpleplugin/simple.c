#include <stdio.h>
#include "IUnityInterface.h"
#include "IUnityGraphics.h"
#include "openuvr.h"

#define my_log(fmt, ...) fprintf(log_file, fmt, ##__VA_ARGS__)

FILE *log_file;

static IUnityInterfaces *s_UnityInterfaces = NULL;
static IUnityGraphics *s_Graphics = NULL;
static UnityGfxRenderer s_RendererType = kUnityGfxRendererNull;

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	switch (eventType)
	{
	default:
		my_log("on graphics device event called\n");
		return;
	}
}

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces *unityInterfaces)
{
	log_file = fopen("simple_log.txt", "w");
	my_log("pointer is %p\n", unityInterfaces->GetInterface);
	s_UnityInterfaces = unityInterfaces;
	s_Graphics = s_UnityInterfaces->GetInterfaceSplit(IUnityGraphics_GUID.m_GUIDHigh, IUnityGraphics_GUID.m_GUIDLow);
	s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

	OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

static void ouvr_init()
{
	openuvr_managed_init(OPENUVR_ENCODER_H264_CUDA, OPENUVR_NETWORK_UDP);
}

static void ouvr_render()
{
	openuvr_managed_copy_framebuffer();
}

static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
	switch (eventID)
	{
	case 1:
		ouvr_init();
		break;
	case 2:
		ouvr_render();
		break;
	default:
		break;
	}
}

UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
	return OnRenderEvent;
}
