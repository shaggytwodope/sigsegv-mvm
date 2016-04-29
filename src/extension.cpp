#include "extension.h"
#include "library.h"
#include "link/link.h"
#include "mod.h"
#include "addr/addr.h"
#include "addr/prescan.h"
#include "gameconf.h"
#include "prop.h"
#include "util/rtti.h"
#include "disasm/disasm.h"
#include "factory.h"
#include "concolor.h"
#include "re/nextbot.h"
#include "version.h"


CExtSigsegv g_Ext;
SMEXT_LINK(&g_Ext);

IServerGameClients *serverGameClients = nullptr;
ICvar *icvar                          = nullptr;
ISpatialPartition *partition          = nullptr;
IEngineTrace *enginetrace             = nullptr;
IStaticPropMgrServer *staticpropmgr   = nullptr;
IGameEventManager2 *gameeventmanager  = nullptr;
IEngineSound *enginesound             = nullptr;
IVModelInfo *modelinfo                = nullptr;
IVDebugOverlay *debugoverlay          = nullptr;

IPhysics *physics                = nullptr;
IPhysicsCollision *physcollision = nullptr;

ISoundEmitterSystemBase *soundemitterbase = nullptr;

IMaterialSystem *g_pMaterialSystem = nullptr;

CGlobalVars *gpGlobals         = nullptr;
CBaseEntityList *g_pEntityList = nullptr;

IVEngineClient *engineclient = nullptr;
IBaseClientDLL *clientdll    = nullptr;

SourcePawn::ISourcePawnEngine *g_pSourcePawn = nullptr;
SourceMod::IExtensionManager *smexts         = nullptr;

//ISDKTools *g_pSDKTools = nullptr;

IEngineTool *enginetools  = nullptr;
IServerTools *servertools = nullptr;
IClientTools *clienttools = nullptr;


#if 0
CON_COMMAND_F(sig_unload, "Unload this extension", FCVAR_NONE)
{
	if (smexts != nullptr) {
		smexts->UnloadExtension(myself);
	}
}
#endif


bool CExtSigsegv::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
	g_pSourcePawn = g_pSM->GetScriptingEngine();
	SM_FIND_IFACE_OR_FAIL(EXTENSIONMANAGER, smexts, error, maxlen);
	
	ColorSpew::Enable();
	
//	sharesys->AddDependency(myself, "sdktools.ext", true, true);
//	SM_GET_IFACE(SDKTOOLS, g_pSDKTools);
	
	if (gameeventmanager != nullptr) {
		gameeventmanager->LoadEventsFromFile("resource/sigsegv_events.res");
	}
	
	this->LoadSoundOverrides();
	
	g_pEntityList = reinterpret_cast<CBaseEntityList *>(gamehelpers->GetGlobalEntityList());
	
	PreScan::DoScans();
	if (!g_GCHook.LoadAll(error, maxlen)) goto fail;
	
	LibMgr::Load();
//	g_Disasm.Load();
	
	RTTI::PreLoad();
	AddrManager::Load();
	
	if (!Link::InitAll()) goto fail;
	
//	CDetourManager::Init(g_pSourcePawn);
	
	Prop::PreloadAll();
	
	g_ModManager.Load();
	
	IGameSystem::Add(this);
	
	for (int i = 0; i < 255; ++i) {
		ConColorMsg(Color(0xff, i, 0x00), "%02x%02x%02x\n", 0xff, i, 0x00);
	}
	
	return true;
	
fail:
	g_GCHook.UnloadAll();
	return false;
}

void CExtSigsegv::SDK_OnUnload()
{
	IGameSystem::Remove(this);
	
	IHotplugAction::UnloadAll();
	
	g_ModManager.Unload();
	
	LibMgr::Unload();
	
	g_GCHook.UnloadAll();
	
	ColorSpew::Disable();
}

void CExtSigsegv::SDK_OnAllLoaded()
{
}

bool CExtSigsegv::QueryRunning(char *error, size_t maxlen)
{
//	SM_CHECK_IFACE(SDKTOOLS, g_pSDKTools);
	
	return true;
}


bool CExtSigsegv::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	DevMsg("CExtSigsegv: compiled @ %s %s\n", GetBuildDate(), GetBuildTime());
	
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetServerFactory, gamedll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_CURRENT(GetServerFactory, serverGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	g_pCVar = icvar;
	ConVar_Register(0, this);
	
	GET_V_IFACE_CURRENT(GetEngineFactory, partition, ISpatialPartition, INTERFACEVERSION_SPATIALPARTITION);
	GET_V_IFACE_CURRENT(GetEngineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, staticpropmgr, IStaticPropMgrServer, INTERFACEVERSION_STATICPROPMGR_SERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, gameeventmanager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_CURRENT(GetEngineFactory, enginesound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, modelinfo, IVModelInfo, VMODELINFO_SERVER_INTERFACE_VERSION);
	
	GET_V_IFACE_CURRENT(GetServerFactory, servertools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
	
	/* optional stuff */
	debugoverlay = (IVDebugOverlay *)ismm->VInterfaceMatch(ismm->GetEngineFactory(), VDEBUG_OVERLAY_INTERFACE_VERSION, 0);
	enginetools = (IEngineTool *)ismm->VInterfaceMatch(ismm->GetEngineFactory(), VENGINETOOL_INTERFACE_VERSION, 0);
	
	GET_V_IFACE_CURRENT(GetPhysicsFactory, physics, IPhysics, VPHYSICS_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetPhysicsFactory, physcollision, IPhysicsCollision, VPHYSICS_COLLISION_INTERFACE_VERSION);
	
	if (GetSoundEmitterSystemFactory() != nullptr) {
		soundemitterbase = (ISoundEmitterSystemBase *)ismm->VInterfaceMatch(GetSoundEmitterSystemFactory(), SOUNDEMITTERSYSTEM_INTERFACE_VERSION, 0);
	}
	
	if (GetMaterialSystemFactory() != nullptr) {
		g_pMaterialSystem = (IMaterialSystem *)ismm->VInterfaceMatch(GetMaterialSystemFactory(), MATERIAL_SYSTEM_INTERFACE_VERSION, 0);
	}
	
	if (GetClientFactory() != nullptr) {
		GET_V_IFACE_CURRENT(GetEngineFactory, engineclient, IVEngineClient, VENGINE_CLIENT_INTERFACE_VERSION);
		clientdll = (IBaseClientDLL *)ismm->VInterfaceMatch(GetClientFactory(), CLIENT_DLL_INTERFACE_VERSION, 0);
		clienttools = (IClientTools *)ismm->VInterfaceMatch(GetClientFactory(), VCLIENTTOOLS_INTERFACE_VERSION, 0);
	}
	
	gpGlobals = ismm->GetCGlobals();
	
	LibMgr::SetPtr(Library::SERVER, (void *)ismm->GetServerFactory(false));
	LibMgr::SetPtr(Library::ENGINE, (void *)ismm->GetEngineFactory(false));
	LibMgr::SetPtr(Library::TIER0,  (void *)&MemAllocScratch);
	LibMgr::SetPtr(Library::CLIENT, (void *)clientdll);
	
	return true;
}

bool CExtSigsegv::SDK_OnMetamodUnload(char *error, size_t maxlen)
{
	return true;
}


bool CExtSigsegv::RegisterConCommandBase(ConCommandBase *pCommand)
{
	META_REGCVAR(pCommand);
	return true;
}


void CExtSigsegv::LevelInitPreEntity()
{
	this->LoadSoundOverrides();
	
	g_ModManager.LevelInitPreEntity();
}

void CExtSigsegv::LevelInitPostEntity()
{
	g_ModManager.LevelInitPostEntity();
}

void CExtSigsegv::LevelShutdownPreEntity()
{
	g_ModManager.LevelShutdownPreEntity();
}

void CExtSigsegv::LevelShutdownPostEntity()
{
	g_ModManager.LevelShutdownPostEntity();
}

void CExtSigsegv::FrameUpdatePreEntityThink()
{
	g_ModManager.FrameUpdatePreEntityThink();
}

void CExtSigsegv::FrameUpdatePostEntityThink()
{
	g_ModManager.FrameUpdatePostEntityThink();
}


void CExtSigsegv::LoadSoundOverrides()
{
	if (soundemitterbase != nullptr) {
		soundemitterbase->AddSoundOverrides("scripts/sigsegv_sound_overrides.txt", true);
	}
}
