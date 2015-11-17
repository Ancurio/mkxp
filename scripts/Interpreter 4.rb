#==============================================================================
# ** Interpreter (part 4)
#------------------------------------------------------------------------------
#  This interpreter runs event commands. This class is used within the
#  Game_System class and the Game_Event class.
#==============================================================================

class Interpreter
  #--------------------------------------------------------------------------
  # * Control Switches
  #--------------------------------------------------------------------------
  def command_121
    # Loop for group control
    for i in @parameters[0] .. @parameters[1]
      # Change switch
      $game_switches[i] = (@parameters[2] == 0)
    end
    # Refresh map
    $game_map.need_refresh = true
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Control Variables
  #--------------------------------------------------------------------------
  def command_122
    # Initialize value
    value = 0
    # Branch with operand
    case @parameters[3]
    when 0  # invariable
      value = @parameters[4]
    when 1  # variable
      value = $game_variables[@parameters[4]]
    when 2  # random number
      value = @parameters[4] + rand(@parameters[5] - @parameters[4] + 1)
    when 3  # item
      value = $game_party.item_number(@parameters[4])
    when 4  # actor
      actor = $game_actors[@parameters[4]]
      if actor != nil
        case @parameters[5]
        when 0  # level
          value = actor.level
        when 1  # EXP
          value = actor.exp
        when 2  # HP
          value = actor.hp
        when 3  # SP
          value = actor.sp
        when 4  # MaxHP
          value = actor.maxhp
        when 5  # MaxSP
          value = actor.maxsp
        when 6  # strength
          value = actor.str
        when 7  # dexterity
          value = actor.dex
        when 8  # agility
          value = actor.agi
        when 9  # intelligence
          value = actor.int
        when 10  # attack power
          value = actor.atk
        when 11  # physical defense
          value = actor.pdef
        when 12  # magic defense
          value = actor.mdef
        when 13  # evasion
          value = actor.eva
        end
      end
    when 5  # enemy
      enemy = $game_troop.enemies[@parameters[4]]
      if enemy != nil
        case @parameters[5]
        when 0  # HP
          value = enemy.hp
        when 1  # SP
          value = enemy.sp
        when 2  # MaxHP
          value = enemy.maxhp
        when 3  # MaxSP
          value = enemy.maxsp
        when 4  # strength
          value = enemy.str
        when 5  # dexterity
          value = enemy.dex
        when 6  # agility
          value = enemy.agi
        when 7  # intelligence
          value = enemy.int
        when 8  # attack power
          value = enemy.atk
        when 9  # physical defense
          value = enemy.pdef
        when 10  # magic defense
          value = enemy.mdef
        when 11  # evasion correction
          value = enemy.eva
        end
      end
    when 6  # character
      character = get_character(@parameters[4])
      if character != nil
        case @parameters[5]
        when 0  # x-coordinate
          value = character.x
        when 1  # y-coordinate
          value = character.y
        when 2  # direction
          value = character.direction
        when 3  # screen x-coordinate
          value = character.screen_x
        when 4  # screen y-coordinate
          value = character.screen_y
        when 5  # terrain tag
          value = character.terrain_tag
        end
      end
    when 7  # other
      case @parameters[4]
      when 0  # map ID
        value = $game_map.map_id
      when 1  # number of party members
        value = $game_party.actors.size
      when 2  # gold
        value = $game_party.gold
      when 3  # steps
        value = $game_party.steps
      when 4  # play time
        value = Graphics.frame_count / Graphics.frame_rate
      when 5  # timer
        value = $game_system.timer / Graphics.frame_rate
      when 6  # save count
        value = $game_system.save_count
      end
    end
    # Loop for group control
    for i in @parameters[0] .. @parameters[1]
      # Branch with control
      case @parameters[2]
      when 0  # substitute
        $game_variables[i] = value
      when 1  # add
        $game_variables[i] += value
      when 2  # subtract
        $game_variables[i] -= value
      when 3  # multiply
        $game_variables[i] *= value
      when 4  # divide
        if value != 0
          $game_variables[i] /= value
        end
      when 5  # remainder
        if value != 0
          $game_variables[i] %= value
        end
      end
      # Maximum limit check
      if $game_variables[i] > 99999999
        $game_variables[i] = 99999999
      end
      # Minimum limit check
      if $game_variables[i] < -99999999
        $game_variables[i] = -99999999
      end
    end
    # Refresh map
    $game_map.need_refresh = true
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Control Self Switch
  #--------------------------------------------------------------------------
  def command_123
    # If event ID is valid
    if @event_id > 0
      # Make a self switch key
      key = [$game_map.map_id, @event_id, @parameters[0]]
      # Change self switches
      $game_self_switches[key] = (@parameters[1] == 0)
    end
    # Refresh map
    $game_map.need_refresh = true
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Control Timer
  #--------------------------------------------------------------------------
  def command_124
    # If started
    if @parameters[0] == 0
      $game_system.timer = @parameters[1] * Graphics.frame_rate
      $game_system.timer_working = true
    end
    # If stopped
    if @parameters[0] == 1
      $game_system.timer_working = false
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Gold
  #--------------------------------------------------------------------------
  def command_125
    # Get value to operate
    value = operate_value(@parameters[0], @parameters[1], @parameters[2])
    # Increase / decrease amount of gold
    $game_party.gain_gold(value)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Items
  #--------------------------------------------------------------------------
  def command_126
    # Get value to operate
    value = operate_value(@parameters[1], @parameters[2], @parameters[3])
    # Increase / decrease items
    $game_party.gain_item(@parameters[0], value)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Weapons
  #--------------------------------------------------------------------------
  def command_127
    # Get value to operate
    value = operate_value(@parameters[1], @parameters[2], @parameters[3])
    # Increase / decrease weapons
    $game_party.gain_weapon(@parameters[0], value)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Armor
  #--------------------------------------------------------------------------
  def command_128
    # Get value to operate
    value = operate_value(@parameters[1], @parameters[2], @parameters[3])
    # Increase / decrease armor
    $game_party.gain_armor(@parameters[0], value)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Party Member
  #--------------------------------------------------------------------------
  def command_129
    # Get actor
    actor = $game_actors[@parameters[0]]
    # If actor is valid
    if actor != nil
      # Branch with control
      if @parameters[1] == 0
        if @parameters[2] == 1
          $game_actors[@parameters[0]].setup(@parameters[0])
        end
        $game_party.add_actor(@parameters[0])
      else
        $game_party.remove_actor(@parameters[0])
      end
    end
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Windowskin
  #--------------------------------------------------------------------------
  def command_131
    # Change windowskin file name
    $game_system.windowskin_name = @parameters[0]
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Battle BGM
  #--------------------------------------------------------------------------
  def command_132
    # Change battle BGM
    $game_system.battle_bgm = @parameters[0]
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Battle End ME
  #--------------------------------------------------------------------------
  def command_133
    # Change battle end ME
    $game_system.battle_end_me = @parameters[0]
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Save Access
  #--------------------------------------------------------------------------
  def command_134
    # Change save access flag
    $game_system.save_disabled = (@parameters[0] == 0)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Menu Access
  #--------------------------------------------------------------------------
  def command_135
    # Change menu access flag
    $game_system.menu_disabled = (@parameters[0] == 0)
    # Continue
    return true
  end
  #--------------------------------------------------------------------------
  # * Change Encounter
  #--------------------------------------------------------------------------
  def command_136
    # Change encounter flag
    $game_system.encounter_disabled = (@parameters[0] == 0)
    # Make encounter count
    $game_player.make_encounter_count
    # Continue
    return true
  end
end
