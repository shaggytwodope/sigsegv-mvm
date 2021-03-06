#ifndef _INCLUDE_SIGSEGV_STUB_ECON_H_
#define _INCLUDE_SIGSEGV_STUB_ECON_H_


#include "link/link.h"


constexpr int NUM_SHOOT_SOUND_TYPES = 15;
constexpr int NUM_VISUALS_BLOCKS    = 5;


struct perteamvisuals_t
{
	DECL_EXTRACT(const char *[NUM_SHOOT_SOUND_TYPES], m_Sounds);
};


class CEconItemDefinition
{
public:
	uint32_t vtable;
	KeyValues *m_pKV;
	short m_iItemDefIndex;
	bool m_bEnabled;
	byte m_iMinILevel;
	byte m_iMaxILevel;
	byte m_iItemQuality;
	byte m_iForcedItemQuality;
	byte m_iItemRarity;
	short m_iDefaultDropQuantity;
	byte m_iItemSeries;
	// ...
	
	DECL_EXTRACT(perteamvisuals_t *[NUM_VISUALS_BLOCKS], m_Visuals);
};
class CTFItemDefinition : public CEconItemDefinition {};


class CEconItemAttributeDefinition
{
public:
	unsigned short GetIndex() const { return this->m_iIndex; }
	
private:
	KeyValues *m_KeyValues;  // +0x00
	unsigned short m_iIndex; // +0x04
	// ...
};


class CEconItemView
{
public:
	CTFItemDefinition *GetStaticData() const;
	CTFItemDefinition *GetItemDefinition() const { return this->GetStaticData(); }
	
	int GetItemDefIndex() const { return vt_GetItemDefIndex(this); }
	
private:
	static MemberVFuncThunk<const CEconItemView *, int> vt_GetItemDefIndex;
};


class CAttributeManager
{
public:
	void NetworkStateChanged()           {}
	void NetworkStateChanged(void *pVar) {}
	
	template<typename T>
	static T AttribHookValue(T value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec = nullptr, bool b1 = true);
	
	static StaticFuncThunk<int, int, const char *, const CBaseEntity *, CUtlVector<CBaseEntity *> *, bool>     ft_AttribHookValue_int;
	static StaticFuncThunk<float, float, const char *, const CBaseEntity *, CUtlVector<CBaseEntity *> *, bool> ft_AttribHookValue_float;
};

template<> inline int CAttributeManager::AttribHookValue<int>(int value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
{ return CAttributeManager::ft_AttribHookValue_int(value, attr, ent, vec, b1); }
template<> inline float CAttributeManager::AttribHookValue<float>(float value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
{ return CAttributeManager::ft_AttribHookValue_float(value, attr, ent, vec, b1); }

template<typename T> void _CallAttribHookRef(T& value, const char *pszClass, const CBaseEntity *pEntity) { value = CAttributeManager::AttribHookValue<T>(value, pszClass, pEntity); }
#define CALL_ATTRIB_HOOK_INT(value, name)                 _CallAttribHookRef<int>  (value, #name, this)
#define CALL_ATTRIB_HOOK_INT_ON_OTHER(ent, value, name)   _CallAttribHookRef<int>  (value, #name, ent )
#define CALL_ATTRIB_HOOK_FLOAT(value, name)               _CallAttribHookRef<float>(value, #name, this)
#define CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(ent, value, name) _CallAttribHookRef<float>(value, #name, ent )


class CAttributeContainer : public CAttributeManager
{
public:
	CEconItemView *GetItem() { return &this->m_Item; }
	
private:
	DECL_SENDPROP_RW(CEconItemView, m_Item);
};


class CEconItemSystem
{
public:
	// TODO
	
private:
	// TODO
};
class CTFItemSystem : public CEconItemSystem {};


class CEconItemSchema
{
public:
	CEconItemDefinition *GetItemDefinition(int def_idx) const                    { return ft_GetItemDefinition           (this, def_idx); }
	CEconItemDefinition *GetItemDefinitionByName(const char *name)               { return ft_GetItemDefinitionByName     (this, name); }
	CEconItemAttributeDefinition *GetAttributeDefinitionByName(const char *name) { return ft_GetAttributeDefinitionByName(this, name); }
	
private:
	static MemberFuncThunk<const CEconItemSchema *, CEconItemDefinition *, int>                   ft_GetItemDefinition;
	static MemberFuncThunk<      CEconItemSchema *, CEconItemDefinition *, const char *>          ft_GetItemDefinitionByName;
	static MemberFuncThunk<      CEconItemSchema *, CEconItemAttributeDefinition *, const char *> ft_GetAttributeDefinitionByName;
};
class CTFItemSchema : public CEconItemSchema {};


CTFItemSchema *GetItemSchema();


extern GlobalThunk<const char *[5]> g_TeamVisualSections;


#endif
