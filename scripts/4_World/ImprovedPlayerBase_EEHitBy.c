modded class PlayerBase
{
	protected float m_LastZombieProcTime;

	// Base chance per infected hit to apply an extra effect
	protected const float ZOMBIE_EXTRA_EFFECT_BASE_CHANCE = 0.05; // 5%
	// Cooldown to dedupe multiple EEHitBy events from a single swing (seconds)
	protected const float ZOMBIE_EFFECT_COOLDOWN_SEC = 1.0;
	// Toggle debug printing
	protected const bool ZOMBIE_EFFECT_DEBUG = false;

	// Weighted distribution of effects (must sum to 100)
	protected const int WEIGHT_CHOLERA = 30; // 30%
	protected const int WEIGHT_FLU = 30;     // 30%
	protected const int WEIGHT_TOXIC = 25;   // 25%
	protected const int WEIGHT_KO = 15;      // 15%

	override void EEHitBy(
		TotalDamageResult damageResult,
		int damageType,
		EntityAI source,
		int component,
		string dmgZone,
		string ammo,
		vector modelPos,
		float speedCoef
	)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		// Run this extra logic on server only to avoid double application
		if (!GetGame() || !GetGame().IsServer())
			return;

		// Must be alive for effects to matter
		if (!IsAlive())
			return;

		// Only trigger if attacker is an infected (zombie)
		if (!source || !source.IsInherited(ZombieBase))
			return;

		// Simple dedupe: enforce a short cooldown between procs
		if (!CanInfectedEffectProcCooldown())
			return;

		float adjustedChance = ComputeAdjustedChance(dmgZone);
		if (Math.RandomFloat01() >= adjustedChance)
			return;

		ApplyRandomInfectedEffect();
	}

	protected bool CanInfectedEffectProcCooldown()
	{
		float nowSec = GetGame().GetTime() * 0.001; // ms -> s
		if (nowSec - m_LastZombieProcTime < ZOMBIE_EFFECT_COOLDOWN_SEC)
			return false;
		m_LastZombieProcTime = nowSec;
		return true;
	}

	protected float ComputeAdjustedChance(string dmgZone)
	{
		float chance = ZOMBIE_EXTRA_EFFECT_BASE_CHANCE;

		// Reduce chance if wearing basic protective gear
		if (IsWearingMask())
			chance *= 0.5; // 50% reduction with mask
		if (IsWearingGloves())
			chance *= 0.7; // 30% reduction with gloves

		// Slightly increase when exposed zones are hit (optional rule-of-thumb)
		if (dmgZone && (dmgZone == "Head" || dmgZone == "Hands"))
			chance *= 1.25;

		return chance;
	}

	protected void ApplyRandomInfectedEffect()
	{
		int roll = Math.RandomInt(0, 100); // 0..99
		int thresholdCholera = WEIGHT_CHOLERA;
		int thresholdFlu = thresholdCholera + WEIGHT_FLU;
		int thresholdToxic = thresholdFlu + WEIGHT_TOXIC;

		ModifiersManager mm = GetModifiersManager();

		if (ZOMBIE_EFFECT_DEBUG)
			DebugPrintInfectedEffectRoll(roll);

		if (roll < thresholdCholera)
		{
			if (!mm.IsModifierActive(eModifiers.MDF_Cholera))
				mm.ActivateModifier(eModifiers.MDF_Cholera);
			return;
		}
		if (roll < thresholdFlu)
		{
			if (!mm.IsModifierActive(eModifiers.MDF_Flu))
				mm.ActivateModifier(eModifiers.MDF_Flu);
			return;
		}
		if (roll < thresholdToxic)
		{
			if (!mm.IsModifierActive(eModifiers.MDF_ToxicPoisoning))
				mm.ActivateModifier(eModifiers.MDF_ToxicPoisoning);
			return;
		}

		// Otherwise: Knockout via shock set to zero (safe KO)
		float currentShock = GetHealth("", "Shock");
		if (currentShock > 0)
			SetHealth("", "Shock", 0);
	}

	protected bool IsWearingMask()
	{
		EntityAI mask = EntityAI.Cast(GetInventory().FindAttachment(InventorySlots.MASK));
		return mask != null;
	}

	protected bool IsWearingGloves()
	{
		EntityAI gloves = EntityAI.Cast(GetInventory().FindAttachment(InventorySlots.GLOVES));
		return gloves != null;
	}

	protected void DebugPrintInfectedEffectRoll(int roll)
	{
		Print(string.Format("[ZombieEffect] roll=%1 cholera<%2 flu<%3 toxic<%4", roll, WEIGHT_CHOLERA, WEIGHT_CHOLERA + WEIGHT_FLU, WEIGHT_CHOLERA + WEIGHT_FLU + WEIGHT_TOXIC));
	}
}

