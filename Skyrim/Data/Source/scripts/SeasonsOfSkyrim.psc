Scriptname SeasonsOfSkyrim Hidden 

;/	SEASON
	None = 0
	Winter = 1
	Spring = 2
	Summer = 3
	Autumn = 4
/;

;Get current season 
int Function GetCurrentSeason() global native

;Functions to override current season
;Takes effect after interior->exterior transition
int Function GetSeasonOverride() global native
Function SetSeasonOverride(int aiSeason) global native
Function ClearSeasonOverride() global native

;Fires on season change (during interior->exterior transition)
Function RegisterForSeasonChange_Form(Form akForm) global native	
Function RegisterForSeasonChange_Alias(ReferenceAlias akAlias) global native
Function RegisterForSeasonChange_AME(ActiveMagicEffect akActiveEffect) global native

Function UnregisterForSeasonChange_Form(Form akForm) global native	
Function UnregisterForSeasonChange_Alias(ReferenceAlias akAlias) global native
Function UnregisterForSeasonChange_AME(ActiveMagicEffect akActiveEffect) global native	

Event OnSeasonChange(int aiOldSeason, int aiNewSeason, bool abOverride)
endEvent