#include "factory.h"


#if defined _WINDOWS
#define DLL_EXT_STRINGS { ".dll" }
#elif defined _LINUX
#define DLL_EXT_STRINGS { ".so", "_srv.so" }
#elif defined _OSX
#define DLL_EXT_STRINGS { ".dylib" }
#endif


CreateInterfaceFn GetFactory_NoExt(const char *name)
{
	char buf[1024];
	
	for (auto ext : DLL_EXT_STRINGS) {
		snprintf(buf, sizeof(buf), "%s%s", name, ext);
		
		auto fn = Sys_GetFactory(buf);
		if (fn != nullptr) return fn;
	}
	
	return nullptr;
}


#define DEF_GET_FACTORY(name, libname) \
	CreateInterfaceFn name ## Factory() \
	{ \
		static bool init = false; \
		static CreateInterfaceFn factory = nullptr; \
		if (!init) { \
			factory = GetFactory_NoExt(libname); \
			if (factory == nullptr) DevWarning("Factory is nullptr: " #name "\n"); \
			init = true; \
		} \
		return factory; \
	}


DEF_GET_FACTORY(Client,             "client");
DEF_GET_FACTORY(SoundEmitterSystem, "soundemittersystem");
DEF_GET_FACTORY(MaterialSystem,     "materialsystem");
DEF_GET_FACTORY(VGUI,               "vgui2");
DEF_GET_FACTORY(VGUIMatSurface,     "vguimatsurface");
DEF_GET_FACTORY(Dedicated,          "dedicated");
DEF_GET_FACTORY(DataCache,          "datacache");


/* all of the stuff below is in tier1, but we can't use the versions in tier1
 * because sm_sdk_ext.cpp defines CreateInterface, so we get a static linker
 * multiple symbol definition error */


#ifdef POSIX
// Linux doesn't have this function so this emulates its functionality
void *GetModuleHandle(const char *name)
{
	void *handle;

	if( name == NULL )
	{
		// hmm, how can this be handled under linux....
		// is it even needed?
		return NULL;
	}

    if( (handle=dlopen(name, RTLD_NOW))==NULL)
    {
    //	DevWarning("DLOPEN Error: %s\n", dlerror());
        // couldn't open this file
        return NULL;
    }

	// read "man dlopen" for details
	// in short dlopen() inc a ref count
	// so dec the ref count by performing the close
	dlclose(handle);
	return handle;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a function, given a module
// Input  : pModuleName - module name
//			*pName - proc name
//-----------------------------------------------------------------------------
static void *Sys_GetProcAddress( const char *pModuleName, const char *pName )
{
	HMODULE hModule = (HMODULE)GetModuleHandle( pModuleName );
	
	/* avoid idiotic cases where we get a nullptr handle, which then makes us
	 * dlopen ourselves */
	if ((void *)hModule == nullptr) return nullptr;
	
#ifdef WIN32
	return (void *)GetProcAddress( hModule, pName );
#else
	return (void *)dlsym( (void *)hModule, pName );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: returns the instance of the named module
// Input  : *pModuleName - name of the module
// Output : interface_instance_t - instance of that module
//-----------------------------------------------------------------------------
CreateInterfaceFn Sys_GetFactory( const char *pModuleName )
{
#ifdef _WIN32
	return static_cast<CreateInterfaceFn>( Sys_GetProcAddress( pModuleName, CREATEINTERFACE_PROCNAME ) );
#elif defined(POSIX)
	// see Sys_GetFactory( CSysModule *pModule ) for an explanation
	return (CreateInterfaceFn)( Sys_GetProcAddress( pModuleName, CREATEINTERFACE_PROCNAME ) );
#endif
}
