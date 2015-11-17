#==============================================================================
# ** Game_Battler (part 3)
#------------------------------------------------------------------------------
#  This class deals with battlers. It's used as a superclass for the Game_Actor
#  and Game_Enemy classes.
#==============================================================================

class Game_Battler
  #--------------------------------------------------------------------------
  # * Determine Usable Skills
  #     skill_id : skill ID
  #--------------------------------------------------------------------------
  def skill_can_use?(skill_id)
    # If there's not enough SP, the skill cannot be used.
    if $data_skills[skill_id].sp_cost > self.sp
      return false
    end
    # Unusable if incapacitated
    if dead?
      return false
    end
    # If silent, only physical skills can be used
    if $data_skills[skill_id].atk_f == 0 and self.restriction == 1
      return false
    end
    # Get usable time
    occasion = $data_skills[skill_id].occasion
    # If in battle
    if $game_temp.in_battle
      # Usable with [Normal] and [Only Battle]
      return (occasion == 0 or occasion == 1)
    # If not in battle
    else
      # Usable with [Normal] and [Only Menu]
      return (occasion == 0 or occasion == 2)
    end
  end
  #--------------------------------------------------------------------------
  # * Applying Normal Attack Effects
  #     attacker : battler
  #--------------------------------------------------------------------------
  def attack_effect(attacker)
    # Clear critical flag
    self.critical = false
    # First hit detection
    hit_result = (rand(100) < attacker.hit)
    # If hit occurs
    if hit_result == true
      # Calculate basic damage
      atk = [attacker.atk - self.pdef / 2, 0].max
      self.damage = atk * (20 + attacker.str) / 20
      # Element correction
      self.damage *= elements_correct(attacker.element_set)
      self.damage /= 100
      # If damage value is strictly positive
      if self.damage > 0
        # Critical correction
        if rand(100) < 4 * attacker.dex / self.agi
          self.damage *= 2
          self.critical = true
        end
        # Guard correction
        if self.guarding?
          self.damage /= 2
        end
      end
      # Dispersion
      if self.damage.abs > 0
        amp = [self.damage.abs * 15 / 100, 1].max
        self.damage += rand(amp+1) + rand(amp+1) - amp
      end
      # Second hit detection
      eva = 8 * self.agi / attacker.dex + self.eva
      hit = self.damage < 0 ? 100 : 100 - eva
      hit = self.cant_evade? ? 100 : hit
      hit_result = (rand(100) < hit)
    end
    # If hit occurs
    if hit_result == true
      # State Removed by Shock
      remove_states_shock
      # Substract damage from HP
      self.hp -= self.damage
      # State change
      @state_changed = false
      states_plus(attacker.plus_state_set)
      states_minus(attacker.minus_state_set)
    # When missing
    else
      # Set damage to "Miss"
      self.damage = "Miss"
      # Clear critical flag
      self.critical = false
    end
    # End Method
    return true
  end
  #--------------------------------------------------------------------------
  # * Apply Skill Effects
  #     user  : the one using skills (battler)
  #     skill : skill
  #--------------------------------------------------------------------------
  def skill_effect(user, skill)
    # Clear critical flag
    self.critical = false
    # If skill scope is for ally with 1 or more HP, and your own HP = 0,
    # or skill scope is for ally with 0, and your own HP = 1 or more
    if ((skill.scope == 3 or skill.scope == 4) and self.hp == 0) or
       ((skill.scope == 5 or skill.scope == 6) and self.hp >= 1)
      # End Method
      return false
    end
    # Clear effective flag
    effective = false
    # Set effective flag if common ID is effective
    effective |= skill.common_event_id > 0
    # First hit detection
    hit = skill.hit
    if skill.atk_f > 0
      hit *= user.hit / 100
    end
    hit_result = (rand(100) < hit)
    # Set effective flag if skill is uncertain
    effective |= hit < 100
    # If hit occurs
    if hit_result == true
      # Calculate power
      power = skill.power + user.atk * skill.atk_f / 100
      if power > 0
        power -= self.pdef * skill.pdef_f / 200
        power -= self.mdef * skill.mdef_f / 200
        power = [power, 0].max
      end
      # Calculate rate
      rate = 20
      rate += (user.str * skill.str_f / 100)
      rate += (user.dex * skill.dex_f / 100)
      rate += (user.agi * skill.agi_f / 100)
      rate += (user.int * skill.int_f / 100)
      # Calculate basic damage
      self.damage = power * rate / 20
      # Element correction
      self.damage *= elements_correct(skill.element_set)
      self.damage /= 100
      # If damage value is strictly positive
      if self.damage > 0
        # Guard correction
        if self.guarding?
          self.damage /= 2
        end
      end
      # Dispersion
      if skill.variance > 0 and self.damage.abs > 0
        amp = [self.damage.abs * skill.variance / 100, 1].max
        self.damage += rand(amp+1) + rand(amp+1) - amp
      end
      # Second hit detection
      eva = 8 * self.agi / user.dex + self.eva
      hit = self.damage < 0 ? 100 : 100 - eva * skill.eva_f / 100
      hit = self.cant_evade? ? 100 : hit
      hit_result = (rand(100) < hit)
      # Set effective flag if skill is uncertain
      effective |= hit < 100
    end
    # If hit occurs
    if hit_result == true
      # If physical attack has power other than 0
      if skill.power != 0 and skill.atk_f > 0
        # State Removed by Shock
        remove_states_shock
        # Set to effective flag
        effective = true
      end
      # Substract damage from HP
      last_hp = self.hp
      self.hp -= self.damage
      effective |= self.hp != last_hp
      # State change
      @state_changed = false
      effective |= states_plus(skill.plus_state_set)
      effective |= states_minus(skill.minus_state_set)
      # If power is 0
      if skill.power == 0
        # Set damage to an empty string
        self.damage = ""
        # If state is unchanged
        unless @state_changed
          # Set damage to "Miss"
          self.damage = "Miss"
        end
      end
    # If miss occurs
    else
      # Set damage to "Miss"
      self.damage = "Miss"
    end
    # If not in battle
    unless $game_temp.in_battle
      # Set damage to nil
      self.damage = nil
    end
    # End Method
    return effective
  end
  #--------------------------------------------------------------------------
  # * Application of Item Effects
  #     item : item
  #--------------------------------------------------------------------------
  def item_effect(item)
    # Clear critical flag
    self.critical = false
    # If item scope is for ally with 1 or more HP, and your own HP = 0,
    # or item scope is for ally with 0 HP, and your own HP = 1 or more
    if ((item.scope == 3 or item.scope == 4) and self.hp == 0) or
       ((item.scope == 5 or item.scope == 6) and self.hp >= 1)
      # End Method
      return false
    end
    # Clear effective flag
    effective = false
    # Set effective flag if common ID is effective
    effective |= item.common_event_id > 0
    # Determine hit
    hit_result = (rand(100) < item.hit)
    # Set effective flag is skill is uncertain
    effective |= item.hit < 100
    # If hit occurs
    if hit_result == true
      # Calculate amount of recovery
      recover_hp = maxhp * item.recover_hp_rate / 100 + item.recover_hp
      recover_sp = maxsp * item.recover_sp_rate / 100 + item.recover_sp
      if recover_hp < 0
        recover_hp += self.pdef * item.pdef_f / 20
        recover_hp += self.mdef * item.mdef_f / 20
        recover_hp = [recover_hp, 0].min
      end
      # Element correction
      recover_hp *= elements_correct(item.element_set)
      recover_hp /= 100
      recover_sp *= elements_correct(item.element_set)
      recover_sp /= 100
      # Dispersion
      if item.variance > 0 and recover_hp.abs > 0
        amp = [recover_hp.abs * item.variance / 100, 1].max
        recover_hp += rand(amp+1) + rand(amp+1) - amp
      end
      if item.variance > 0 and recover_sp.abs > 0
        amp = [recover_sp.abs * item.variance / 100, 1].max
        recover_sp += rand(amp+1) + rand(amp+1) - amp
      end
      # If recovery code is negative
      if recover_hp < 0
        # Guard correction
        if self.guarding?
          recover_hp /= 2
        end
      end
      # Set damage value and reverse HP recovery amount
      self.damage = -recover_hp
      # HP and SP recovery
      last_hp = self.hp
      last_sp = self.sp
      self.hp += recover_hp
      self.sp += recover_sp
      effective |= self.hp != last_hp
      effective |= self.sp != last_sp
      # State change
      @state_changed = false
      effective |= states_plus(item.plus_state_set)
      effective |= states_minus(item.minus_state_set)
      # If parameter value increase is effective
      if item.parameter_type > 0 and item.parameter_points != 0
        # Branch by parameter
        case item.parameter_type
        when 1  # Max HP
          @maxhp_plus += item.parameter_points
        when 2  # Max SP
          @maxsp_plus += item.parameter_points
        when 3  # Strength
          @str_plus += item.parameter_points
        when 4  # Dexterity
          @dex_plus += item.parameter_points
        when 5  # Agility
          @agi_plus += item.parameter_points
        when 6  # Intelligence
          @int_plus += item.parameter_points
        end
        # Set to effective flag
        effective = true
      end
      # If HP recovery rate and recovery amount are 0
      if item.recover_hp_rate == 0 and item.recover_hp == 0
        # Set damage to empty string
        self.damage = ""
        # If SP recovery rate / recovery amount are 0, and parameter increase
        # value is ineffective.
        if item.recover_sp_rate == 0 and item.recover_sp == 0 and
           (item.parameter_type == 0 or item.parameter_points == 0)
          # If state is unchanged
          unless @state_changed
            # Set damage to "Miss"
            self.damage = "Miss"
          end
        end
      end
    # If miss occurs
    else
      # Set damage to "Miss"
      self.damage = "Miss"
    end
    # If not in battle
    unless $game_temp.in_battle
      # Set damage to nil
      self.damage = nil
    end
    # End Method
    return effective
  end
  #--------------------------------------------------------------------------
  # * Application of Slip Damage Effects
  #--------------------------------------------------------------------------
  def slip_damage_effect
    # Set damage
    self.damage = self.maxhp / 10
    # Dispersion
    if self.damage.abs > 0
      amp = [self.damage.abs * 15 / 100, 1].max
      self.damage += rand(amp+1) + rand(amp+1) - amp
    end
    # Subtract damage from HP
    self.hp -= self.damage
    # End Method
    return true
  end
  #--------------------------------------------------------------------------
  # * Calculating Element Correction
  #     element_set : element
  #--------------------------------------------------------------------------
  def elements_correct(element_set)
    # If not an element
    if element_set == []
      # Return 100
      return 100
    end
    # Return the weakest object among the elements given
    # * "element_rate" method is defined by Game_Actor and Game_Enemy classes,
    #    which inherit from this class.
    weakest = -100
    for i in element_set
      weakest = [weakest, self.element_rate(i)].max
    end
    return weakest
  end
end
