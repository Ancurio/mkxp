#==============================================================================
# ** Game_Battler (part 2)
#------------------------------------------------------------------------------
#  This class deals with battlers. It's used as a superclass for the Game_Actor
#  and Game_Enemy classes.
#==============================================================================

class Game_Battler
  #--------------------------------------------------------------------------
  # * Check State
  #     state_id : state ID
  #--------------------------------------------------------------------------
  def state?(state_id)
    # Return true if the applicable state is added.
    return @states.include?(state_id)
  end
  #--------------------------------------------------------------------------
  # * Determine if a state is full or not.
  #     state_id : state ID
  #--------------------------------------------------------------------------
  def state_full?(state_id)
    # Return false if the applicable state is not added.
    unless self.state?(state_id)
      return false
    end
    # Return true if the number of maintenance turns is -1 (auto state).
    if @states_turn[state_id] == -1
      return true
    end
    # Return true if the number of maintenance turns is equal to the
    # lowest number of natural removal turns.
    return @states_turn[state_id] == $data_states[state_id].hold_turn
  end
  #--------------------------------------------------------------------------
  # * Add State
  #     state_id : state ID
  #     force    : forcefully added flag (used to deal with auto state)
  #--------------------------------------------------------------------------
  def add_state(state_id, force = false)
    # For an ineffective state
    if $data_states[state_id] == nil
      # End Method
      return
    end
    # If not forcefully added
    unless force
      # A state loop already in existance
      for i in @states
        # If a new state is included in the state change (-) of an existing
        # state, and that state is not included in the state change (-) of
        # a new state (example: an attempt to add poison during dead)
        if $data_states[i].minus_state_set.include?(state_id) and
           not $data_states[state_id].minus_state_set.include?(i)
          # End Method
          return
        end
      end
    end
    # If this state is not added
    unless state?(state_id)
      # Add state ID to @states array
      @states.push(state_id)
      # If option [regarded as HP 0]is effective
      if $data_states[state_id].zero_hp
        # Change HP to 0
        @hp = 0
      end
      # All state loops
      for i in 1...$data_states.size
        # Dealing with a state change (+)
        if $data_states[state_id].plus_state_set.include?(i)
          add_state(i)
        end
        # Dealing with a state change (-)
        if $data_states[state_id].minus_state_set.include?(i)
          remove_state(i)
        end
      end
      # line change to a large rating order (if value is the same, then a
      # strong restriction order)
      @states.sort! do |a, b|
        state_a = $data_states[a]
        state_b = $data_states[b]
        if state_a.rating > state_b.rating
          -1
        elsif state_a.rating < state_b.rating
          +1
        elsif state_a.restriction > state_b.restriction
          -1
        elsif state_a.restriction < state_b.restriction
          +1
        else
          a <=> b
        end
      end
    end
    # If added forcefully
    if force
      # Set the natural removal's lowest number of turns to -1
      @states_turn[state_id] = -1
    end
    # If not added forcefully
    unless  @states_turn[state_id] == -1
      # Set the natural removal's lowest number of turns
      @states_turn[state_id] = $data_states[state_id].hold_turn
    end
    # If unable to move
    unless movable?
      # Clear action
      @current_action.clear
    end
    # Check the maximum value of HP and SP
    @hp = [@hp, self.maxhp].min
    @sp = [@sp, self.maxsp].min
  end
  #--------------------------------------------------------------------------
  # * Remove State
  #     state_id : state ID
  #     force    : forcefully removed flag (used to deal with auto state)
  #--------------------------------------------------------------------------
  def remove_state(state_id, force = false)
    # If this state is added
    if state?(state_id)
      # If a forcefully added state is not forcefully removed
      if @states_turn[state_id] == -1 and not force
        # End Method
        return
      end
      # If current HP is at 0 and options are effective [regarded as HP 0]
      if @hp == 0 and $data_states[state_id].zero_hp
        # Determine if there's another state [regarded as HP 0] or not
        zero_hp = false
        for i in @states
          if i != state_id and $data_states[i].zero_hp
            zero_hp = true
          end
        end
        # Change HP to 1 if OK to remove incapacitation.
        if zero_hp == false
          @hp = 1
        end
      end
      # Delete state ID from @states and @states_turn hash array
      @states.delete(state_id)
      @states_turn.delete(state_id)
    end
    # Check maximum value for HP and SP
    @hp = [@hp, self.maxhp].min
    @sp = [@sp, self.maxsp].min
  end
  #--------------------------------------------------------------------------
  # * Get State Animation ID
  #--------------------------------------------------------------------------
  def state_animation_id
    # If no states are added
    if @states.size == 0
      return 0
    end
    # Return state animation ID with maximum rating
    return $data_states[@states[0]].animation_id
  end
  #--------------------------------------------------------------------------
  # * Get Restriction
  #--------------------------------------------------------------------------
  def restriction
    restriction_max = 0
    # Get maximum restriction from currently added states
    for i in @states
      if $data_states[i].restriction >= restriction_max
        restriction_max = $data_states[i].restriction
      end
    end
    return restriction_max
  end
  #--------------------------------------------------------------------------
  # * Determine [Can't Get EXP] States
  #--------------------------------------------------------------------------
  def cant_get_exp?
    for i in @states
      if $data_states[i].cant_get_exp
        return true
      end
    end
    return false
  end
  #--------------------------------------------------------------------------
  # * Determine [Can't Evade] States
  #--------------------------------------------------------------------------
  def cant_evade?
    for i in @states
      if $data_states[i].cant_evade
        return true
      end
    end
    return false
  end
  #--------------------------------------------------------------------------
  # * Determine [Slip Damage] States
  #--------------------------------------------------------------------------
  def slip_damage?
    for i in @states
      if $data_states[i].slip_damage
        return true
      end
    end
    return false
  end
  #--------------------------------------------------------------------------
  # * Remove Battle States (called up during end of battle)
  #--------------------------------------------------------------------------
  def remove_states_battle
    for i in @states.clone
      if $data_states[i].battle_only
        remove_state(i)
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Natural Removal of States (called up each turn)
  #--------------------------------------------------------------------------
  def remove_states_auto
    for i in @states_turn.keys.clone
      if @states_turn[i] > 0
        @states_turn[i] -= 1
      elsif rand(100) < $data_states[i].auto_release_prob
        remove_state(i)
      end
    end
  end
  #--------------------------------------------------------------------------
  # * State Removed by Shock (called up each time physical damage occurs)
  #--------------------------------------------------------------------------
  def remove_states_shock
    for i in @states.clone
      if rand(100) < $data_states[i].shock_release_prob
        remove_state(i)
      end
    end
  end
  #--------------------------------------------------------------------------
  # * State Change (+) Application
  #     plus_state_set  : State Change (+)
  #--------------------------------------------------------------------------
  def states_plus(plus_state_set)
    # Clear effective flag
    effective = false
    # Loop (added state)
    for i in plus_state_set
      # If this state is not guarded
      unless self.state_guard?(i)
        # Set effective flag if this state is not full
        effective |= self.state_full?(i) == false
        # If states offer [no resistance]
        if $data_states[i].nonresistance
          # Set state change flag
          @state_changed = true
          # Add a state
          add_state(i)
        # If this state is not full
        elsif self.state_full?(i) == false
          # Convert state effectiveness to probability,
          # compare to random numbers
          if rand(100) < [0,100,80,60,40,20,0][self.state_ranks[i]]
            # Set state change flag
            @state_changed = true
            # Add a state
            add_state(i)
          end
        end
      end
    end
    # End Method
    return effective
  end
  #--------------------------------------------------------------------------
  # * Apply State Change (-)
  #     minus_state_set : state change (-)
  #--------------------------------------------------------------------------
  def states_minus(minus_state_set)
    # Clear effective flag
    effective = false
    # Loop (state to be removed)
    for i in minus_state_set
      # Set effective flag if this state is added
      effective |= self.state?(i)
      # Set a state change flag
      @state_changed = true
      # Remove state
      remove_state(i)
    end
    # End Method
    return effective
  end
end
