#==============================================================================
# ** Scene_Battle (part 1)
#------------------------------------------------------------------------------
#  This class performs battle screen processing.
#==============================================================================

class Scene_Battle
  #--------------------------------------------------------------------------
  # * Main Processing
  #--------------------------------------------------------------------------
  def main
    # Initialize each kind of temporary battle data
    $game_temp.in_battle = true
    $game_temp.battle_turn = 0
    $game_temp.battle_event_flags.clear
    $game_temp.battle_abort = false
    $game_temp.battle_main_phase = false
    $game_temp.battleback_name = $game_map.battleback_name
    $game_temp.forcing_battler = nil
    # Initialize battle event interpreter
    $game_system.battle_interpreter.setup(nil, 0)
    # Prepare troop
    @troop_id = $game_temp.battle_troop_id
    $game_troop.setup(@troop_id)
    # Make actor command window
    s1 = $data_system.words.attack
    s2 = $data_system.words.skill
    s3 = $data_system.words.guard
    s4 = $data_system.words.item
    @actor_command_window = Window_Command.new(160, [s1, s2, s3, s4])
    @actor_command_window.y = 160
    @actor_command_window.back_opacity = 160
    @actor_command_window.active = false
    @actor_command_window.visible = false
    # Make other windows
    @party_command_window = Window_PartyCommand.new
    @help_window = Window_Help.new
    @help_window.back_opacity = 160
    @help_window.visible = false
    @status_window = Window_BattleStatus.new
    @message_window = Window_Message.new
    # Make sprite set
    @spriteset = Spriteset_Battle.new
    # Initialize wait count
    @wait_count = 0
    # Execute transition
    if $data_system.battle_transition == ""
      Graphics.transition(20)
    else
      Graphics.transition(40, "Graphics/Transitions/" +
        $data_system.battle_transition)
    end
    # Start pre-battle phase
    start_phase1
    # Main loop
    loop do
      # Update game screen
      Graphics.update
      # Update input information
      Input.update
      # Frame update
      update
      # Abort loop if screen is changed
      if $scene != self
        break
      end
    end
    # Refresh map
    $game_map.refresh
    # Prepare for transition
    Graphics.freeze
    # Dispose of windows
    @actor_command_window.dispose
    @party_command_window.dispose
    @help_window.dispose
    @status_window.dispose
    @message_window.dispose
    if @skill_window != nil
      @skill_window.dispose
    end
    if @item_window != nil
      @item_window.dispose
    end
    if @result_window != nil
      @result_window.dispose
    end
    # Dispose of sprite set
    @spriteset.dispose
    # If switching to title screen
    if $scene.is_a?(Scene_Title)
      # Fade out screen
      Graphics.transition
      Graphics.freeze
    end
    # If switching from battle test to any screen other than game over screen
    if $BTEST and not $scene.is_a?(Scene_Gameover)
      $scene = nil
    end
  end
  #--------------------------------------------------------------------------
  # * Determine Battle Win/Loss Results
  #--------------------------------------------------------------------------
  def judge
    # If all dead determinant is true, or number of members in party is 0
    if $game_party.all_dead? or $game_party.actors.size == 0
      # If possible to lose
      if $game_temp.battle_can_lose
        # Return to BGM before battle starts
        $game_system.bgm_play($game_temp.map_bgm)
        # Battle ends
        battle_end(2)
        # Return true
        return true
      end
      # Set game over flag
      $game_temp.gameover = true
      # Return true
      return true
    end
    # Return false if even 1 enemy exists
    for enemy in $game_troop.enemies
      if enemy.exist?
        return false
      end
    end
    # Start after battle phase (win)
    start_phase5
    # Return true
    return true
  end
  #--------------------------------------------------------------------------
  # * Battle Ends
  #     result : results (0:win 1:lose 2:escape)
  #--------------------------------------------------------------------------
  def battle_end(result)
    # Clear in battle flag
    $game_temp.in_battle = false
    # Clear entire party actions flag
    $game_party.clear_actions
    # Remove battle states
    for actor in $game_party.actors
      actor.remove_states_battle
    end
    # Clear enemies
    $game_troop.enemies.clear
    # Call battle callback
    if $game_temp.battle_proc != nil
      $game_temp.battle_proc.call(result)
      $game_temp.battle_proc = nil
    end
    # Switch to map screen
    $scene = Scene_Map.new
  end
  #--------------------------------------------------------------------------
  # * Battle Event Setup
  #--------------------------------------------------------------------------
  def setup_battle_event
    # If battle event is running
    if $game_system.battle_interpreter.running?
      return
    end
    # Search for all battle event pages
    for index in 0...$data_troops[@troop_id].pages.size
      # Get event pages
      page = $data_troops[@troop_id].pages[index]
      # Make event conditions possible for reference with c
      c = page.condition
      # Go to next page if no conditions are appointed
      unless c.turn_valid or c.enemy_valid or
             c.actor_valid or c.switch_valid
        next
      end
      # Go to next page if action has been completed
      if $game_temp.battle_event_flags[index]
        next
      end
      # Confirm turn conditions
      if c.turn_valid
        n = $game_temp.battle_turn
        a = c.turn_a
        b = c.turn_b
        if (b == 0 and n != a) or
           (b > 0 and (n < 1 or n < a or n % b != a % b))
          next
        end
      end
      # Confirm enemy conditions
      if c.enemy_valid
        enemy = $game_troop.enemies[c.enemy_index]
        if enemy == nil or enemy.hp * 100.0 / enemy.maxhp > c.enemy_hp
          next
        end
      end
      # Confirm actor conditions
      if c.actor_valid
        actor = $game_actors[c.actor_id]
        if actor == nil or actor.hp * 100.0 / actor.maxhp > c.actor_hp
          next
        end
      end
      # Confirm switch conditions
      if c.switch_valid
        if $game_switches[c.switch_id] == false
          next
        end
      end
      # Set up event
      $game_system.battle_interpreter.setup(page.list, 0)
      # If this page span is [battle] or [turn]
      if page.span <= 1
        # Set action completed flag
        $game_temp.battle_event_flags[index] = true
      end
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # If battle event is running
    if $game_system.battle_interpreter.running?
      # Update interpreter
      $game_system.battle_interpreter.update
      # If a battler which is forcing actions doesn't exist
      if $game_temp.forcing_battler == nil
        # If battle event has finished running
        unless $game_system.battle_interpreter.running?
          # Rerun battle event set up if battle continues
          unless judge
            setup_battle_event
          end
        end
        # If not after battle phase
        if @phase != 5
          # Refresh status window
          @status_window.refresh
        end
      end
    end
    # Update system (timer) and screen
    $game_system.update
    $game_screen.update
    # If timer has reached 0
    if $game_system.timer_working and $game_system.timer == 0
      # Abort battle
      $game_temp.battle_abort = true
    end
    # Update windows
    @help_window.update
    @party_command_window.update
    @actor_command_window.update
    @status_window.update
    @message_window.update
    # Update sprite set
    @spriteset.update
    # If transition is processing
    if $game_temp.transition_processing
      # Clear transition processing flag
      $game_temp.transition_processing = false
      # Execute transition
      if $game_temp.transition_name == ""
        Graphics.transition(20)
      else
        Graphics.transition(40, "Graphics/Transitions/" +
          $game_temp.transition_name)
      end
    end
    # If message window is showing
    if $game_temp.message_window_showing
      return
    end
    # If effect is showing
    if @spriteset.effect?
      return
    end
    # If game over
    if $game_temp.gameover
      # Switch to game over screen
      $scene = Scene_Gameover.new
      return
    end
    # If returning to title screen
    if $game_temp.to_title
      # Switch to title screen
      $scene = Scene_Title.new
      return
    end
    # If battle is aborted
    if $game_temp.battle_abort
      # Return to BGM used before battle started
      $game_system.bgm_play($game_temp.map_bgm)
      # Battle ends
      battle_end(1)
      return
    end
    # If waiting
    if @wait_count > 0
      # Decrease wait count
      @wait_count -= 1
      return
    end
    # If battler forcing an action doesn't exist,
    # and battle event is running
    if $game_temp.forcing_battler == nil and
       $game_system.battle_interpreter.running?
      return
    end
    # Branch according to phase
    case @phase
    when 1  # pre-battle phase
      update_phase1
    when 2  # party command phase
      update_phase2
    when 3  # actor command phase
      update_phase3
    when 4  # main phase
      update_phase4
    when 5  # after battle phase
      update_phase5
    end
  end
end
