#==============================================================================
# ** Scene_Battle (part 2)
#------------------------------------------------------------------------------
#  This class performs battle screen processing.
#==============================================================================

class Scene_Battle
  #--------------------------------------------------------------------------
  # * Start Pre-Battle Phase
  #--------------------------------------------------------------------------
  def start_phase1
    # Shift to phase 1
    @phase = 1
    # Clear all party member actions
    $game_party.clear_actions
    # Set up battle event
    setup_battle_event
  end
  #--------------------------------------------------------------------------
  # * Frame Update (pre-battle phase)
  #--------------------------------------------------------------------------
  def update_phase1
    # Determine win/loss situation
    if judge
      # If won or lost: end method
      return
    end
    # Start party command phase
    start_phase2
  end
  #--------------------------------------------------------------------------
  # * Start Party Command Phase
  #--------------------------------------------------------------------------
  def start_phase2
    # Shift to phase 2
    @phase = 2
    # Set actor to non-selecting
    @actor_index = -1
    @active_battler = nil
    # Enable party command window
    @party_command_window.active = true
    @party_command_window.visible = true
    # Disable actor command window
    @actor_command_window.active = false
    @actor_command_window.visible = false
    # Clear main phase flag
    $game_temp.battle_main_phase = false
    # Clear all party member actions
    $game_party.clear_actions
    # If impossible to input command
    unless $game_party.inputable?
      # Start main phase
      start_phase4
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (party command phase)
  #--------------------------------------------------------------------------
  def update_phase2
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # Branch by party command window cursor position
      case @party_command_window.index
      when 0  # fight
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Start actor command phase
        start_phase3
      when 1  # escape
        # If it's not possible to escape
        if $game_temp.battle_can_escape == false
          # Play buzzer SE
          $game_system.se_play($data_system.buzzer_se)
          return
        end
        # Play decision SE
        $game_system.se_play($data_system.decision_se)
        # Escape processing
        update_phase2_escape
      end
      return
    end
  end
  #--------------------------------------------------------------------------
  # * Frame Update (party command phase: escape)
  #--------------------------------------------------------------------------
  def update_phase2_escape
    # Calculate enemy agility average
    enemies_agi = 0
    enemies_number = 0
    for enemy in $game_troop.enemies
      if enemy.exist?
        enemies_agi += enemy.agi
        enemies_number += 1
      end
    end
    if enemies_number > 0
      enemies_agi /= enemies_number
    end
    # Calculate actor agility average
    actors_agi = 0
    actors_number = 0
    for actor in $game_party.actors
      if actor.exist?
        actors_agi += actor.agi
        actors_number += 1
      end
    end
    if actors_number > 0
      actors_agi /= actors_number
    end
    # Determine if escape is successful
    success = rand(100) < 50 * actors_agi / enemies_agi
    # If escape is successful
    if success
      # Play escape SE
      $game_system.se_play($data_system.escape_se)
      # Return to BGM before battle started
      $game_system.bgm_play($game_temp.map_bgm)
      # Battle ends
      battle_end(1)
    # If escape is failure
    else
      # Clear all party member actions
      $game_party.clear_actions
      # Start main phase
      start_phase4
    end
  end
  #--------------------------------------------------------------------------
  # * Start After Battle Phase
  #--------------------------------------------------------------------------
  def start_phase5
    # Shift to phase 5
    @phase = 5
    # Play battle end ME
    $game_system.me_play($game_system.battle_end_me)
    # Return to BGM before battle started
    $game_system.bgm_play($game_temp.map_bgm)
    # Initialize EXP, amount of gold, and treasure
    exp = 0
    gold = 0
    treasures = []
    # Loop
    for enemy in $game_troop.enemies
      # If enemy is not hidden
      unless enemy.hidden
        # Add EXP and amount of gold obtained
        exp += enemy.exp
        gold += enemy.gold
        # Determine if treasure appears
        if rand(100) < enemy.treasure_prob
          if enemy.item_id > 0
            treasures.push($data_items[enemy.item_id])
          end
          if enemy.weapon_id > 0
            treasures.push($data_weapons[enemy.weapon_id])
          end
          if enemy.armor_id > 0
            treasures.push($data_armors[enemy.armor_id])
          end
        end
      end
    end
    # Treasure is limited to a maximum of 6 items
    treasures = treasures[0..5]
    # Obtaining EXP
    for i in 0...$game_party.actors.size
      actor = $game_party.actors[i]
      if actor.cant_get_exp? == false
        last_level = actor.level
        actor.exp += exp
        if actor.level > last_level
          @status_window.level_up(i)
        end
      end
    end
    # Obtaining gold
    $game_party.gain_gold(gold)
    # Obtaining treasure
    for item in treasures
      case item
      when RPG::Item
        $game_party.gain_item(item.id, 1)
      when RPG::Weapon
        $game_party.gain_weapon(item.id, 1)
      when RPG::Armor
        $game_party.gain_armor(item.id, 1)
      end
    end
    # Make battle result window
    @result_window = Window_BattleResult.new(exp, gold, treasures)
    # Set wait count
    @phase5_wait_count = 100
  end
  #--------------------------------------------------------------------------
  # * Frame Update (after battle phase)
  #--------------------------------------------------------------------------
  def update_phase5
    # If wait count is larger than 0
    if @phase5_wait_count > 0
      # Decrease wait count
      @phase5_wait_count -= 1
      # If wait count reaches 0
      if @phase5_wait_count == 0
        # Show result window
        @result_window.visible = true
        # Clear main phase flag
        $game_temp.battle_main_phase = false
        # Refresh status window
        @status_window.refresh
      end
      return
    end
    # If C button was pressed
    if Input.trigger?(Input::ACTION)
      # Battle ends
      battle_end(0)
    end
  end
end
