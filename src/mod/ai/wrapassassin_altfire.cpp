#include "mod.h"
#include "stub/tfweaponbase.h"
#include "stub/tfbot.h"
#include "util/scope.h"


namespace Mod_AI_WrapAssassin_AltFire
{
	RefCount rc_CTFBot_OpportunisticallyUseWeaponAbilities;
	DETOUR_DECL_MEMBER(Action<CTFBot> *, CTFBot_OpportunisticallyUseWeaponAbilities)
	{
	//	DevMsg("CTFBot::OpportunisticallyUseWeaponAbilities BEGIN\n");
		SCOPED_INCREMENT(rc_CTFBot_OpportunisticallyUseWeaponAbilities);
		auto result = DETOUR_MEMBER_CALL(CTFBot_OpportunisticallyUseWeaponAbilities)();
	//	DevMsg("CTFBot::OpportunisticallyUseWeaponAbilities END\n");
		return result;
	}
	
	DETOUR_DECL_MEMBER(int, CTFBat_Giftwrap_GetWeaponID)
	{
		if (rc_CTFBot_OpportunisticallyUseWeaponAbilities > 0) {
		//	DevMsg("  CTFBat_Giftwrap::GetWeaponID PASSTHRU\n");
			return TF_WEAPON_BAT_WOOD;
		}
		
	//	DevMsg("  CTFBat_Giftwrap::GetWeaponID PASSTHRU\n");
		return DETOUR_MEMBER_CALL(CTFBat_Giftwrap_GetWeaponID)();
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("AI:WrapAssassin_AltFire")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBot_OpportunisticallyUseWeaponAbilities, "CTFBot::OpportunisticallyUseWeaponAbilities");
			MOD_ADD_DETOUR_MEMBER(CTFBat_Giftwrap_GetWeaponID,                "CTFBat_Giftwrap::GetWeaponID");
		}
		
		void SetEnabled(bool enable)
		{
			this->ToggleAllDetours(enable);
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_wrapassassin_altfire", "0", FCVAR_NOTIFY,
		"Mod: make bots fire Wrap Assassin balls, similar to how they use the Sandman",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue) {
			ConVarRef var(pConVar);
			s_Mod.SetEnabled(var.GetBool());
		});
}
