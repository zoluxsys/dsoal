/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2001 TransGaming Technologies, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Linux does not support better timing than 10ms */
#define DS_TIME_RES 2  /* Resolution of multimedia timer */
#define DS_TIME_DEL 10  /* Delay of multimedia timer callback, and duration of HEL fragment */
/* Default refresh count, can be overridden */
#define FAKE_REFRESH_COUNT (1000/DS_TIME_DEL/2)


#ifdef __WINESRC__

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#else

#include <al.h>
#include <alc.h>

#include <stdio.h>

extern int LogLevel;

#if DEBUG_INFO
#define TRACE(fmt,args...) do {                                         \
    if(LogLevel >= 3)                                                   \
        fprintf(stderr, "trace:dsound:%s " fmt, __FUNCTION__, ##args);  \
} while(0)
#define WARN(fmt,args...) do {                                          \
    if(LogLevel >= 2)                                                   \
        fprintf(stderr, "warn:dsound:%s " fmt, __FUNCTION__, ##args);   \
} while(0)
#define FIXME(fmt,args...) do {                                         \
    if(LogLevel >= 1)                                                   \
        fprintf(stderr, "fixme:dsound:%s " fmt, __FUNCTION__, ##args);  \
} while(0)
#define ERR(fmt,args...) do {                                           \
    if(LogLevel >= 0)                                                   \
        fprintf(stderr, "err:dsound:%s " fmt, __FUNCTION__, ##args);    \
} while(0)
#else
#define TRACE(args...)
#define WARN(args...)
#define FIXME(args...)
#define ERR(args...)
#endif

const char *wine_dbg_sprintf( const char *format, ... );
const char *wine_dbgstr_wn( const WCHAR *str, int n );

static inline const char *debugstr_guid( const GUID *id )
{
    if (!id) return "(null)";
    if (!((ULONG_PTR)id >> 16)) return wine_dbg_sprintf( "<guid-0x%04hx>", (WORD)(ULONG_PTR)id );
    return wine_dbg_sprintf( "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                             id->Data1, id->Data2, id->Data3,
                             id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                             id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
}

static inline const char *debugstr_w( const WCHAR *s ) { return wine_dbgstr_wn( s, -1 ); }

#endif


#include "alext.h"
#include "eax.h"

/* Set to 1 to build a DLL that can be used in an app that uses OpenAL itself.
 * Thread-local contexts are needed for true concurrency, however. Disallowing
 * concurrency can avoid a save and restore of the context. */
#define ALLOW_CONCURRENT_AL 0

/* All openal functions */
extern int openal_loaded;
#ifdef SONAME_LIBOPENAL
extern LPALCCREATECONTEXT palcCreateContext;
extern LPALCMAKECONTEXTCURRENT palcMakeContextCurrent;
extern LPALCPROCESSCONTEXT palcProcessContext;
extern LPALCSUSPENDCONTEXT palcSuspendContext;
extern LPALCDESTROYCONTEXT palcDestroyContext;
extern LPALCGETCURRENTCONTEXT palcGetCurrentContext;
extern LPALCGETCONTEXTSDEVICE palcGetContextsDevice;
extern LPALCOPENDEVICE palcOpenDevice;
extern LPALCCLOSEDEVICE palcCloseDevice;
extern LPALCGETERROR palcGetError;
extern LPALCISEXTENSIONPRESENT palcIsExtensionPresent;
extern LPALCGETPROCADDRESS palcGetProcAddress;
extern LPALCGETENUMVALUE palcGetEnumValue;
extern LPALCGETSTRING palcGetString;
extern LPALCGETINTEGERV palcGetIntegerv;
extern LPALCCAPTUREOPENDEVICE palcCaptureOpenDevice;
extern LPALCCAPTURECLOSEDEVICE palcCaptureCloseDevice;
extern LPALCCAPTURESTART palcCaptureStart;
extern LPALCCAPTURESTOP palcCaptureStop;
extern LPALCCAPTURESAMPLES palcCaptureSamples;
extern LPALENABLE palEnable;
extern LPALDISABLE palDisable;
extern LPALISENABLED palIsEnabled;
extern LPALGETSTRING palGetString;
extern LPALGETBOOLEANV palGetBooleanv;
extern LPALGETINTEGERV palGetIntegerv;
extern LPALGETFLOATV palGetFloatv;
extern LPALGETDOUBLEV palGetDoublev;
extern LPALGETBOOLEAN palGetBoolean;
extern LPALGETINTEGER palGetInteger;
extern LPALGETFLOAT palGetFloat;
extern LPALGETDOUBLE palGetDouble;
extern LPALGETERROR palGetError;
extern LPALISEXTENSIONPRESENT palIsExtensionPresent;
extern LPALGETPROCADDRESS palGetProcAddress;
extern LPALGETENUMVALUE palGetEnumValue;
extern LPALLISTENERF palListenerf;
extern LPALLISTENER3F palListener3f;
extern LPALLISTENERFV palListenerfv;
extern LPALLISTENERI palListeneri;
extern LPALLISTENER3I palListener3i;
extern LPALLISTENERIV palListeneriv;
extern LPALGETLISTENERF palGetListenerf;
extern LPALGETLISTENER3F palGetListener3f;
extern LPALGETLISTENERFV palGetListenerfv;
extern LPALGETLISTENERI palGetListeneri;
extern LPALGETLISTENER3I palGetListener3i;
extern LPALGETLISTENERIV palGetListeneriv;
extern LPALGENSOURCES palGenSources;
extern LPALDELETESOURCES palDeleteSources;
extern LPALISSOURCE palIsSource;
extern LPALSOURCEF palSourcef;
extern LPALSOURCE3F palSource3f;
extern LPALSOURCEFV palSourcefv;
extern LPALSOURCEI palSourcei;
extern LPALSOURCE3I palSource3i;
extern LPALSOURCEIV palSourceiv;
extern LPALGETSOURCEF palGetSourcef;
extern LPALGETSOURCE3F palGetSource3f;
extern LPALGETSOURCEFV palGetSourcefv;
extern LPALGETSOURCEI palGetSourcei;
extern LPALGETSOURCE3I palGetSource3i;
extern LPALGETSOURCEIV palGetSourceiv;
extern LPALSOURCEPLAYV palSourcePlayv;
extern LPALSOURCESTOPV palSourceStopv;
extern LPALSOURCEREWINDV palSourceRewindv;
extern LPALSOURCEPAUSEV palSourcePausev;
extern LPALSOURCEPLAY palSourcePlay;
extern LPALSOURCESTOP palSourceStop;
extern LPALSOURCEREWIND palSourceRewind;
extern LPALSOURCEPAUSE palSourcePause;
extern LPALSOURCEQUEUEBUFFERS palSourceQueueBuffers;
extern LPALSOURCEUNQUEUEBUFFERS palSourceUnqueueBuffers;
extern LPALGENBUFFERS palGenBuffers;
extern LPALDELETEBUFFERS palDeleteBuffers;
extern LPALISBUFFER palIsBuffer;
extern LPALBUFFERF palBufferf;
extern LPALBUFFER3F palBuffer3f;
extern LPALBUFFERFV palBufferfv;
extern LPALBUFFERI palBufferi;
extern LPALBUFFER3I palBuffer3i;
extern LPALBUFFERIV palBufferiv;
extern LPALGETBUFFERF palGetBufferf;
extern LPALGETBUFFER3F palGetBuffer3f;
extern LPALGETBUFFERFV palGetBufferfv;
extern LPALGETBUFFERI palGetBufferi;
extern LPALGETBUFFER3I palGetBuffer3i;
extern LPALGETBUFFERIV palGetBufferiv;
extern LPALBUFFERDATA palBufferData;
extern LPALDOPPLERFACTOR palDopplerFactor;
extern LPALDOPPLERVELOCITY palDopplerVelocity;
extern LPALDISTANCEMODEL palDistanceModel;
extern LPALSPEEDOFSOUND palSpeedOfSound;

#define alcCreateContext palcCreateContext
#define alcMakeContextCurrent palcMakeContextCurrent
#define alcProcessContext palcProcessContext
#define alcSuspendContext palcSuspendContext
#define alcDestroyContext palcDestroyContext
#define alcGetCurrentContext palcGetCurrentContext
#define alcGetContextsDevice palcGetContextsDevice
#define alcOpenDevice palcOpenDevice
#define alcCloseDevice palcCloseDevice
#define alcGetError palcGetError
#define alcIsExtensionPresent palcIsExtensionPresent
#define alcGetProcAddress palcGetProcAddress
#define alcGetEnumValue palcGetEnumValue
#define alcGetString palcGetString
#define alcGetIntegerv palcGetIntegerv
#define alcCaptureOpenDevice palcCaptureOpenDevice
#define alcCaptureCloseDevice palcCaptureCloseDevice
#define alcCaptureStart palcCaptureStart
#define alcCaptureStop palcCaptureStop
#define alcCaptureSamples palcCaptureSamples
#define alEnable palEnable
#define alDisable palDisable
#define alIsEnabled palIsEnabled
#define alGetString palGetString
#define alGetBooleanv palGetBooleanv
#define alGetIntegerv palGetIntegerv
#define alGetFloatv palGetFloatv
#define alGetDoublev palGetDoublev
#define alGetBoolean palGetBoolean
#define alGetInteger palGetInteger
#define alGetFloat palGetFloat
#define alGetDouble palGetDouble
#define alGetError palGetError
#define alIsExtensionPresent palIsExtensionPresent
#define alGetProcAddress palGetProcAddress
#define alGetEnumValue palGetEnumValue
#define alListenerf palListenerf
#define alListener3f palListener3f
#define alListenerfv palListenerfv
#define alListeneri palListeneri
#define alListener3i palListener3i
#define alListeneriv palListeneriv
#define alGetListenerf palGetListenerf
#define alGetListener3f palGetListener3f
#define alGetListenerfv palGetListenerfv
#define alGetListeneri palGetListeneri
#define alGetListener3i palGetListener3i
#define alGetListeneriv palGetListeneriv
#define alGenSources palGenSources
#define alDeleteSources palDeleteSources
#define alIsSource palIsSource
#define alSourcef palSourcef
#define alSource3f palSource3f
#define alSourcefv palSourcefv
#define alSourcei palSourcei
#define alSource3i palSource3i
#define alSourceiv palSourceiv
#define alGetSourcef palGetSourcef
#define alGetSource3f palGetSource3f
#define alGetSourcefv palGetSourcefv
#define alGetSourcei palGetSourcei
#define alGetSource3i palGetSource3i
#define alGetSourceiv palGetSourceiv
#define alSourcePlayv palSourcePlayv
#define alSourceStopv palSourceStopv
#define alSourceRewindv palSourceRewindv
#define alSourcePausev palSourcePausev
#define alSourcePlay palSourcePlay
#define alSourceStop palSourceStop
#define alSourceRewind palSourceRewind
#define alSourcePause palSourcePause
#define alSourceQueueBuffers palSourceQueueBuffers
#define alSourceUnqueueBuffers palSourceUnqueueBuffers
#define alGenBuffers palGenBuffers
#define alDeleteBuffers palDeleteBuffers
#define alIsBuffer palIsBuffer
#define alBufferf palBufferf
#define alBuffer3f palBuffer3f
#define alBufferfv palBufferfv
#define alBufferi palBufferi
#define alBuffer3i palBuffer3i
#define alBufferiv palBufferiv
#define alGetBufferf palGetBufferf
#define alGetBuffer3f palGetBuffer3f
#define alGetBufferfv palGetBufferfv
#define alGetBufferi palGetBufferi
#define alGetBuffer3i palGetBuffer3i
#define alGetBufferiv palGetBufferiv
#define alBufferData palBufferData
#define alDopplerFactor palDopplerFactor
#define alDopplerVelocity palDopplerVelocity
#define alDistanceModel palDistanceModel
#define alSpeedOfSound palSpeedOfSound
#endif

#include <math.h>
#include "wingdi.h"
#include "mmreg.h"

/* OpenAL only allows for 1 single access to the device at the same time */
extern CRITICAL_SECTION openal_crst;

extern const ALCchar *DSOUND_getdevicestrings(void);
extern const ALCchar *DSOUND_getcapturedevicestrings(void);

extern LPALCMAKECONTEXTCURRENT set_context;
extern LPALCGETCURRENTCONTEXT get_context;
extern BOOL local_contexts;

/* Device implementation */
typedef struct DS8Primary DS8Primary;
typedef struct DS8Buffer DS8Buffer;

typedef struct DS8Impl
{
    IDirectSound8 IDirectSound8_iface;

    LONG ref;
    BOOL is_8;

    LONG *deviceref;
    ALCdevice *device;
    DS8Primary *primary;

    DWORD speaker_config;
    DWORD prio_level;
    GUID guid;
} DS8Impl;

/* Sample types */
#define AL_BYTE                                  0x1400
#define AL_UNSIGNED_BYTE                         0x1401
#define AL_SHORT                                 0x1402
#define AL_UNSIGNED_SHORT                        0x1403
#define AL_INT                                   0x1404
#define AL_UNSIGNED_INT                          0x1405
#define AL_FLOAT                                 0x1406
#define AL_DOUBLE                                0x1407
#define AL_BYTE3                                 0x1408
#define AL_UNSIGNED_BYTE3                        0x1409
#define AL_MULAW                                 0x1410
#define AL_IMA4                                  0x1411

/* Channel configurations */
#define AL_MONO                                  0x1500
#define AL_STEREO                                0x1501
#define AL_REAR                                  0x1502
#define AL_QUAD                                  0x1503
#define AL_5POINT1                               0x1504 /* (WFX order) */
#define AL_6POINT1                               0x1505 /* (WFX order) */
#define AL_7POINT1                               0x1506 /* (WFX order) */

enum {
    EXT_EFX,
    EXT_FLOAT32,
    EXT_MCFORMATS,
    EXT_STATIC_BUFFER,
    SOFT_BUFFER_SAMPLES,
    SOFT_BUFFER_SUB_DATA,
    SOFT_DEFERRED_UPDATES,

    MAX_EXTENSIONS
};

typedef struct ExtALFuncs
{
    PFNALBUFFERSUBDATASOFTPROC BufferSubData;
    PFNALBUFFERDATASTATICPROC BufferDataStatic;

    LPALGENEFFECTS GenEffects;
    LPALDELETEEFFECTS DeleteEffects;
    LPALEFFECTI Effecti;
    LPALEFFECTF Effectf;

    LPALGENAUXILIARYEFFECTSLOTS GenAuxiliaryEffectSlots;
    LPALDELETEAUXILIARYEFFECTSLOTS DeleteAuxiliaryEffectSlots;
    LPALAUXILIARYEFFECTSLOTI AuxiliaryEffectSloti;

    void (AL_APIENTRY*BufferSamplesSOFT)(ALuint,ALuint,ALenum,ALsizei,ALenum,ALenum,const ALvoid*);
    void (AL_APIENTRY*BufferSubSamplesSOFT)(ALuint,ALsizei,ALsizei,ALenum,ALenum,const ALvoid*);
    void (AL_APIENTRY*GetBufferSamplesSOFT)(ALuint,ALsizei,ALsizei,ALenum,ALenum,ALvoid*);
    ALboolean (AL_APIENTRY*IsBufferFormatSupportedSOFT)(ALenum);

    void (AL_APIENTRY*DeferUpdates)(void);
    void (AL_APIENTRY*ProcessUpdates)(void);
} ExtALFuncs;

struct DS8Primary
{
    IDirectSoundBuffer IDirectSoundBuffer_iface;
    IDirectSound3DListener IDirectSound3DListener_iface;
    IKsPropertySet IKsPropertySet_iface;

    LONG ref, ds3d_ref, prop_ref;
    IDirectSoundBuffer8 *write_emu;
    DS8Impl *parent;

    CRITICAL_SECTION crst;

    DWORD buf_size;
    BOOL stopped;
    DWORD flags;
    WAVEFORMATEXTENSIBLE format;
    ALCcontext *ctx;

    ALboolean SupportedExt[MAX_EXTENSIONS];

    ALuint *sources;
    DWORD nsources, sizesources;
    DWORD max_sources;
    DS8Buffer **buffers;
    DWORD nbuffers, sizebuffers;
    DS8Buffer **notifies;
    DWORD nnotifies, sizenotifies;
    UINT timer_id;
    DWORD timer_res;

    ALuint auxslot;
    ALuint effect;
    EAXLISTENERPROPERTIES eax_prop;

    ExtALFuncs ExtAL;
    union {
        struct {
            BOOL pos : 1;
            BOOL vel : 1;
            BOOL orientation : 1;
            BOOL distancefactor : 1;
            BOOL rollofffactor : 1;
            BOOL dopplerfactor : 1;
            BOOL effect : 1;
        } bit;
        int flags;
    } dirty;
    ALfloat rollofffactor;
    DS3DLISTENER listen;
};

typedef struct DS8Data DS8Data;

struct DS8Buffer
{
    IDirectSoundBuffer8 IDirectSoundBuffer8_iface;
    IDirectSound3DBuffer IDirectSound3DBuffer_iface;
    IDirectSoundNotify IDirectSoundNotify_iface;
    IKsPropertySet IKsPropertySet_iface;

    LONG ref, ds3d_ref, not_ref, prop_ref;
    LONG all_ref;

    DWORD ds3dmode;
    DS8Primary *primary;
    DS8Data *buffer;
    ALuint source;
    ALuint curidx;
    BOOL isplaying, islooping, bufferlost;
    ALCcontext *ctx;
    ExtALFuncs *ExtAL;
    CRITICAL_SECTION *crst;

    DS3DBUFFER ds3dbuffer;
    union {
        struct {
            BOOL pos : 1;
            BOOL vel : 1;
            BOOL cone_angles : 1;
            BOOL cone_orient : 1;
            BOOL cone_outsidevolume : 1;
            BOOL min_distance : 1;
            BOOL max_distance : 1;
            BOOL mode : 1;
        } bit;
        int flags;
    } dirty;

    DWORD nnotify, lastpos;
    DSBPOSITIONNOTIFY *notify;
};

extern HRESULT DS8Primary_Create(DS8Primary **prim, DS8Impl *parent);
extern void DS8Primary_Destroy(DS8Primary *prim);

extern HRESULT DS8Buffer_Create(DS8Buffer **ppv, DS8Primary *parent, DS8Buffer *orig);
extern void DS8Buffer_Destroy(DS8Buffer *buf);

static inline ALdouble gain_to_mB(ALdouble gain)
{
    return log10(gain) * 2000.0;
}
static inline ALdouble mB_to_gain(ALdouble millibels)
{
    return pow(10.0, millibels/2000.0);
}

static inline LONG clampI(LONG val, LONG minval, LONG maxval)
{
    if(val >= maxval) return maxval;
    if(val <= minval) return minval;
    return val;
}
static inline ULONG clampU(ULONG val, ULONG minval, ULONG maxval)
{
    if(val >= maxval) return maxval;
    if(val <= minval) return minval;
    return val;
}
static inline FLOAT clampF(FLOAT val, FLOAT minval, FLOAT maxval)
{
    if(val >= maxval) return maxval;
    if(val <= minval) return minval;
    return val;
}


#define getALError() \
do { \
    ALenum err = alGetError(); \
    if(err != AL_NO_ERROR) \
    { \
        ERR(">>>>>>>>>>>> Received AL error %#x on context %p, %s:%u\n", err, get_context(), __FUNCTION__, __LINE__); \
    } \
} while (0)

#define getALCError(dev) \
do { \
    ALenum err = alcGetError(dev); \
    if(err != ALC_NO_ERROR) \
    { \
        ERR(">>>>>>>>>>>> Received ALC error %#x on device %p, %s:%u\n", err, dev, __FUNCTION__, __LINE__); \
    } \
} while(0)

#if ALLOW_CONCURRENT_AL

#define setALContext(actx) \
    do { \
        ALCcontext *__old_ctx, *cur_ctx = actx; \
        if (!local_contexts) EnterCriticalSection(&openal_crst); \
        __old_ctx = get_context(); \
        if (__old_ctx != cur_ctx && set_context(cur_ctx) == ALC_FALSE) {\
            ERR("Couldn't set current context!!\n"); \
            getALCError(alcGetContextsDevice(cur_ctx)); \
        }

/* Only restore a NULL context if using global contexts, for TLS contexts always restore */
#define popALContext() \
        if (__old_ctx != cur_ctx && \
            (local_contexts || __old_ctx) && \
            set_context(__old_ctx) == ALC_FALSE) { \
            ERR("Couldn't restore old context!!\n"); \
            getALCError(alcGetContextsDevice(__old_ctx)); \
        } \
        if (!local_contexts) LeaveCriticalSection(&openal_crst); \
    } while (0)

#else

#define setALContext(actx) \
    do { \
        ALCcontext *cur_ctx = actx; \
        if (!local_contexts) EnterCriticalSection(&openal_crst); \
        if (set_context(cur_ctx) == ALC_FALSE) { \
            ERR("Couldn't set current context!!\n"); \
            getALCError(alcGetContextsDevice(cur_ctx)); \
        }

#define popALContext() \
        if (!local_contexts) LeaveCriticalSection(&openal_crst); \
    } while (0)

#endif

HRESULT DSOUND_Create(REFIID riid, void **ppDS);
HRESULT DSOUND_Create8(REFIID riid, void **ppDS);
HRESULT DSOUND_FullDuplexCreate(REFIID riid, LPDIRECTSOUNDFULLDUPLEX* ppDSFD);
HRESULT IKsPrivatePropertySetImpl_Create(REFIID riid, void **piks);
HRESULT DSOUND_CaptureCreate(REFIID riid, LPDIRECTSOUNDCAPTURE *ppDSC);
HRESULT DSOUND_CaptureCreate8(REFIID riid, LPDIRECTSOUNDCAPTURE8 *ppDSC8);

extern const GUID DSOUND_renderer_guid;
extern const GUID DSOUND_capture_guid;