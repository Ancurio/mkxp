#==============================================================================
# ** Scene_Battle (part 4)
#------------------------------------------------------------------------------
#  This class performs battle screen processing.
#==============================================================================

class Scene_Battle
  #--------------------------------------------------------------------------
  # * Start Main Phase
  #--------------------------------------------------------------------------
  def start_phase4
    # Shift to phase 4
    @phase = 4
    # Turn count
    $game_temp.battle_turn += 1
    # Search all battle event pages
    for index in 0...$data_troops[@troop_id].pages.size
      # Get event page
      page = $data_troops[@troop_id].pages[index]
      # If this page span is [turn]
      if page.span == 1
        # Clear action completed flags
        $game_temp.battle_event_flags[index] = false
      end
    end
    # Set actor as unselectable
    @actor_index = -1
    @active_battler = nil
    # Enable party command window
    @party_command_window.active = false
    @party_command_window.visible = false
    # Disable actor command window
    @actor_command_window.active = false
    @actor_command_window.visible = false
    # Set main phase flag
    $game_temp.battle_main_phase = true
    # Make enemy action
    for enemy in $game_troop.enemies
      enemy.make_action
    end
    # Make action orders
    make_action_orders
    # Shift to step 1
    @phase4_step = 1
  end
  #--------------------------------------------------------------------------
  # * Make Action Orders
  #--------------------------------------------------------------------------
  def make_action_orders
    # Initialize @action_battlers array
    @action_battlers = []
    # Add enemy to @action_battlers array
    for enemy in $game_troop.enemies
      @action_battlers.push(enemy)
    end
    # Add actor to @action_battlers array
    for actor in $game_party.actors
      @action_battlers.push(actor)
    end
    # Decide action speed for all
    for battler in @action_battlers
      battler.make_action_speed
    end
    # Line up action speed in order from greatest to least
    @action_battlers.sort! {|a,b|
      b.current_action.speed - a.current_action.speed }
  end
  #--------------------------------------------------------------------------
  # * Frame Update (main phase)
  #--------------------------------------------------------------------------
  def update_phase4
    case @phase4_step
    when 1
      update_phase4_step1
    when 2
      update_phase4_step2
    when 3
      update_phase4_step3
    when 4
      update_phase4_step4
    when 5
      update_phase4_step5
    when 6
      update_phase4_step6
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (main phase step 1 : action preparation)
  #--------------------------------------------------------------------------
  def update_phase4_step1
    # Hide help window
    @help_window.visible = false
    # Determine win/loss
    if judge
      # If won, or if lost : end method
      return
    end
    # If an action forcing battler doesn't exist
    if $game_temp.forcing_battler == nil
      # Set up battle event
      setup_battle_event
      # If battle event is running
      if $game_system.battle_interpreter.running?
        return
      end
    end
    # If an action forcing battler exists
    if $game_temp.forcing_battler != nil
      # Add to head, or move
      @action_battlers.delete($game_temp.forcing_battler)
      @action_battlers.unshift($game_temp.forcing_battler)
    end
    # If no actionless battlers exist (all have performed an action)
    if @action_battlers.size == 0
      # Start party command phase
      start_phase2
      return
    end
    # Initialize animation ID and common event ID
    @animation1_id = 0
    @animation2_id = 0
    @common_event_id = 0
    # Shift from head of actionless battlers
    @active_battler = @action_battlers.shift
    # If already removed from battle
    if @active_battler.index == nil
      return
    end
    # Slip damage
    if @active_battler.hp > 0 and @active_battler.slip_damage?
      @active_battler.slip_damage_effect
      @active_battler.damage_pop = true
    end
    # Natural removal of states
    @active_battler.remove_states_auto
    # Refresh status window
    @status_window.refresh
    # Shift to step 2
    @phase4_step = 2
  end
  #--------------------------------------------------------------------------
  # * Frame Update (main phase step 2 : start action)
  #--------------------------------------------------------------------------
  def update_phase4_step2
    # If not a forcing action
    unless @active_battler.current_action.forcing
      # If restriction is [normal attack enemy] or [normal attack ally]
      if @active_battler.restriction == 2 or @active_battler.restriction == 3
        # Set attack as an action
        @active_battler.current_action.kind = 0
        @active_battler.current_action.basic = 0
      end
      # If restriction is [cannot perform action]
      if @active_battler.restriction == 4
        # Clear battler being forced into action
        $game_temp.forcing_battler = nil
        # Shift to step 1
        @phase4_step = 1
        return
      end
    end
    # Clear target battlers
    @target_battlers = []
    # Branch according to each action
    case @active_battler.current_action.kind
    when 0  # basic
      make_basic_action_result
    when 1  # skill
      make_skill_action_result
    when 2  # item
      make_item_action_result
    end
    # Shift to step 3
    if @phase4_step == 2
      @phase4_step = 3
    end
  end
  #--------------------------------------------------------------------------
  # * Make Basic Action Results
  #--------------------------------------------------------------------------
  def make_basic_action_result
    # If attack
    if @active_battler.current_action.basic == 0
      # Set anaimation ID
      @animation1_id = @active_battler.animation1_id
      @animation2_id = @active_battler.animation2_id
      # If action battler is enemy
      if @active_battler.is_a?(Game_Enemy)
        if @active_battler.restriction == 3
          target = $game_troop.random_target_enemy
        elsif @active_battler.restriction == 2
          target = $game_party.random_target_actor
        else
          index = @active_battler.current_action.target_index
          target = $game_party.smooth_target_actor(index)
        end
      end
      # If action battler is actor
      if @active_battler.is_a?(Game_Actor)
        if @active_battler.restriction == 3
          target = $game_party.random_target_actor
        elsif @active_battler.restriction == 2
          target = $game_troop.random_target_enemy
        else
          index = @active_battler.current_action.target_index
          target = $game_troop.smooth_target_enemy(index)
        end
      end
      # Set array of targeted battlers
      @target_battlers = [target]
      # Apply normal attack results
      for target in @target_battlers
        target.attack_effect(@active_battler)
      end
      return
    end
    # If guard
    if @active_battler.current_action.basic == 1
      # Display "Guard" in help window
      @help_window.set_text($data_system.words.guard, 1)
      return
    end
    # If escape
    if @active_battler.is_a?(Game_Enemy) and
       @active_battler.current_action.basic == 2
      # Display "Escape" in help window
      @help_window.set_text("Escape", 1)
      # Escape
      @active_battler.escape
      return
    end
    # If doing nothing
    if @active_battler.current_action.basic == 3
      # Clear battler being forced into action
      $game_temp.forcing_battler = nil
      # Shift to step 1
      @phase4_step = 1
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Set Targeted Battler for Skill or Item
  #     scope : effect scope for skill or item
  #--------------------------------------------------------------------------
  def set_target_battlers(scope)
    # If battler performing action is enemy
    if @active_battler.is_a?(Game_Enemy)
      # Branch by effect scope
      case scope
      when 1  # single enemy
        index = @active_battler.current_action.target_index
        @target_battlers.push($game_party.smooth_target_actor(index))
      when 2  # all enemies
        for actor in $game_party.actors
          if actor.exist?
            @target_battlers.push(actor)
          end
        end
      when 3  # single ally
        index = @active_battler.current_action.target_index
        @target_battlers.push($game_troop.smooth_target_enemy(index))
      when 4  # all allies
        for enemy in $game_troop.enemies
          if enemy.exist?
            @target_battlers.push(enemy)
          end
        end
      when 5  # single ally (HP 0)
        index = @active_battler.current_action.target_index
        enemy = $game_troop.enemies[index]
        if enemy != nil and enemy.hp0?
          @target_battlers.push(enemy)
        end
      when 6  # all allies (HP 0)
        for enemy in $game_troop.enemies
          if enemy != nil and enemy.hp0?
            @target_battlers.push(enemy)
          end
        end
      when 7  # user
        @target_battlers.push(@active_battler)
      end
    end
    # If battler performing action is actor
    if @active_battler.is_a?(Game_Actor)
      # Branch by effect scope
      case scope
      when 1  # single enemy
        index = @active_battler.current_action.target_index
        @target_battlers.push($game_troop.smooth_target_enemy(index))
      when 2  # all enemies
        for enemy in $game_troop.enemies
          if enemy.exist?
            @target_battlers.push(enemy)
          end
        end
      when 3  # single ally
        index = @active_battler.current_action.target_index
        @target_battlers.push($game_party.smooth_target_actor(index))
      when 4  # all allies
        for actor in $game_party.actors
          if actor.exist?
            @target_battlers.push(actor)
          end
        end
      when 5  # single ally (HP 0)
        index = @active_battler.current_action.target_index
        actor = $game_party.actors[index]
        if actor != nil and actor.hp0?
          @target_battlers.push(actor)
        end
      when 6  # all allies (HP 0)
        for actor in $game_party.actors
          if actor != nil and actor.hp0?
            @target_battlers.push(actor)
          end
        end
      when 7  # user
        @target_battlers.push(@active_battler)
      end
    end
  end
  #--------------------------------------------------------------------------
  # * Make Skill Action Results
  #--------------------------------------------------------------------------
  def make_skill_action_result
    # Get skill
    @skill = $data_skills[@active_battler.current_action.skill_id]
    # If not a forcing action
    unless @active_battler.current_action.forcing
      # If unable to use due to SP running out
      unless @active_battler.skill_can_use?(@skill.id)
        # Clear battler being forced into action
        $game_temp.forcing_battler = nil
        # Shift to step 1
        @phase4_step = 1
        return
      end
    end
    # Use up SP
    @active_battler.sp -= @skill.sp_cost
    # Refresh status window
    @status_window.refresh
    # Show skill name on help window
    @help_window.set_text(@skill.name, 1)
    # Set animation ID
    @animation1_id = @skill.animation1_id
    @animation2_id = @skill.animation2_id
    # Set command event ID
    @common_event_id = @skill.common_event_id
    # Set target battlers
    set_target_battlers(@skill.scope)
    # Apply skill effect
    for target in @target_battlers
      target.skill_effect(@active_battler, @skill)
    end
  end
  #--------------------------------------------------------------------------
  # * Make Item Action Results
  #--------------------------------------------------------------------------
  def make_item_action_result
    # Get item
    @item = $data_items[@active_battler.current_action.item_id]
    # If unable to use due to items running out
    unless $game_party.item_can_use?(@item.id)
      # Shift to step 1
      @phase4_step = 1
      return
    end
    # If consumable
    if @item.consumable
      # Decrease used item by 1
      $game_party.lose_item(@item.id, 1)
    end
    # Display item name on help window
    @help_window.set_text(@item.name, 1)
    # Set animation ID
    @animation1_id = @item.animation1_id
    @animation2_id = @item.animation2_id
    # Set common event ID
    @common_event_id = @item.common_event_id
    # Decide on target
    index = @active_battler.current_action.target_index
    target = $game_party.smooth_target_actor(index)
    # Set targeted battlers
    set_target_battlers(@item.scope)
    # Apply item effect
    for target in @target_battlers
      target.item_effect(@item)
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (main phase step 3 : animation for action performer)
  #--------------------------------------------------------------------------
  def update_phase4_step3
    # Animation for action performer (if ID is 0, then white flash)
    if @animation1_id == 0
      @active_battler.white_flash = true
    else
      @active_battler.animation_id = @animation1_id
      @active_battler.animation_hit = true
    end
    # Shift to step 4
    @phase4_step = 4
  end
  #--------------------------------------------------------------------------
  # * Frame Update (main phase step 4 : animation for target)
  #--------------------------------------------------------------------------
  def update_phase4_step4
    # Animation for target
    for target in @target_battlers
      target.animation_id = @animation2_id
      target.animation_hit = (target.damage != "Miss")
    end
    # Animation has at least 8 frames, regardless of its length
    @wait_count = 8
    # Shift to step 5
    @phase4_step = 5
  end
  #--------------------------------------------------------------------------
  # * Frame Update (main phase step 5 : damage display)
  #--------------------------------------------------------------------------
  def update_phase4_step5
    # Hide help window
    @help_window.visible = false
    # Refresh status window
    @status_window.refresh
    # Display damage
    for target in @target_battlers
      if target.damage != nil
        target.damage_pop = true
      end
    end
    # Shift to step 6
    @phase4_step = 6
  end
  #--------------------------------------------------------------------------
  # * Frame Update (main phase step 6 : refresh)
  #--------------------------------------------------------------------------
  def update_phase4_step6
    # Clear battler being forced into action
    $game_temp.forcing_battler = nil
    # If common event ID is valid
    if @common_event_id > 0
      # Set up event
      common_event = $data_common_events[@common_event_id]
      $game_system.battle_interpreter.setup(common_event.list, 0)
    end
    # Shift to step 1
    @phase4_step = 1
  end
end
